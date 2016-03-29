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
	{ 0xca96e3ea, __VMLINUX_SYMBOL_STR(rpl_vxlan_xmit) },
	{ 0xb1a6b251, __VMLINUX_SYMBOL_STR(ovs_netdev_tunnel_destroy) },
	{ 0xac272b2c, __VMLINUX_SYMBOL_STR(ovs_vport_ops_unregister) },
	{ 0x90fc745b, __VMLINUX_SYMBOL_STR(__ovs_vport_ops_register) },
	{ 0xa57eac2f, __VMLINUX_SYMBOL_STR(__skb_get_hash) },
	{ 0x9e3ca78b, __VMLINUX_SYMBOL_STR(ovs_tunnel_get_egress_info) },
	{ 0x4e48de94, __VMLINUX_SYMBOL_STR(inet_get_local_port_range) },
	{ 0x50a589b0, __VMLINUX_SYMBOL_STR(ovs_vport_free) },
	{ 0xf22fd64b, __VMLINUX_SYMBOL_STR(ovs_netdev_link) },
	{ 0x6e720ff2, __VMLINUX_SYMBOL_STR(rtnl_unlock) },
	{ 0x342e38cd, __VMLINUX_SYMBOL_STR(dev_change_flags) },
	{ 0x697d9ab6, __VMLINUX_SYMBOL_STR(rpl_vxlan_dev_create) },
	{ 0xc7a4fbed, __VMLINUX_SYMBOL_STR(rtnl_lock) },
	{ 0x4f391d0e, __VMLINUX_SYMBOL_STR(nla_parse) },
	{ 0x6dd4e49b, __VMLINUX_SYMBOL_STR(ovs_vport_alloc) },
	{ 0xcd279169, __VMLINUX_SYMBOL_STR(nla_find) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x160ef3c9, __VMLINUX_SYMBOL_STR(nla_put) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=openvswitch";


MODULE_INFO(srcversion, "08B6066B9647D5A138207F1");
