#include <linux/version.h>
#include <linux/kconfig.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/falloc.h>
#include <linux/miscdevice.h>
#include <linux/security.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/uaccess.h>
#include <linux/personality.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/shmem_fs.h>
#include <linux/completion.h>
#include <asm/atomic.h>
#include <linux/workqueue.h>

#include "nx_ashmem.h"
#include "nexus_platform.h"
#include "nexus_base_mmap.h"
#if !defined(V3D_VARIANT_v3d)
#include "bvc5_bin_pool_alloc_priv.h"
#endif

static int gfx_alloc_dbg = 0;
static int gfx_move_dbg = 0;
static int gfx_refcnt_dbg = 0;

/* block management mode - 1) locked: cannot move blocks, blocks are always
 * pinned and/or mapped by default, this is the legacy and default mode
 * of operations; 2) unlocked: can move blocks, blocks are only pinned and/or
 * mapped on demand, this mode is needed for compaction to work.
 */
#define BLOCK_MGMT_MODE_LOCKED "locked"
static char block_mgmt_mode[16]=BLOCK_MGMT_MODE_LOCKED;
module_param_string(block_mgmt, block_mgmt_mode, sizeof(block_mgmt_mode), 0);

static char nx_devname[16]="/dev/nx_ashmem";
module_param_string(devname, nx_devname, sizeof(nx_devname), 0);

#define GFX_UHD_FB (4096*2160*4)
static char gfx_heap_grow[16] ="4k";
static unsigned long gfx_heap_grow_size = 0;
module_param_string(heap_grow, gfx_heap_grow, sizeof(gfx_heap_grow), 0);
static int gfx_heap_grow_factor = 0;

static DEFINE_MUTEX(nx_ashmem_mutex);

static struct kmem_cache *nx_ashmem_area_cachep __read_mostly;

struct nx_ashmem_state {
   struct mutex block_lock;
   struct list_head block_list;

   NEXUS_HeapHandle gfx_heap;
   NEXUS_HeapHandle gfx_fb_heap;
   NEXUS_HeapHandle gfx_alt_heap[2];

   struct nx_ashmem_mgr_cfg mgr_cfg;
   int gfx_heap_dyn;

   struct mutex video_lock;
   int video_in_use;
   int video_secure_in_use;

#if !defined(V3D_VARIANT_v3d)
   BVC5_BinPoolBlock_MemInterface memIf;
#endif
};

static struct nx_ashmem_state *nx_ashmem_global;

/*
* nx_ashmem_area - anonymous shared memory area
* Lifecycle: From our parent file's open() until its release()
* Locking: Protected by `ashmem_mutex'
* Big Note: it dies on close()
*/
struct nx_ashmem_area {
   struct list_head block_list;
   NEXUS_Addr p_address;               /* pointer to physical address in heap of the allocation */
   NEXUS_MemoryBlockHandle block;      /* nexus block handle for the allocation */
   size_t size;                        /* size of the mapping, in bytes */
   size_t align;                       /* alignment of the mapping, in bytes */
   int pid_alloc;
   size_t movable;
   NEXUS_HeapHandle heap_alloc;
   size_t marker;
   int ext_refcnt;
   int defer_free;
   int heap_wanted;
   int refcnt;
};

#if !defined(V3D_VARIANT_v3d)
static int gfx_alloc_bin_dbg = 0;

static BMMA_Block_Handle nx_ashmem_memif_alloc(BMMA_Heap_Handle context, size_t size, uint32_t align)
{
   NEXUS_MemoryBlockHandle block = NULL;
   size_t allocation = (size + (align-1)) & ~(align-1);

   (void)context;

   block = NEXUS_MemoryBlock_Allocate(nx_ashmem_global->gfx_heap, allocation, align, NULL);
   if (nx_ashmem_global->gfx_heap_dyn && (block == NULL)) {
      int rc;
      size_t dyn_heap_grow_adjust = (size_t)gfx_heap_grow_size;
      if ((allocation > (size_t)gfx_heap_grow_size) &&
          (allocation <= (size_t)(gfx_heap_grow_factor * gfx_heap_grow_size))) {
         dyn_heap_grow_adjust = allocation;
      }
      rc = NEXUS_Platform_GrowHeap(nx_ashmem_global->gfx_heap, dyn_heap_grow_adjust);
      if (rc == 0) {
         block = NEXUS_MemoryBlock_Allocate(nx_ashmem_global->gfx_heap, allocation, align, NULL);
      }
   }

   if (gfx_alloc_bin_dbg) {
      pr_info("nx::alt-alloc:%s::%lx::sz:%u::al:%u\n",
              nx_ashmem_global->gfx_heap_dyn ? "block-handle" : "address",
              (long)block, size, align);
   }

   if (block == NULL) {
      return NULL;
   } else {
      return (BMMA_Block_Handle)block;
   }
}

