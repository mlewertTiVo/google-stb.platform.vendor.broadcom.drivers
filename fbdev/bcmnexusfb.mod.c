#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#include <linux/kconfig.h>
#else
#include <linux/config.h>
#endif
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = __stringify(KBUILD_MODNAME),
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const char __module_depends[]
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
__used
#else
__attribute_used__
#endif
__attribute__((section(".modinfo"))) =
"depends=fb";

