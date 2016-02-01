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

struct nx_ashmem_alloc {
   size_t size;
   size_t align;
   int movable;
   int marker;
   int heap;
};

struct nx_ashmem_mgr_cfg {
   size_t alt_use_threshold;
   int alt_use_max[2];
};

struct nx_ashmem_ext_refcnt {
   size_t hdl;
   int cnt;
};

#define NX_ASHMEM_SET_SIZE                _IOW(__NX_ASHMEMIOC, 1, struct nx_ashmem_alloc)
#define NX_ASHMEM_GET_SIZE                _IO(__NX_ASHMEMIOC,  2)
#define NX_ASHMEM_GETMEM                  _IOW(__NX_ASHMEMIOC, 3, size_t)
#define NX_ASHMEM_DUMP_ALL                _IO(__NX_ASHMEMIOC,  4)
#define NX_ASHMEM_MGR_CFG                 _IOW(__NX_ASHMEMIOC, 5, struct nx_ashmem_mgr_cfg)
#define NX_ASHMEM_CHK_SEC                 _IOW(__NX_ASHMEMIOC, 6, int)
#define NX_ASHMEM_CHK_PLAY                _IOW(__NX_ASHMEMIOC, 7, int)
#define NX_ASHMEM_EXT_REFCNT              _IOW(__NX_ASHMEMIOC, 8, struct nx_ashmem_ext_refcnt)

#endif /* _LINUX_NX_ASHMEM_H */
