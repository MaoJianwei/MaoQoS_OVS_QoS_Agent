/* Copyright (c) 2008, 2009, 2010, 2011, 2012, 2014 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VSWITCHD_BRIDGE_H
#define VSWITCHD_BRIDGE_H 1


#include <config.h>

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>

#include "async-append.h"
#include "bfd.h"
#include "bitmap.h"
#include "cfm.h"
#include "connectivity.h"
#include "coverage.h"
#include "daemon.h"
#include "dirs.h"
#include "dpif.h"
#include "dynamic-string.h"
#include "hash.h"
#include "hmap.h"
#include "hmapx.h"
#include "jsonrpc.h"
#include "lacp.h"
#include "list.h"
#include "ovs-lldp.h"
#include "mac-learning.h"
#include "mcast-snooping.h"
#include "meta-flow.h"
#include "netdev.h"
#include "nx-match.h"
#include "ofp-print.h"
#include "ofp-util.h"
#include "ofpbuf.h"
#include "ofproto/bond.h"
#include "ofproto/ofproto.h"
#include "ovs-numa.h"
#include "poll-loop.h"
#include "if-notifier.h"
#include "seq.h"
#include "sha1.h"
#include "shash.h"
#include "smap.h"
#include "socket-util.h"
#include "stream.h"
#include "stream-ssl.h"
#include "sset.h"
#include "system-stats.h"
#include "timeval.h"
#include "util.h"
#include "unixctl.h"
#include "vlandev.h"
#include "lib/vswitch-idl.h"
#include "xenserver.h"
#include "openvswitch/vlog.h"
#include "sflow_api.h"
#include "vlan-bitmap.h"
#include "packets.h"

struct simap;




void bridge_init(const char *remote);
void bridge_exit(void);

void bridge_run(void);
void bridge_wait(void);

void bridge_get_memory_usage(struct simap *usage);


struct iface {
    /* These members are always valid.
     *
     * They are immutable: they never change between iface_create() and
     * iface_destroy(). */
    struct ovs_list port_elem;  /* Element in struct port's "ifaces" list. */
    struct hmap_node name_node; /* In struct bridge's "iface_by_name" hmap. */
    struct hmap_node ofp_port_node; /* In struct bridge's "ifaces" hmap. */
    struct port *port;          /* Containing port. */
    char *name;                 /* Host network device name. */
    struct netdev *netdev;      /* Network device. */
    ofp_port_t ofp_port;        /* OpenFlow port number. */
    uint64_t change_seq;

    /* These members are valid only within bridge_reconfigure(). */
    const char *type;           /* Usually same as cfg->type. */
    const struct ovsrec_interface *cfg;
};

struct mirror {
    struct uuid uuid;           /* UUID of this "mirror" record in database. */
    struct hmap_node hmap_node; /* In struct bridge's "mirrors" hmap. */
    struct bridge *bridge;
    char *name;
    const struct ovsrec_mirror *cfg;
};

struct port {
    struct hmap_node hmap_node; /* Element in struct bridge's "ports" hmap. */
    struct bridge *bridge;
    char *name;

    const struct ovsrec_port *cfg;

    /* An ordinary bridge port has 1 interface.
     * A bridge port for bonding has at least 2 interfaces. */
    struct ovs_list ifaces;    /* List of "struct iface"s. */
};

struct bridge {
    struct hmap_node node;      /* In 'all_bridges'. */
    char *name;                 /* User-specified arbitrary name. */
    char *type;                 /* Datapath type. */
    struct eth_addr ea;         /* Bridge Ethernet Address. */
    struct eth_addr default_ea; /* Default MAC. */
    const struct ovsrec_bridge *cfg;

    /* OpenFlow switch processing. */
    struct ofproto *ofproto;    /* OpenFlow switch. */

    /* Bridge ports. */
    struct hmap ports;          /* "struct port"s indexed by name. */
    struct hmap ifaces;         /* "struct iface"s indexed by ofp_port. */
    struct hmap iface_by_name;  /* "struct iface"s indexed by name. */

    /* Port mirroring. */
    struct hmap mirrors;        /* "struct mirror" indexed by UUID. */

    /* Auto Attach */
    struct hmap mappings;       /* "struct" indexed by UUID */

    /* Used during reconfiguration. */
    struct shash wanted_ports;

    /* Synthetic local port if necessary. */
    struct ovsrec_port synth_local_port;
    struct ovsrec_interface synth_local_iface;
    struct ovsrec_interface *synth_local_ifacep;


    /* Mao QoS agent */
    struct maoQos * MaoQos;

};

struct aa_mapping {
    struct hmap_node hmap_node; /* In struct bridge's "mappings" hmap. */
    struct bridge *bridge;
    uint32_t isid;
    uint16_t vlan;
    char *br_name;
};



#endif /* bridge.h */