static void nx_ashmem_memif_free(BMMA_Block_Handle block)
{
   NEXUS_MemoryBlock_Free((NEXUS_MemoryBlockHandle)block);

   if (gfx_alloc_bin_dbg) {
      pr_info("nx::alt-free:%s::%lx\n",
              nx_ashmem_global->gfx_heap_dyn ? "block-handle" : "address",
              (long)block);
   }
}

static void nx_ashmem_memif_unlock(BMMA_Block_Handle block, BMMA_DeviceOffset off)
{
   (void)off;
   NEXUS_MemoryBlock_UnlockOffset((NEXUS_MemoryBlockHandle)block);
}

static void nx_ashmem_memif_lock(BMMA_Block_Handle block, BMMA_DeviceOffset *off, uint32_t *phys)
{
   NEXUS_Addr addr;
   NEXUS_MemoryBlock_LockOffset((NEXUS_MemoryBlockHandle)block, &addr);
   *off = (BMMA_DeviceOffset)addr;
   *phys = *off & 0xFFFFFFFF;
}
#endif

static void nx_ashmem_move_block_worker(struct work_struct *work)
{
   bool in_error = false;
   NEXUS_Error rc;

   mutex_lock(&nx_ashmem_global->video_lock);
   while (true) {
      unsigned num[2] = {0, 0};
      unsigned move = 10;

      if ((nx_ashmem_global->gfx_alt_heap[0] == NULL) && (nx_ashmem_global->gfx_alt_heap[1] == NULL)) break;

move_alt0:
      if (nx_ashmem_global->gfx_alt_heap[0]) {
         rc = NEXUS_Memory_MoveUnlockedBlocks(nx_ashmem_global->gfx_alt_heap[0], nx_ashmem_global->gfx_heap, move, &num[0]);
         if (rc == BERR_OUT_OF_DEVICE_MEMORY) {
            rc = NEXUS_Platform_GrowHeap(nx_ashmem_global->gfx_heap, (size_t)gfx_heap_grow_size);
            if (rc == 0) {
               goto move_alt0;
            }
         }
         if (rc || num[0] == 0) {
            in_error = true;
         }
         if (gfx_move_dbg) pr_info("nx::moved:%u::alt0:%p::main:%p::rc:%d\n",
            num[0], nx_ashmem_global->gfx_alt_heap[0], nx_ashmem_global->gfx_heap, rc);
      }

move_alt1:
      if (nx_ashmem_global->gfx_alt_heap[1]) {
         rc = NEXUS_Memory_MoveUnlockedBlocks(nx_ashmem_global->gfx_alt_heap[1], nx_ashmem_global->gfx_heap, move, &num[1]);
         if (rc == BERR_OUT_OF_DEVICE_MEMORY) {
            rc = NEXUS_Platform_GrowHeap(nx_ashmem_global->gfx_heap, (size_t)gfx_heap_grow_size);
            if (rc == 0) {
               goto move_alt1;
            }
         }
         if (rc || num[1] == 0) {
            in_error = true;
         }
         if (gfx_move_dbg) pr_info("nx::moved:%u::alt1:%p::main:%p::rc:%d\n",
            num[1], nx_ashmem_global->gfx_alt_heap[1], nx_ashmem_global->gfx_heap, rc);
      }

      if (in_error) break;
   }
   mutex_unlock(&nx_ashmem_global->video_lock);
}
static DECLARE_WORK(nx_move_block_work, nx_ashmem_move_block_worker);

static void nx_ashmem_process_marker(size_t marker, bool add)
{
   bool heap_move = false;

   mutex_lock(&(nx_ashmem_global->video_lock));
   if ((marker == NX_ASHMEM_MARKER_VIDEO_DECODER) || (marker == NX_ASHMEM_MARKER_VIDEO_ENCODER)) {
      if (add) {
         nx_ashmem_global->video_in_use++;
      } else {
         nx_ashmem_global->video_in_use--;
         /* protect rogue, should really not happen. */
         if (nx_ashmem_global->video_in_use < 0) {
            nx_ashmem_global->video_in_use = 0;
         }
      }
   } else if (marker == NX_ASHMEM_MARKER_VIDEO_DEC_SECURE) {
      if (add) {
         nx_ashmem_global->video_secure_in_use++;
      } else {
         nx_ashmem_global->video_secure_in_use--;
         /* protect rogue, should really not happen. */
         if (nx_ashmem_global->video_secure_in_use < 0) {
            nx_ashmem_global->video_secure_in_use = 0;
         }
      }
   }

   if (nx_ashmem_global->video_in_use || nx_ashmem_global->video_secure_in_use) {
      if ((nx_ashmem_global->gfx_alt_heap[0] != NULL) || (nx_ashmem_global->gfx_alt_heap[1] != NULL)) {
         heap_move = true;
      }
   }

   if (heap_move) {
      schedule_work(&nx_move_block_work);
   }
   mutex_unlock(&(nx_ashmem_global->video_lock));
}

