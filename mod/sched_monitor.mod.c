#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xe0e92029, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x255b6fac, __VMLINUX_SYMBOL_STR(misc_deregister) },
	{ 0x32bc0fcf, __VMLINUX_SYMBOL_STR(preempt_notifier_dec) },
	{ 0x402cbbf, __VMLINUX_SYMBOL_STR(preempt_notifier_inc) },
	{ 0x238c90c9, __VMLINUX_SYMBOL_STR(misc_register) },
	{ 0xf4fa543b, __VMLINUX_SYMBOL_STR(arm_copy_to_user) },
	{ 0x53ddfbd0, __VMLINUX_SYMBOL_STR(preempt_notifier_register) },
	{ 0xb7e914e3, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x120120ae, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x51d559d1, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_irqrestore) },
	{ 0x94eea794, __VMLINUX_SYMBOL_STR(getnstimeofday64) },
	{ 0x598542b2, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irqsave) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x91211512, __VMLINUX_SYMBOL_STR(preempt_notifier_unregister) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "3A8873FE275CF2165CC73B4");
