
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

void maoInitQueue(struct cmdQueueHead * queue){

	TAILQ_INIT(queue);
}
/**
 * cmd will be copied away, That release cmd is the invoker's duty
 */
void maoEnQueue(struct cmdQueueHead *cmdQueue, char * cmd){
	struct cmdQueueEntry * data = (struct cmdQueueEntry * )calloc(1, sizeof(struct cmdQueueEntry));
	data->cmdQos = (char*)calloc(1,strlen(cmd)+1); // +1 include \0
	strcpy(data->cmdQos,cmd);
	TAILQ_INSERT_TAIL(cmdQueue, data, tailq_entry);
}
/**
 * That release cmd char* is the invoker's duty; The rest is my duty
 * @Return : char*, will be NULL if Queue is empty
 */
char * maoDeQueue(struct cmdQueueHead *cmdQueue){

	if(TAILQ_EMPTY(cmdQueue)){
		return NULL;
	}

	struct cmdQueueEntry* DATA = TAILQ_FIRST(cmdQueue);

	char * cmd = DATA->cmdQos;

	TAILQ_REMOVE(cmdQueue, DATA, tailq_entry);
	free(DATA);
	DATA = NULL;

	return cmd;
}
void maoClearQueue(struct cmdQueueHead *cmdQueue){

	while(NULL != maoDeQueue(cmdQueue)){
		;
	}
}




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
/**
 * @maoQosModule : output valid and initialized
 */
struct maoQos * maoQosNewOne(){

	struct maoQos * maoQosModule = (struct maoQos *)calloc(1, sizeof(struct maoQos));
	if(NULL == maoQosModule){
		return NULL;
	}

	maoQosModule->cmdQueue = (struct cmdQueueHead *)calloc(1,sizeof(struct cmdQueueHead));
	maoInitQueue(maoQosModule->cmdQueue);

	maoQosModule->pMutex = (pthread_mutex_t *)calloc(1,sizeof(pthread_mutex_t));
	pthread_mutex_init(maoQosModule->pMutex,NULL);

	maoQosModule->pCond = (pthread_cond_t *)calloc(1,sizeof(pthread_cond_t));
	pthread_cond_init(maoQosModule->pCond,NULL);

	maoQosModule->needShutdown = 0;

	maoQosModule->bashThread = (pthread_t *)calloc(1,sizeof(pthread_t));
	maoQosModule->socketThread = (pthread_t *)calloc(1,sizeof(pthread_t));

	return maoQosModule;
}
/**
 * That both threads have exited Must be confirmed!
 * @maoQosModule : input valid, output released BUT not NULL !!!
 */
void maoQosFreeOne(struct maoQos * maoQosModule){

	maoClearQueue(maoQosModule->cmdQueue);
	free(maoQosModule->cmdQueue);

	pthread_mutex_destroy(maoQosModule->pMutex);
	free(maoQosModule->pMutex);

	pthread_cond_destroy(maoQosModule->pCond);
	free(maoQosModule->pCond);

	free(maoQosModule->bashThread);
	free(maoQosModule->socketThread);

	free(maoQosModule);
}
/**
 * start Mao Qos Module
 */
void maoQosRun(struct maoQos * maoQosModule){
	maoQosModule->needShutdown = 0;

	int ret = 0;

	ret |= pthread_create(maoQosModule->bashThread,NULL,workBash,maoQosModule);
	ret |= pthread_create(maoQosModule->socketThread,NULL,workSocket,maoQosModule);

	if(0 != ret){
		//TODO:pthread_kill signal
	}
}
/**
 * just ask module to shutdown.
 */
void maoQosShutdown(struct maoQos * maoQosModule){
	maoQosModule->needShutdown = 1;

	//TODO:need pthread_join?
	//if use pthread_join, invoke maoQosFreeOne
}

/**
 * Bash Thread main function
 */
void * workBash(void * module){

	//TODO: Initialize

	struct maoQos * maoQosModule = (struct maoQos * ) module;
}

/**
 * Socket Thread main function
 */
void * workSocket(void * module){

	//TODO: Initialize

	struct maoQos * maoQosModule = (struct maoQos * ) module;


}



#endif // _MaoQos_H