static bool nx_ashmem_allocate_from_alternate(size_t current_main, bool *alt_index)
{
   bool ret = false;
   NEXUS_MemoryStatus memStatus;
   NEXUS_Error rc;

   mutex_lock(&(nx_ashmem_global->video_lock));
   if (nx_ashmem_global->video_in_use || nx_ashmem_global->video_secure_in_use) {
      mutex_unlock(&(nx_ashmem_global->video_lock));
      goto out;
   }
   mutex_unlock(&(nx_ashmem_global->video_lock));

   alt_index[0] = false;
   alt_index[1] = false;

   if (current_main > nx_ashmem_global->mgr_cfg.alt_use_threshold) {
      if (nx_ashmem_global->gfx_alt_heap[0] != NULL) {
         rc = NEXUS_Heap_GetStatus(nx_ashmem_global->gfx_alt_heap[0], &memStatus);
         if (rc == NEXUS_SUCCESS) {
            unsigned allocated = 0, size_percent = 1;
            size_percent = memStatus.size / 100;
            allocated = (memStatus.size - memStatus.free);
            if ((allocated/size_percent) < nx_ashmem_global->mgr_cfg.alt_use_max[0]) {
               alt_index[0] = true;
            }
         }
      }
      if (nx_ashmem_global->gfx_alt_heap[1] != NULL) {
         rc = NEXUS_Heap_GetStatus(nx_ashmem_global->gfx_alt_heap[1], &memStatus);
         if (rc == NEXUS_SUCCESS) {
            unsigned allocated = 0, size_percent = 1;
            size_percent = memStatus.size / 100;
            allocated = (memStatus.size - memStatus.free);
            if ((allocated/size_percent) < nx_ashmem_global->mgr_cfg.alt_use_max[1]) {
               alt_index[1] = true;
            }
         }
      }

      if (alt_index[0] || alt_index[1]) {
         ret = true;
      }
   }

out:
   return ret;
}

static int nx_ashmem_open(struct inode *inode, struct file *file)
{
   struct nx_ashmem_area *asma;
   int ret;

   ret = generic_file_open(inode, file);
   if (unlikely(ret))
      return ret;

   asma = kmem_cache_zalloc(nx_ashmem_area_cachep, GFP_KERNEL);
   if (unlikely(!asma))
      return -ENOMEM;

   asma->pid_alloc = current->tgid;
   asma->ext_refcnt = 0;
   asma->defer_free = 0;
   asma->refcnt = 0;
   file->private_data = asma;

   mutex_lock(&(nx_ashmem_global->block_lock));
   list_add(&asma->block_list, &nx_ashmem_global->block_list);
   mutex_unlock(&(nx_ashmem_global->block_lock));

   return 0;
}

static void nx_ashmem_asma_free_block(struct nx_ashmem_area *asma)
{
   if (asma->block) {
      if (!asma->movable) {
         if (!strncmp(block_mgmt_mode, BLOCK_MGMT_MODE_LOCKED, sizeof(BLOCK_MGMT_MODE_LOCKED))) {
            if (asma->p_address) {
               NEXUS_MemoryBlock_UnlockOffset(asma->block);
               asma->p_address = (NEXUS_Addr)0;
            }
         }
      }
      NEXUS_MemoryBlock_Free(asma->block);
      asma->block = NULL;
   }
}

static int nx_ashmem_release(struct inode *ignored, struct file *file)
{
   struct nx_ashmem_area *asma = file->private_data;
   int marker = asma->marker;

   mutex_lock(&(nx_ashmem_global->block_lock));
   if (asma->block) {
      if (asma->ext_refcnt > 0) {
         if (gfx_alloc_dbg) {
            pr_err("nx::free-deferred:%s::%lx::%lx::%lx::sz:%u::al:%u:cnt:%d\n",
                   (nx_ashmem_global->gfx_heap_dyn ? "block-handle" : "address"),
                   (long)asma, (long)asma->block, (long)asma->p_address, asma->size, asma->align, asma->ext_refcnt);
         }
         asma->defer_free = 1;
         mutex_unlock(&(nx_ashmem_global->block_lock));
      } else {
         if (gfx_alloc_dbg) {
            pr_info("nx::free:%s::%lx::%lx::%lx::sz:%u::al:%u:cnt:%d\n",
                    (nx_ashmem_global->gfx_heap_dyn ? "block-handle" : "address"),
                    (long)asma, (long)asma->block, (long)asma->p_address, asma->size, asma->align, asma->ext_refcnt);
         }
         nx_ashmem_asma_free_block(asma);
         list_del(&asma->block_list);
         mutex_unlock(&(nx_ashmem_global->block_lock));
         kmem_cache_free(nx_ashmem_area_cachep, asma);
      }
   } else {
      list_del(&asma->block_list);
      mutex_unlock(&(nx_ashmem_global->block_lock));
      kmem_cache_free(nx_ashmem_area_cachep, asma);
   }

   if (marker)
      nx_ashmem_process_marker(marker, false);

   return 0;
}

