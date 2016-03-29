
#ifndef _MaoQos_H
#define _MaoQos_H

#include <malloc.h>
#include <sys/queue.h>

#ifndef NULL
#define NULL ((void *)0)
#endif


int maoLog(char * logStr);


void maoInitQueue(struct cmdQueueHead * queue);
void maoEnQueue(struct cmdQueueHead *cmdQueue, char * cmd);
char * maoDeQueue(struct cmdQueueHead *cmdQueue);
void maoClearQueue(struct cmdQueueHead *cmdQueue);
int maoIsEmptyQueue(struct cmdQueueHead *cmdQueue);


struct maoQos * maoQosNewOne();
void maoQosFreeOne(struct maoQos * maoQosModule);
void maoQosRun(struct maoQos * maoQosModule);
void maoQosShutdown(struct maoQos * maoQosModule);


void * workBash(void * args);
void * workSocket(void * args);



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
};




#endif // _MaoQos_H
