#ifndef _LINUX_NX_ASHMEM_H
#define _LINUX_NX_ASHMEM_H

#include <linux/ioctl.h>

#define __NX_ASHMEMIOC                    0x7A

#define NX_ASHMEM_MARKER_VIDEO_DECODER    0xCAFE0001
#define NX_ASHMEM_MARKER_VIDEO_DEC_SECURE 0xCAFE0002
#define NX_ASHMEM_MARKER_VIDEO_ENCODER    0xCAFE0003

#define NX_ASHMEM_REFCNT_ADD              0xBAAD0001
#define NX_ASHMEM_REFCNT_REM              0xBAAD0002

#define NX_ASHMEM_HEAP_FB                 0xABCD0001
#define NX_ASHMEM_HEAP_DCMA               0xABCD0002

#define NX_ASHMEM_NEXUS_DCMA_MARKER       0x100000
#define NX_ASHMEM_NEXUS_SML_MARKER        0x200000

struct nx_ashmem_alloc {
   __u32 size;
   __u32 align;
   __u32 movable;
   __u32 marker;
   __u32 heap;
};

struct nx_ashmem_getmem {
   __u64 hdl;
};

struct nx_ashmem_mgr_cfg {
   __u32 alt_use_threshold;
   __u32 alt_use_max[2];
};

struct nx_ashmem_ext_refcnt {
   __u64 hdl;
   __u32 cnt;
};

struct nx_ashmem_refcnt {
   __u64 hdl;
   __u32 cnt;
   __u32 rel;
};

struct nx_ashmem_pidalloc {
   __u32 pid;
   __u32 alloc;
};

#define NX_ASHMEM_SET_SIZE                _IOW(__NX_ASHMEMIOC, 1, struct nx_ashmem_alloc)
#define NX_ASHMEM_GET_SIZE                _IOW(__NX_ASHMEMIOC, 2, struct nx_ashmem_alloc)
#define NX_ASHMEM_GETMEM                  _IOW(__NX_ASHMEMIOC, 3, struct nx_ashmem_getmem)
#define NX_ASHMEM_DUMP_ALL                _IO(__NX_ASHMEMIOC,  4)
#define NX_ASHMEM_MGR_CFG                 _IOW(__NX_ASHMEMIOC, 5, struct nx_ashmem_mgr_cfg)
#define NX_ASHMEM_CHK_SEC                 _IOW(__NX_ASHMEMIOC, 6, __u32)
#define NX_ASHMEM_CHK_PLAY                _IOW(__NX_ASHMEMIOC, 7, __u32)
#define NX_ASHMEM_EXT_REFCNT              _IOW(__NX_ASHMEMIOC, 8, struct nx_ashmem_ext_refcnt)
#define NX_ASHMEM_GET_BLK                 _IOW(__NX_ASHMEMIOC, 9, struct nx_ashmem_getmem)
#define NX_ASHMEM_REFCNT                  _IOW(__NX_ASHMEMIOC, 10, struct nx_ashmem_refcnt)
#define NX_ASHMEM_ALLOC_PER_PID           _IOW(__NX_ASHMEMIOC, 11, struct nx_ashmem_pidalloc)

#endif /* _LINUX_NX_ASHMEM_H */
