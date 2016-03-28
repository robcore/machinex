#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

MODULE_INFO(intree, "Y");

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x68ef8030, "module_layout" },
	{ 0x6af20eb2, "kernel_kobj" },
	{ 0x734119ca, "kobject_put" },
	{ 0x5252f81b, "sysfs_create_group" },
	{ 0x7967b42b, "kobject_create_and_add" },
	{ 0xfe7c4287, "nr_cpu_ids" },
	{ 0xd3e6f60d, "cpu_possible_mask" },
	{ 0xf087137d, "__dynamic_pr_debug" },
	{ 0x8664f62e, "cpufreq_update_policy" },
	{ 0x27e1a049, "printk" },
	{ 0xb9fe0637, "msm_cpufreq_set_freq_limits" },
	{ 0xd3f57a2, "_find_next_bit_le" },
	{ 0x20c55ae0, "sscanf" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x91715312, "sprintf" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

