
#include <stdio.h>

#include "maoqos.h"





//int maoLog(char * logStr);
int maoLog(char * logStr){

	if (NULL == logStr)
		return 2;

	FILE * log = fopen("/home/mao/maoOVSlog/1.txt","a+");
	if(NULL == log)
		return 1;

	fputs(logStr, log);

	fclose(log);

	return 0;
}







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
/**
 * @return: empty-1; notEmpty-0
 */
int maoIsEmptyQueue(struct cmdQueueHead *cmdQueue){

	if(TAILQ_EMPTY(cmdQueue)){
			return 1;
	}

	return 0;
}







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

	pthread_join(maoQosModule->bashThread);
	pthread_join(maoQosModule->socketThread);

	//TODO:need pthread_join?
	//if use pthread_join, invoke maoQosFreeOne inside or outside
}

/**
 * Bash Thread main function
 */
void * workBash(void * module){

	//TODO: Initialize
	char * cmdHead = "echo 123 | sudo -S ";

	struct maoQos * maoQosModule = (struct maoQos * ) module;

	while(0 == maoQosModule->needShutdown){

        char * logTemp = calloc(1,30);
        strcat(logTemp,"Bash working...");
        maoLog(logTemp);
        free(logTemp);
        logTemp = NULL;

		sleep(1);
	}
	return NULL;

	while(0 == maoQosModule->needShutdown){

		pthread_mutex_lock(maoQosModule->pMutex);

		while(1 == maoIsEmptyQueue(maoQosModule->cmdQueue) && 0 == maoQosModule->needShutdown){
			pthread_cond_wait(maoQosModule->pCond, maoQosModule->pMutex);
		}

		if(1 == maoQosModule->needShutdown){
			pthread_mutex_unlock(maoQosModule->pMutex);
			break;
		}

		if(1 == maoIsEmptyQueue(maoQosModule->cmdQueue)){
			pthread_mutex_unlock(maoQosModule->pMutex);
			continue;
		}


		char * cmdPayload = maoDeQueue(maoQosModule->needShutdown);

		pthread_mutex_unlock(maoQosModule->pMutex);

		if(NULL == cmdPayload){
			continue;
		}



		char * cmdBash = (char*) calloc(1, strlen(cmdHead) + strlen(cmdPayload) + 1 ); // should not use sizeof
		strcat(cmdBash, cmdHead);
		strcat(cmdBash, cmdPayload);

		FILE * bashPipe = popen(cmdBash, "r");
		pclose(bashPipe);

		free(cmdBash);
	}

	//TODO: any Destroy

	return NULL;
	//pthread_exit(NULL);
}

/**
 * Socket Thread main function
 */
void * workSocket(void * module){

	//TODO: Initialize

	struct maoQos * maoQosModule = (struct maoQos * ) module;


	while(0 == maoQosModule->needShutdown){

        char * logTemp = calloc(1,30);
        strcat(logTemp,"Bash working...");
        maoLog(logTemp);
        free(logTemp);
        logTemp = NULL;

		sleep(1);
	}
	return NULL;




	return NULL;
	//pthread_exit(NULL);
}