static long nx_ashmem_mmap(struct file *file, struct nx_ashmem_getmem *getmem)
{
   struct nx_ashmem_area *asma = file->private_data;
   NEXUS_MemoryBlockHandle block = NULL;
   NEXUS_Addr addr = 0;
   NEXUS_MemoryStatus memStatus;
   NEXUS_Error rc;
   size_t allocation;
   bool alt_index[2] = {false, false};
   NEXUS_HeapHandle heap_allocator = nx_ashmem_global->gfx_heap;

   getmem->hdl = 0;
   if (unlikely(!asma->size))
      goto out;
   if (asma->block)
      goto asma_block_allocated;

   if (asma->movable) {
      allocation = (asma->size + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
      rc = NEXUS_Heap_GetStatus(heap_allocator, &memStatus);
      if (rc == NEXUS_SUCCESS) {
         if (nx_ashmem_allocate_from_alternate(memStatus.size, alt_index)) {
            if (alt_index[0]) {
               block = NEXUS_MemoryBlock_Allocate(nx_ashmem_global->gfx_alt_heap[0], allocation, PAGE_SIZE, NULL);
               if (block != NULL) {
                  asma->heap_alloc = nx_ashmem_global->gfx_alt_heap[0];
                  goto block_allocated;
               }
            }
            if (alt_index[1]) {
               block = NEXUS_MemoryBlock_Allocate(nx_ashmem_global->gfx_alt_heap[1], allocation, PAGE_SIZE, NULL);
               if (block != NULL) {
                  asma->heap_alloc = nx_ashmem_global->gfx_alt_heap[1];
                  goto block_allocated;
               }
            }
         }
      }
   }

   allocation = (asma->size + (asma->align-1)) & ~(asma->align-1);
   if (asma->heap_wanted == NX_ASHMEM_HEAP_FB) {
      heap_allocator = nx_ashmem_global->gfx_fb_heap;
   }
   asma->heap_alloc = heap_allocator;

   block = NEXUS_MemoryBlock_Allocate(heap_allocator, allocation, asma->align, NULL);
   if (nx_ashmem_global->gfx_heap_dyn && (heap_allocator == nx_ashmem_global->gfx_heap) && (block == NULL)) {
      int rc;
      size_t dyn_heap_grow_adjust = (size_t)gfx_heap_grow_size;
      if ((allocation > (size_t)gfx_heap_grow_size) &&
          (allocation <= (size_t)(gfx_heap_grow_factor * gfx_heap_grow_size))) {
         dyn_heap_grow_adjust = (allocation + (PAGE_SIZE-1)) & ~(PAGE_SIZE -1);
      }
      rc = NEXUS_Platform_GrowHeap(heap_allocator, dyn_heap_grow_adjust);
      if (rc == 0) {
         block = NEXUS_MemoryBlock_Allocate(heap_allocator, allocation, asma->align, NULL);
      }
   }

   if (unlikely(block == NULL)) {
      pr_err("nx::alloc-failure::%p::sz:%u::al:%u\n", heap_allocator, asma->size, asma->align);
      goto out;
   }

block_allocated:
   if (asma->movable) {
      asma->p_address = (NEXUS_Addr)0;
   } else if (!strncmp(block_mgmt_mode, BLOCK_MGMT_MODE_LOCKED, sizeof(BLOCK_MGMT_MODE_LOCKED))) {
      NEXUS_MemoryBlock_LockOffset(block, &addr);
      asma->p_address = addr;
   } else {
      asma->p_address = (NEXUS_Addr)0;
   }
   asma->block = block;

asma_block_allocated:
   if (nx_ashmem_global->gfx_heap_dyn) {
      NEXUS_Platform_SetSharedHandle(asma->block, true);
      getmem->hdl = asma->block;
   } else {
      getmem->hdl = asma->p_address;
   }

out:
   if (gfx_alloc_dbg) pr_info("nx::alloc:%s::%lx::%lx::%lx::sz:%u::al:%u\n",
                              nx_ashmem_global->gfx_heap_dyn ? "block-handle" : "address",
                              (long)asma, (long)asma->block, (long)asma->p_address, asma->size, asma->align);
   return 0;
}

static int nx_ashmem_process_ext_refcnt(NEXUS_MemoryBlockHandle hdl, int cnt)
{
   struct nx_ashmem_area *block = NULL, *free_block = NULL;
   int marked = 0;
   int ret = 0;

   mutex_lock(&(nx_ashmem_global->block_lock));
   if (!list_empty(&nx_ashmem_global->block_list)) {
      list_for_each_entry(block, &nx_ashmem_global->block_list, block_list) {
         if (block->block && (block->block == hdl)) {
            marked = 1;
            if (cnt == NX_ASHMEM_REFCNT_ADD) {
               block->ext_refcnt++;
            } else if (cnt == NX_ASHMEM_REFCNT_REM) {
               block->ext_refcnt--;
            } else {
               ret = -EINVAL;
               pr_err("nx::refcnt-invalid::%lx::%lx::sz:%u::al:%u:cnt:%d\n",
                       (long)block->block, (long)block->p_address, block->size, block->align, block->ext_refcnt);
               mutex_unlock(&(nx_ashmem_global->block_lock));
               goto out;
            }
            if (block->ext_refcnt < 0) {
               block->ext_refcnt = 0;
               pr_info("nx::refcnt-underrun::%lx::%lx::sz:%u::al:%u\n",
                       (long)block->block, (long)block->p_address, block->size, block->align);
            } else if (gfx_refcnt_dbg) {
               pr_info("nx::refcnt::%lx::%lx::sz:%u::al:%u:cnt:%d\n",
                       (long)block->block, (long)block->p_address, block->size, block->align, block->ext_refcnt);
            }
            if ((block->ext_refcnt == 0) && block->defer_free) {
               free_block = block;
            }
         }
      }
   }
   mutex_unlock(&(nx_ashmem_global->block_lock));

   if (!marked) {
      if (gfx_refcnt_dbg) {
         pr_info("nx::refcnt-failed::%lx::cnt:%d\n", (long)hdl, cnt);
      }
      ret = -ENOMEM;
   } else if (free_block != NULL) {
      if (gfx_alloc_dbg)
         pr_info("nx::free-delay:%s::%lx::%lx::%lx::sz:%u::al:%u:cnt:%d\n",
                 (nx_ashmem_global->gfx_heap_dyn ? "block-handle" : "address"),
                 (long)free_block, (long)free_block->block, (long)free_block->p_address, free_block->size, free_block->align, free_block->ext_refcnt);

      mutex_lock(&(nx_ashmem_global->block_lock));
      nx_ashmem_asma_free_block(free_block);
      list_del(&free_block->block_list);
      mutex_unlock(&(nx_ashmem_global->block_lock));
   }

out:
   return ret;
}

static int nx_ashmem_process_refcnt(NEXUS_MemoryBlockHandle hdl, int cnt, int *rel)
{
   struct nx_ashmem_area *block = NULL, *free_block = NULL;
   int ret = 0;

   mutex_lock(&(nx_ashmem_global->block_lock));
   if (!list_empty(&nx_ashmem_global->block_list)) {
      list_for_each_entry(block, &nx_ashmem_global->block_list, block_list) {
         if (block->block && (block->block == hdl)) {
            if (cnt == NX_ASHMEM_REFCNT_ADD) {
               block->refcnt++;
            } else if (cnt == NX_ASHMEM_REFCNT_REM) {
               block->refcnt--;
            } else {
               ret = -EINVAL;
               mutex_unlock(&(nx_ashmem_global->block_lock));
               goto out;
            }
            if (block->refcnt == 0) {
               *rel = 1;
            }
            break;
         }
      }
   }
   mutex_unlock(&(nx_ashmem_global->block_lock));

out:
   return ret;
}

static void nx_ashmem_dump_all(void)
{
   struct nx_ashmem_area *block = NULL;
   long phys_addr = 0;

   mutex_lock(&(nx_ashmem_global->block_lock));
   if (!list_empty(&nx_ashmem_global->block_list)) {
      list_for_each_entry(block, &nx_ashmem_global->block_list, block_list) {
         if (block->block) {
            if (!strncmp(block_mgmt_mode, BLOCK_MGMT_MODE_LOCKED, sizeof(BLOCK_MGMT_MODE_LOCKED))) {
               phys_addr = (long)block->p_address;
            } else {
               NEXUS_MemoryBlock_LockOffset(block->block, &block->p_address);
               phys_addr = (long)block->p_address;
               NEXUS_MemoryBlock_UnlockOffset(block->block);
               block->p_address = 0;
            }
            pr_info("nx::hp:%p::mv:%d::pid:%d::blk:%lx:%lx::sz:%u::al:%u\n",
                    block->heap_alloc, block->movable, block->pid_alloc,
                    (long)block->block, phys_addr, block->size, block->align);
         }
      }
   }
   mutex_unlock(&(nx_ashmem_global->block_lock));
}

static long nx_ashmem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
   struct nx_ashmem_area *asma = file->private_data;
   struct nx_ashmem_alloc alloc;
   struct nx_ashmem_getmem getmem;
   NEXUS_Error rc;
   int alt_mem_index = -1;
   long ret = -ENOTTY;
   NEXUS_ClientConfiguration clientConfig;
   NEXUS_MemoryStatus memStatus;
   struct nx_ashmem_ext_refcnt ext_refcnt;
   struct nx_ashmem_refcnt refcnt;

   switch (cmd) {
   case NX_ASHMEM_SET_SIZE:
      mutex_lock(&nx_ashmem_mutex);
      if (!asma->p_address) {
         if (copy_from_user(&alloc, (void *)arg, sizeof(struct nx_ashmem_alloc)) != 0) {
            mutex_unlock(&nx_ashmem_mutex);
            return -EFAULT;
         }
         ret = 0;
         asma->size  = alloc.size;
         asma->align = alloc.align;
         asma->movable = alloc.movable;
         asma->marker = alloc.marker;
         asma->heap_wanted = alloc.heap;
         mutex_unlock(&nx_ashmem_mutex);
         nx_ashmem_process_marker(alloc.marker, true);
      } else {
         ret = -EINVAL;
         mutex_unlock(&nx_ashmem_mutex);
      }
      break;
   case NX_ASHMEM_GET_SIZE:
      mutex_lock(&nx_ashmem_mutex);
      if (copy_from_user(&alloc, (void *)arg, sizeof(struct nx_ashmem_alloc)) != 0) {
         mutex_unlock(&nx_ashmem_mutex);
         return -EFAULT;
      }
      alloc.size = asma->size;
      alloc.align = asma->align;
      ret = 0;
      if (copy_to_user((void *)arg, &alloc, sizeof(struct nx_ashmem_alloc)) != 0) {
         mutex_unlock(&(nx_ashmem_mutex));
         return -EFAULT;
      }
      mutex_unlock(&nx_ashmem_mutex);
      break;
   case NX_ASHMEM_GETMEM:
      mutex_lock(&nx_ashmem_mutex);
      if (copy_from_user(&getmem, (void *)arg, sizeof(struct nx_ashmem_getmem)) != 0) {
         mutex_unlock(&nx_ashmem_mutex);
         return -EFAULT;
      }
      nx_ashmem_mmap(file, &getmem);
      ret = 0;
      if (copy_to_user((void *)arg, &getmem, sizeof(struct nx_ashmem_getmem)) != 0) {
         mutex_unlock(&(nx_ashmem_mutex));
         return -EFAULT;
      }
      mutex_unlock(&nx_ashmem_mutex);
      break;
   case NX_ASHMEM_DUMP_ALL:
      nx_ashmem_dump_all();
      ret = 0;
      break;
   case NX_ASHMEM_EXT_REFCNT:
      mutex_lock(&nx_ashmem_mutex);
      if (copy_from_user(&ext_refcnt, (void *)arg, sizeof(struct nx_ashmem_ext_refcnt)) != 0) {
         mutex_unlock(&nx_ashmem_mutex);
         return -EFAULT;
      }
      ret = nx_ashmem_process_ext_refcnt(ext_refcnt.hdl, ext_refcnt.cnt);
      mutex_unlock(&nx_ashmem_mutex);
      break;
   case NX_ASHMEM_MGR_CFG:
      mutex_lock(&nx_ashmem_mutex);
      if (copy_from_user(&(nx_ashmem_global->mgr_cfg), (void *)arg, sizeof(struct nx_ashmem_mgr_cfg)) != 0) {
         mutex_unlock(&nx_ashmem_mutex);
         return -EFAULT;
      }
      NEXUS_Platform_GetClientConfiguration(&clientConfig);
      if (nx_ashmem_global->mgr_cfg.alt_use_threshold) {
         rc = NEXUS_Heap_GetStatus(nx_ashmem_global->gfx_heap, &memStatus);
         if (rc == NEXUS_SUCCESS) { alt_mem_index = memStatus.memcIndex; }
#if defined(NEXUS_MEMC1_PICTURE_BUFFER_HEAP)
         rc = NEXUS_Heap_GetStatus(clientConfig.heap[NEXUS_MEMC1_PICTURE_BUFFER_HEAP], &memStatus);
         if (rc == NEXUS_SUCCESS) {
            if (memStatus.memcIndex == alt_mem_index) {
               nx_ashmem_global->gfx_alt_heap[0] = clientConfig.heap[NEXUS_MEMC1_PICTURE_BUFFER_HEAP];
               if (nx_ashmem_global->mgr_cfg.alt_use_max[1] > 0) {
                  nx_ashmem_global->gfx_alt_heap[1] = clientConfig.heap[NEXUS_MEMC0_PICTURE_BUFFER_HEAP];
               }
            } else {
               nx_ashmem_global->gfx_alt_heap[0] = clientConfig.heap[NEXUS_MEMC0_PICTURE_BUFFER_HEAP];
               if (nx_ashmem_global->mgr_cfg.alt_use_max[1] > 1) {
                  nx_ashmem_global->gfx_alt_heap[1] = clientConfig.heap[NEXUS_MEMC1_PICTURE_BUFFER_HEAP];
               }
            }
         }
#else
         nx_ashmem_global->gfx_alt_heap[0] = clientConfig.heap[NEXUS_MEMC0_PICTURE_BUFFER_HEAP];
#endif
      }
      pr_info("nx::at:%u::alt0:%p:%d::alt1:%p:%d\n", nx_ashmem_global->mgr_cfg.alt_use_threshold,
              nx_ashmem_global->gfx_alt_heap[0], nx_ashmem_global->mgr_cfg.alt_use_max[0],
              nx_ashmem_global->gfx_alt_heap[1], nx_ashmem_global->mgr_cfg.alt_use_max[1]);
      ret = 0;
      mutex_unlock(&nx_ashmem_mutex);
      break;
   case NX_ASHMEM_CHK_SEC:
      mutex_lock(&(nx_ashmem_global->video_lock));
      if (copy_to_user((void *)arg, &nx_ashmem_global->video_secure_in_use, sizeof(nx_ashmem_global->video_secure_in_use)) != 0) {
         mutex_unlock(&(nx_ashmem_global->video_lock));
         return -EFAULT;
      }
      mutex_unlock(&(nx_ashmem_global->video_lock));
      break;
   case NX_ASHMEM_CHK_PLAY:
      {
         int video_in_use;
         mutex_lock(&(nx_ashmem_global->video_lock));
         video_in_use = (nx_ashmem_global->video_secure_in_use || nx_ashmem_global->video_in_use) ? 1 : 0;
         if (copy_to_user((void *)arg, &video_in_use, sizeof(video_in_use)) != 0) {
            mutex_unlock(&(nx_ashmem_global->video_lock));
            return -EFAULT;
         }
         mutex_unlock(&(nx_ashmem_global->video_lock));
      }
      break;
   case NX_ASHMEM_GET_BLK:
      mutex_lock(&nx_ashmem_mutex);
      if (copy_from_user(&getmem, (void *)arg, sizeof(struct nx_ashmem_getmem)) != 0) {
         mutex_unlock(&nx_ashmem_mutex);
         return -EFAULT;
      }
      getmem.hdl = asma->block;
      ret = 0;
      if (copy_to_user((void *)arg, &getmem, sizeof(struct nx_ashmem_getmem)) != 0) {
         mutex_unlock(&(nx_ashmem_mutex));
         return -EFAULT;
      }
      mutex_unlock(&nx_ashmem_mutex);
      break;
   case NX_ASHMEM_REFCNT:
      mutex_lock(&nx_ashmem_mutex);
      if (copy_from_user(&refcnt, (void *)arg, sizeof(struct nx_ashmem_refcnt)) != 0) {
         mutex_unlock(&nx_ashmem_mutex);
         return -EFAULT;
      }
      ret = nx_ashmem_process_refcnt(refcnt.hdl, refcnt.cnt, &refcnt.rel);
      if (copy_to_user((void *)arg, &refcnt, sizeof(struct nx_ashmem_refcnt)) != 0) {
         mutex_unlock(&nx_ashmem_mutex);
         return -EFAULT;
      }
      mutex_unlock(&nx_ashmem_mutex);
      break;
   }

   return ret;
}

