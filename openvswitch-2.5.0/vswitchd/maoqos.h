
#ifndef _MaoQos_H
#define _MaoQos_H

#include <malloc.h>
#include <sys/queue.h>
#include <pthread.h>
#include <string.h>


#include "bridge.h"
#include "ofproto/connmgr.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

char * mao_bridge_get_port_name(struct bridge * br);
char * mao_bridge_get_controller_IP(struct bridge * br);
char * mao_bridge_get_Bridge_ID(struct bridge * br);

/* for c queue */
struct cmdQueueEntry{
	char * cmdQos;
	TAILQ_ENTRY(cmdQueueEntry) tailq_entry;
};
struct cmdQueueHead {
	  struct cmdQueueEntry *tqh_first;
	  struct cmdQueueEntry * *tqh_last;
};



/*
 * Mao QoS Module Life Cycle:
 *
 * maoQosNewOne -> maoQosRun -> maoQosShutdown -> maoQosFreeOne
 */

/* Module main struct*/
struct maoQos{
	struct cmdQueueHead * cmdQueue;
	pthread_mutex_t * pMutex;
	pthread_cond_t * pCond;
	int needShutdown;
	pthread_t * bashThread;
	pthread_t * socketThread;

	struct bridge * myBoss;
};

#define MaoLogDefaultMaster "Mao SYSTEM"
enum MaoLogLevel {
    INFO = 1,/*Mao Log Info Level*/
    DEBUG = 2,/*Mao Log Debug Level*/
	WARNING = 3,/*Mao Log Warning Level*/
	ERROR = 4,/*Mao Log Error Level*/
	TRACE = 5/*Mao Log Trace Level*/
};
int maoLog(enum MaoLogLevel level, char * logStr, char * log_master_name);


void maoInitQueue(struct cmdQueueHead * queue);
void maoEnQueue(struct cmdQueueHead *cmdQueue, char * cmd);
char * maoDeQueue(struct cmdQueueHead *cmdQueue);
void maoClearQueue(struct cmdQueueHead *cmdQueue);
int maoIsEmptyQueue(struct cmdQueueHead *cmdQueue);


struct maoQos * maoQosNewOne(struct bridge* br);
void maoQosFreeOne(struct maoQos * maoQosModule);
void maoQosRun(struct maoQos * maoQosModule);
void maoQosShutdown(struct maoQos * maoQosModule);


void * workBash(void * args);
void * workSocket(void * args);

char * maoParseCmdProtocol(char * protocolBuf);
int maoSocketRecv(int * connectSocket, char * buf, int wantBytes, int * needShutdown, char * log_master_name);

void recordRecvTimestamp(void);
void recordRunStartTimestamp(void);
void recordRunOverTimestamp(void);


/*Mao: below is reference copy*/




/* The connection states have the following meanings:
 *
 *    - S_VOID: No connection information is configured.
 *
 *    - S_BACKOFF: Waiting for a period of time before reconnecting.
 *
 *    - S_CONNECTING: A connection attempt is in progress and has not yet
 *      succeeded or failed.
 *
 *    - S_ACTIVE: A connection has been established and appears to be healthy.
 *
 *    - S_IDLE: A connection has been established but has been idle for some
 *      time.  An echo request has been sent, but no reply has yet been
 *      received.
 *
 *    - S_DISCONNECTED: An unreliable connection has disconnected and cannot be
 *      automatically retried.
 */
#define STATES                                  \
    STATE(VOID, 1 << 0)                         \
    STATE(BACKOFF, 1 << 1)                      \
    STATE(CONNECTING, 1 << 2)                   \
    STATE(ACTIVE, 1 << 3)                       \
    STATE(IDLE, 1 << 4)                         \
    STATE(DISCONNECTED, 1 << 5)
enum state {
#define STATE(NAME, VALUE) S_##NAME = VALUE,
    STATES
#undef STATE
};




/* A reliable connection to an OpenFlow switch or controller.
 *
 * See the large comment in rconn.h for more information. */
struct rconn {
    struct ovs_mutex mutex;

    enum state state;
    time_t state_entered;

    struct vconn *vconn;
    char *name;                 /* Human-readable descriptive name. */
    char *target;               /* vconn name, passed to vconn_open(). */
    bool reliable;

    struct ovs_list txq;        /* Contains "struct ofpbuf"s. */

    int backoff;
    int max_backoff;
    time_t backoff_deadline;
    time_t last_connected;
    time_t last_disconnected;
    unsigned int seqno;
    int last_error;

    /* In S_ACTIVE and S_IDLE, probably_admitted reports whether we believe
     * that the peer has made a (positive) admission control decision on our
     * connection.  If we have not yet been (probably) admitted, then the
     * connection does not reset the timer used for deciding whether the switch
     * should go into fail-open mode.
     *
     * last_admitted reports the last time we believe such a positive admission
     * control decision was made. */
    bool probably_admitted;
    time_t last_admitted;

    /* These values are simply for statistics reporting, not used directly by
     * anything internal to the rconn (or ofproto for that matter). */
    unsigned int n_attempted_connections, n_successful_connections;
    time_t creation_time;
    unsigned long int total_time_connected;

    /* Throughout this file, "probe" is shorthand for "inactivity probe".  When
     * no activity has been observed from the peer for a while, we send out an
     * echo request as an inactivity probe packet.  We should receive back a
     * response.
     *
     * "Activity" is defined as either receiving an OpenFlow message from the
     * peer or successfully sending a message that had been in 'txq'. */
    int probe_interval;         /* Secs of inactivity before sending probe. */
    time_t last_activity;       /* Last time we saw some activity. */

