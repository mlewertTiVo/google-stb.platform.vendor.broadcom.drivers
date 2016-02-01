#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
#include <linux/kconfig.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
#include <generated/autoconf.h>
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37))
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif

#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = "nx_ashmem",
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
 .unlocked_ioctl = nx_ashmem_ioctl,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=nexus_sa";
