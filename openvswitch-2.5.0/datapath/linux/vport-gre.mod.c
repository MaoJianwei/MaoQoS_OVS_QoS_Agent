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
	{ 0x151b2616, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x8a896982, __VMLINUX_SYMBOL_STR(rpl_gre_fb_xmit) },
	{ 0xb1a6b251, __VMLINUX_SYMBOL_STR(ovs_netdev_tunnel_destroy) },
	{ 0xac272b2c, __VMLINUX_SYMBOL_STR(ovs_vport_ops_unregister) },
	{ 0x90fc745b, __VMLINUX_SYMBOL_STR(__ovs_vport_ops_register) },
	{ 0x50a589b0, __VMLINUX_SYMBOL_STR(ovs_vport_free) },
	{ 0xf22fd64b, __VMLINUX_SYMBOL_STR(ovs_netdev_link) },
	{ 0x6e720ff2, __VMLINUX_SYMBOL_STR(rtnl_unlock) },
	{ 0x342e38cd, __VMLINUX_SYMBOL_STR(dev_change_flags) },
	{ 0x76d94505, __VMLINUX_SYMBOL_STR(rpl_gretap_fb_dev_create) },
	{ 0xc7a4fbed, __VMLINUX_SYMBOL_STR(rtnl_lock) },
	{ 0x6dd4e49b, __VMLINUX_SYMBOL_STR(ovs_vport_alloc) },
	{ 0x9e3ca78b, __VMLINUX_SYMBOL_STR(ovs_tunnel_get_egress_info) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=openvswitch";


MODULE_INFO(srcversion, "6826B0BFB602CB64F5501DF");