    uint8_t dscp;

    /* Messages sent or received are copied to the monitor connections. */
#define MAXIMUM_MONITORS 8
    struct vconn *monitors[MAXIMUM_MONITORS];
    size_t n_monitors;

    uint32_t allowed_versions;
};




/* An OpenFlow connection.
 *
 *
 * Thread-safety
 * =============
 *
 * 'ofproto_mutex' must be held whenever an ofconn is created or destroyed or,
 * more or less equivalently, whenever an ofconn is added to or removed from a
 * connmgr.  'ofproto_mutex' doesn't protect the data inside the ofconn, except
 * as specifically noted below. */
struct ofconn {
/* Configuration that persists from one connection to the next. */

    struct ovs_list node;       /* In struct connmgr's "all_conns" list. */
    struct hmap_node hmap_node; /* In struct connmgr's "controllers" map. */

    struct connmgr *connmgr;    /* Connection's manager. */
    struct rconn *rconn;        /* OpenFlow connection. */
    enum ofconn_type type;      /* Type. */
    enum ofproto_band band;     /* In-band or out-of-band? */
    bool enable_async_msgs;     /* Initially enable async messages? */

/* State that should be cleared from one connection to the next. */

    /* OpenFlow state. */
    enum ofp12_controller_role role;           /* Role. */
    enum ofputil_protocol protocol; /* Current protocol variant. */
    enum nx_packet_in_format packet_in_format; /* OFPT_PACKET_IN format. */

    /* OFPT_PACKET_IN related data. */
    struct rconn_packet_counter *packet_in_counter; /* # queued on 'rconn'. */
#define N_SCHEDULERS 2
    struct pinsched *schedulers[N_SCHEDULERS];
    struct pktbuf *pktbuf;         /* OpenFlow packet buffers. */
    int miss_send_len;             /* Bytes to send of buffered packets. */
    uint16_t controller_id;     /* Connection controller ID. */

    /* Number of OpenFlow messages queued on 'rconn' as replies to OpenFlow
     * requests, and the maximum number before we stop reading OpenFlow
     * requests.  */
#define OFCONN_REPLY_MAX 100
    struct rconn_packet_counter *reply_counter;

    /* Asynchronous message configuration in each possible roles.
     *
     * A 1-bit enables sending an asynchronous message for one possible reason
     * that the message might be generated, a 0-bit disables it. */
    uint32_t master_async_config[OAM_N_TYPES]; /* master, other */
    uint32_t slave_async_config[OAM_N_TYPES];  /* slave */

    /* Flow table operation logging. */
    int n_add, n_delete, n_modify; /* Number of unreported ops of each kind. */
    long long int first_op, last_op; /* Range of times for unreported ops. */
    long long int next_op_report;    /* Time to report ops, or LLONG_MAX. */
    long long int op_backoff;        /* Earliest time to report ops again. */

/* Flow monitors (e.g. NXST_FLOW_MONITOR). */

    /* Configuration.  Contains "struct ofmonitor"s. */
    struct hmap monitors OVS_GUARDED_BY(ofproto_mutex);

    /* Flow control.
     *
     * When too many flow monitor notifications back up in the transmit buffer,
     * we pause the transmission of further notifications.  These members track
     * the flow control state.
     *
     * When notifications are flowing, 'monitor_paused' is 0.  When
     * notifications are paused, 'monitor_paused' is the value of
     * 'monitor_seqno' at the point we paused.
     *
     * 'monitor_counter' counts the OpenFlow messages and bytes currently in
     * flight.  This value growing too large triggers pausing. */
    uint64_t monitor_paused OVS_GUARDED_BY(ofproto_mutex);
    struct rconn_packet_counter *monitor_counter OVS_GUARDED_BY(ofproto_mutex);

    /* State of monitors for a single ongoing flow_mod.
     *
     * 'updates' is a list of "struct ofpbuf"s that contain
     * NXST_FLOW_MONITOR_REPLY messages representing the changes made by the
     * current flow_mod.
     *
     * When 'updates' is nonempty, 'sent_abbrev_update' is true if 'updates'
     * contains an update event of type NXFME_ABBREV and false otherwise.. */
    struct ovs_list updates OVS_GUARDED_BY(ofproto_mutex);
    bool sent_abbrev_update OVS_GUARDED_BY(ofproto_mutex);

    /* Active bundles. Contains "struct ofp_bundle"s. */
    struct hmap bundles;
};




/* Connection manager for an OpenFlow switch. */
struct connmgr {
    struct ofproto *ofproto;
    char *name;
    char *local_port_name;

    /* OpenFlow connections. */
    struct hmap controllers;     /* All OFCONN_PRIMARY controllers. */
    struct ovs_list all_conns;   /* All controllers. */
    uint64_t master_election_id; /* monotonically increasing sequence number
                                  * for master election */
    bool master_election_id_defined;

    /* OpenFlow listeners. */
    struct hmap services;       /* Contains "struct ofservice"s. */
    struct pvconn **snoops;
    size_t n_snoops;

    /* Fail open. */
    struct fail_open *fail_open;
    enum ofproto_fail_mode fail_mode;

    /* In-band control. */
    struct in_band *in_band;
    struct sockaddr_in *extra_in_band_remotes;
    size_t n_extra_remotes;
    int in_band_queue;
};



#endif // _MaoQos_H