static const struct file_operations nx_ashmem_fops = {
   .owner          = THIS_MODULE,
   .open           = nx_ashmem_open,
   .release        = nx_ashmem_release,
   .unlocked_ioctl = nx_ashmem_ioctl,
   .compat_ioctl   = nx_ashmem_ioctl,
};

static struct miscdevice nx_ashmem_misc = {
   .minor = MISC_DYNAMIC_MINOR,
   .name = nx_devname,
   .fops = &nx_ashmem_fops,
};

static int __init nx_ashmem_module_init(void)
{
   NEXUS_ClientConfiguration clientConfig;
   NEXUS_MemoryStatus memStatus;
   int i;
   int ret;
   BERR_Code err;

   nx_ashmem_area_cachep = KMEM_CACHE(nx_ashmem_area, 0);
   if (unlikely(!nx_ashmem_area_cachep)) {
      pr_err("failed to create slab cache\n");
      ret = -ENOMEM;
      goto error0;
   }

   ret = misc_register(&nx_ashmem_misc);
   if (unlikely(ret)) {
      pr_err("failed to register misc device\n");
      goto error1;
   }

   nx_ashmem_global = kzalloc(sizeof(struct nx_ashmem_state), GFP_KERNEL);
   if (unlikely(!nx_ashmem_global)) {
      pr_err("failed to create global state\n");
      ret = -ENOMEM;
      goto error1;
   }
   mutex_init(&nx_ashmem_global->block_lock);
   mutex_init(&nx_ashmem_global->video_lock);
   INIT_LIST_HEAD(&nx_ashmem_global->block_list);

   NEXUS_Platform_GetClientConfiguration(&clientConfig);
   nx_ashmem_global->gfx_heap = NULL;
   for (i = 0; i < NEXUS_MAX_HEAPS; i++) {
      NEXUS_Heap_GetStatus(clientConfig.heap[i], &memStatus);
      if ((memStatus.memoryType & (NEXUS_MEMORY_TYPE_MANAGED|NEXUS_MEMORY_TYPE_ONDEMAND_MAPPED|NEXUS_MEMORY_TYPE_DYNAMIC)) &&
          (memStatus.heapType & NX_ASHMEM_NEXUS_DCMA_MARKER)) {
         nx_ashmem_global->gfx_heap = clientConfig.heap[i];
         pr_info("selected d-cma heap %d (%p)\n", i, nx_ashmem_global->gfx_heap);
         break;
      }
   }
   nx_ashmem_global->gfx_alt_heap[0] = NULL;
   nx_ashmem_global->gfx_alt_heap[1] = NULL;
   if (nx_ashmem_global->gfx_heap == NULL) {
      /* no dynamic heap, fallback to default platform graphics one. */
      pr_info("fallback on failure to associate d-cma heap!\n");
      nx_ashmem_global->gfx_heap = NEXUS_Platform_GetFramebufferHeap(NEXUS_OFFSCREEN_SURFACE);
   } else {
      nx_ashmem_global->gfx_heap_dyn = 1;
   }
   gfx_heap_grow_size = memparse(gfx_heap_grow, NULL);
   gfx_heap_grow_factor = (GFX_UHD_FB / gfx_heap_grow_size) + 1;
   nx_ashmem_global->gfx_fb_heap = NEXUS_Platform_GetFramebufferHeap(0);

#if defined(V3D_VARIANT_v3d)
   err = BERR_NOT_INITIALIZED;
#else
   nx_ashmem_global->memIf.BinPoolBlock_Alloc  = nx_ashmem_memif_alloc;
   nx_ashmem_global->memIf.BinPoolBlock_Free   = nx_ashmem_memif_free;
   nx_ashmem_global->memIf.BinPoolBlock_Lock   = nx_ashmem_memif_lock;
   nx_ashmem_global->memIf.BinPoolBlock_Unlock = nx_ashmem_memif_unlock;
   err = BVC5_RegisterAlternateMemInterface(THIS_MODULE, &nx_ashmem_global->memIf);
#endif

   pr_info("nx::%p::%s:%p::gs:%lu:(x%d)::mm:%s::if:%s::fb:%p\n",
           nx_ashmem_global, nx_ashmem_global->gfx_heap_dyn ? "dynamic" : "static",
           nx_ashmem_global->gfx_heap, gfx_heap_grow_size, gfx_heap_grow_factor,
           block_mgmt_mode, (err==BERR_SUCCESS)?"reg":"non", nx_ashmem_global->gfx_fb_heap);

   return 0;

error1:
   kmem_cache_destroy(nx_ashmem_area_cachep);
error0:
   return ret;
}

static void __exit nx_ashmem_module_exit(void)
{
   int ret;

#if !defined(V3D_VARIANT_v3d)
   BVC5_UnregisterAlternateMemInterface(THIS_MODULE);
#endif

   ret = misc_deregister(&nx_ashmem_misc);
   if (unlikely(ret))
      pr_err("failed to unregister misc device!\n");

   kmem_cache_destroy(nx_ashmem_area_cachep);

   mutex_destroy(&(nx_ashmem_global->block_lock));
   mutex_destroy(&(nx_ashmem_global->video_lock));
   kfree(nx_ashmem_global);

   pr_info("nx_ashmem: unloaded\n");
}

module_init(nx_ashmem_module_init);
module_exit(nx_ashmem_module_exit);

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Broadcom Limited");
MODULE_DESCRIPTION("ashmem nexus integration");
