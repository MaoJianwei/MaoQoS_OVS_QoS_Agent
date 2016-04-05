#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /*struct sockaddr_in*/
#include <arpa/inet.h>  /*inet_pton */
#include <error.h>

#include <stdio.h>
#include <unistd.h>

#include "maoqos.h"




int maoLog(char * logStr, char * log_master_name) {

	if (NULL == logStr)
		return 2;

	if (NULL == log_master_name)
		log_master_name = MaoLogDefaultMaster;

	char * logName1 = "/home/mao/maoOVSlog/";
	char * logName2 = ".txt";

	char * logName = calloc(1, strlen(logName1) + strlen(logName2) + strlen(log_master_name) + 1); // +1 for \0
	strcat(logName, logName1);
	strcat(logName, log_master_name);
	strcat(logName, logName2);



	FILE * log = fopen(logName, "a+");

	if (NULL == log){
		free(logName);
		return 1;
	}

	char * logTemp = (char *) calloc(1,strlen(logStr) + strlen(log_master_name) + 2 + 1 + 1); // +2 for ", "    +1 for \n    +1 for \0
	sprintf(logTemp, "%s, %s\n", log_master_name, logStr);

	fputs(logTemp, log);

	fclose(log);

	free(logTemp);
	free(logName);
	return 0;
}



/**
 * Attention! need free outside!
 */
char * mao_bridge_get_controller_IP(struct bridge * br)
{
    if(NULL == br->ofproto){
    	maoLog("Warning, ofproto is NULL", br->name);
    	return NULL;
    }

    if(NULL == br->ofproto->connmgr){
    	maoLog("Warning, connmgr is NULL", br->name);
    	return NULL;
    }

    if(NULL == &(br->ofproto->connmgr->controllers)){
    	maoLog("Warning, controllers is NULL", br->name);
    	return NULL;
    }

    struct ofconn *ofconn;
    char * controllerIP = (char*) calloc(1,16);
    HMAP_FOR_EACH (ofconn, hmap_node, &(br->ofproto->connmgr->controllers)) {

    	char * target = ofconn->rconn->target;
    	char * head = strchr(target,':');
    	char * tail = strrchr(target,':');
    	memcpy(controllerIP, head+1, tail-head-1); //outside test OK!

    	return controllerIP;
    }
    maoLog("Warning: Exit each, return NULL.", br->name);
    free(controllerIP);
    return NULL;
}





void maoInitQueue(struct cmdQueueHead * queue) {

	TAILQ_INIT(queue);
}
/**
 * cmd will be copied away, That release cmd is the invoker's duty
 */
void maoEnQueue(struct cmdQueueHead *cmdQueue, char * cmd) {
	struct cmdQueueEntry * data = (struct cmdQueueEntry *) calloc(1,
			sizeof(struct cmdQueueEntry));
	data->cmdQos = (char*) calloc(1, strlen(cmd) + 1); // +1 include \0
	strcpy(data->cmdQos, cmd);
	TAILQ_INSERT_TAIL(cmdQueue, data, tailq_entry);
}
/**
 * That release cmd char* is the invoker's duty; The rest is my duty
 * @Return : char*, will be NULL if Queue is empty
 */
char * maoDeQueue(struct cmdQueueHead *cmdQueue) {

	if (TAILQ_EMPTY(cmdQueue)) {
		return NULL;
	}

	struct cmdQueueEntry* DATA = TAILQ_FIRST(cmdQueue);

	char * cmd = DATA->cmdQos;

	TAILQ_REMOVE(cmdQueue, DATA, tailq_entry);
	free(DATA);
	DATA = NULL;

	return cmd;
}
void maoClearQueue(struct cmdQueueHead *cmdQueue) {

	while (NULL != maoDeQueue(cmdQueue)) {
		;
	}
}
/**
 * @return: empty-1; notEmpty-0
 */
int maoIsEmptyQueue(struct cmdQueueHead *cmdQueue) {

	if (TAILQ_EMPTY(cmdQueue)) {
		return 1;
	}

	return 0;
}

/**
 * @maoQosModule : output valid and initialized
 */
struct maoQos * maoQosNewOne(struct bridge* br) {

	struct maoQos * maoQosModule = (struct maoQos *) calloc(1,
			sizeof(struct maoQos));
	if (NULL == maoQosModule) {
		return NULL;
	}

	maoQosModule->cmdQueue = (struct cmdQueueHead *) calloc(1,
			sizeof(struct cmdQueueHead));
	maoInitQueue(maoQosModule->cmdQueue);

	maoQosModule->pMutex = (pthread_mutex_t *) calloc(1,
			sizeof(pthread_mutex_t));
	pthread_mutex_init(maoQosModule->pMutex, NULL);

	maoQosModule->pCond = (pthread_cond_t *) calloc(1, sizeof(pthread_cond_t));
	pthread_cond_init(maoQosModule->pCond, NULL);

	maoQosModule->needShutdown = 0;

	maoQosModule->bashThread = (pthread_t *) calloc(1, sizeof(pthread_t));
	maoQosModule->socketThread = (pthread_t *) calloc(1, sizeof(pthread_t));

	maoQosModule->myBoss = br;

	return maoQosModule;
}
/**
 * That both threads have exited Must be confirmed!
 * @maoQosModule : input valid, output released BUT not NULL !!!
 */
void maoQosFreeOne(struct maoQos * maoQosModule) {

	maoClearQueue(maoQosModule->cmdQueue);
	free(maoQosModule->cmdQueue);

	pthread_mutex_destroy(maoQosModule->pMutex);
	free(maoQosModule->pMutex);

	pthread_cond_destroy(maoQosModule->pCond);
	free(maoQosModule->pCond);

	free(maoQosModule->bashThread);
	free(maoQosModule->socketThread);

	free(maoQosModule);

	maoQosModule->myBoss = NULL;
}
/**
 * start Mao Qos Module
 */
void maoQosRun(struct maoQos * maoQosModule) {
	maoQosModule->needShutdown = 0;

	int ret = 0;

	ret |= pthread_create(maoQosModule->bashThread, NULL, workBash,
			maoQosModule);
	ret |= pthread_create(maoQosModule->socketThread, NULL, workSocket,
			maoQosModule);

	if (0 != ret) {
		//TODO:pthread_kill signal
	}
}
/**
 * just ask module to shutdown.
 */
void maoQosShutdown(struct maoQos * maoQosModule) {


	maoQosModule->needShutdown = 1;

	pthread_cond_broadcast(maoQosModule->pCond);// attention! we need this!

	pthread_join(*(maoQosModule->bashThread), NULL);
	pthread_join(*(maoQosModule->socketThread), NULL);

	//TODO:need pthread_join?
	//if use pthread_join, invoke maoQosFreeOne inside or outside
}

/**
 * Bash Thread main function
 */
void * workBash(void * module) {

	//TODO: Initialize
	char * cmdHead = "echo 123 | sudo -S ";

	struct maoQos * maoQosModule = (struct maoQos *) module;

	maoLog("INFO, Bash working...", maoQosModule->myBoss->name);

	while (0 == maoQosModule->needShutdown) {

		maoLog("INFO, lock mutex", maoQosModule->myBoss->name);


		pthread_mutex_lock(maoQosModule->pMutex);

		while (1 == maoIsEmptyQueue(maoQosModule->cmdQueue) && 0 == maoQosModule->needShutdown) {
			pthread_cond_wait(maoQosModule->pCond, maoQosModule->pMutex);
		}

		if (1 == maoQosModule->needShutdown) {
			pthread_mutex_unlock(maoQosModule->pMutex);
			break;
		}

		if (1 == maoIsEmptyQueue(maoQosModule->cmdQueue)) {
			pthread_mutex_unlock(maoQosModule->pMutex);
			continue;
		}


		char * cmdPayload = maoDeQueue(maoQosModule->cmdQueue);

		pthread_mutex_unlock(maoQosModule->pMutex);

		if (NULL == cmdPayload) {
			maoLog("Warning, cmd string is NULL", maoQosModule->myBoss->name);
			continue;
		}



		maoLog("INFO, have got cmd string", maoQosModule->myBoss->name);
		maoLog(cmdPayload, maoQosModule->myBoss->name);

		char * cmdBash = (char*) calloc(1, strlen(cmdHead) + strlen(cmdPayload) + 1); // should not use sizeof
		strcat(cmdBash, cmdHead);
		strcat(cmdBash, cmdPayload);



		maoLog("INFO, run bash", maoQosModule->myBoss->name);
		maoLog(cmdBash, maoQosModule->myBoss->name);

		FILE * bashPipe = popen(cmdBash, "r");
		pclose(bashPipe);

		maoLog("INFO, run bash finish", maoQosModule->myBoss->name);



		free(cmdBash);
		free(cmdPayload);
	}

	//TODO: any Destroy

	maoLog("INFO, Bash Thread is turning off", maoQosModule->myBoss->name);

	return NULL;
	//pthread_exit(NULL);
}

/**
 * Socket Thread main function
 */
void * workSocket(void * module) {

	//TODO: Initialize

	struct maoQos * maoQosModule = (struct maoQos *) module;


	maoLog("INFO, Socket working...", maoQosModule->myBoss->name);


	while (0 == maoQosModule->needShutdown) {

		maoLog("INFO, Get controller's IP...", maoQosModule->myBoss->name);

		char * controllerIP = NULL;
		char brMac[18] = { 0 };
		int connectSocket = -1;
		while (0 == maoQosModule->needShutdown) {

			controllerIP = mao_bridge_get_controller_IP(maoQosModule->myBoss);

			if (NULL == controllerIP) {
				maoLog("INFO, Wait controller's IP...", maoQosModule->myBoss->name);
				sleep(1);
				continue;
			}
			maoLog("INFO, Get controller's IP OK!", maoQosModule->myBoss->name);



			if (0 != maoQosModule->needShutdown) {
				break;
			}

			sprintf(brMac, "%02X:%02X:%02X:%02X:%02X:%02X",
					maoQosModule->myBoss->default_ea.ea[0],
					maoQosModule->myBoss->default_ea.ea[1],
					maoQosModule->myBoss->default_ea.ea[2],
					maoQosModule->myBoss->default_ea.ea[3],
					maoQosModule->myBoss->default_ea.ea[4],
					maoQosModule->myBoss->default_ea.ea[5]);

			maoLog("INFO, Get My MAC OK!", maoQosModule->myBoss->name);
			maoLog(brMac, maoQosModule->myBoss->name);


			struct sockaddr_in controllerSockaddr = { 0 };
			controllerSockaddr.sin_family = AF_INET;
			controllerSockaddr.sin_port = htons(5511);

			int ptonRet = inet_pton(AF_INET, controllerIP, &(controllerSockaddr.sin_addr));
			if (1 != ptonRet) {

				free(controllerIP);
				controllerIP = NULL;

				maoLog("Error, convert IP fail", maoQosModule->myBoss->name);

				sleep(1);
				continue;
			}
			maoLog("INFO, convert IP OK!", maoQosModule->myBoss->name);


			connectSocket = socket(AF_INET, SOCK_STREAM, 0);
			int connectRet = connect(connectSocket,(struct sockaddr*) (&controllerSockaddr),sizeof(struct sockaddr_in));
			if(-1 == connectRet){

				free(controllerIP);
				controllerIP = NULL;

				close(connectSocket);
				connectSocket = -1;

				maoLog("Error, connect fail", maoQosModule->myBoss->name);

				sleep(1);
				continue;
			}
			maoLog("INFO, connect controller OK!", maoQosModule->myBoss->name);



			struct timeval recvTimeout = { 0, 500 };
			int setRet = setsockopt(connectSocket, SOL_SOCKET, SO_RCVTIMEO,(char*) &recvTimeout, sizeof(struct timeval));
			if(-1 == setRet){

				free(controllerIP);
				controllerIP = NULL;

				close(connectSocket);
				connectSocket = -1;

				maoLog("Error, set timeout fail", maoQosModule->myBoss->name);

				sleep(1);
				continue;
			}
			maoLog("INFO, set timeout OK!", maoQosModule->myBoss->name);


			int dataLen = strlen(brMac);
			int dataSentLen = 0;
			int sendRet = 0;
			while(-1 != sendRet && dataSentLen != dataLen){
				sendRet = send(connectSocket, brMac+dataSentLen, dataLen - dataSentLen, 0);
				dataSentLen += sendRet;
			}
			if(-1 == sendRet){

				free(controllerIP);
				controllerIP = NULL;

				close(connectSocket);
				connectSocket = -1;

				maoLog("Error, send MAC fail", maoQosModule->myBoss->name);

				sleep(1);
				continue;
			}
			maoLog("INFO, send MAC OK!", maoQosModule->myBoss->name);

			break;
		}
		maoLog("INFO, quit Connect and Hello", maoQosModule->myBoss->name);


		//char buf[1024] = { 0 };

		// Mao Qos Protocol v0.9
		while (0 == maoQosModule->needShutdown) {

			maoLog("INFO, wait for protocol length", maoQosModule->myBoss->name);

			// ------ recv lengthBehind
			char * lenBuf = (char*) calloc(1, 4);
			int maoRecvRet = maoSocketRecv(&connectSocket, lenBuf, 4, &(maoQosModule->needShutdown));// TODO: Further deal, robustness
			if (-1 == maoRecvRet) {

				free(lenBuf);
				lenBuf = NULL;

				maoLog("Warning, recv protocol length fail", maoQosModule->myBoss->name);

				break;// different from above, because the above will re-init the socket
			}

			maoLog("INFO, get protocol length", maoQosModule->myBoss->name);

			int cmdLength = 0;
			memcpy(&cmdLength, lenBuf, 4);
			free(lenBuf);
			lenBuf = NULL;

			//wait length cost much time, so we should check needShutdown here
			if (0 != maoQosModule->needShutdown) {
				break;
			}


			maoLog("INFO, wait protocol content", maoQosModule->myBoss->name);

			// ------ recv cmd string
			char * protocolBuf = (char*) calloc(1, cmdLength+1);
			maoRecvRet = maoSocketRecv(&connectSocket, protocolBuf, cmdLength, &(maoQosModule->needShutdown)); // TODO: Further deal, robustness
			if (-1 == maoRecvRet) {

				free(protocolBuf);
				protocolBuf = NULL;

				maoLog("Warning, recv cmd string fail", maoQosModule->myBoss->name);

				break; // different from above, because the above will re-init the socket
			}

			maoLog("INFO, start parse cmd", maoQosModule->myBoss->name);

			char * cmd = maoParseCmdProtocol(protocolBuf);

			free(protocolBuf);
			protocolBuf = NULL;


			maoLog("INFO, cmd ready for enqueue", maoQosModule->myBoss->name);
			maoLog(cmd, maoQosModule->myBoss->name);


			pthread_mutex_lock(maoQosModule->pMutex);

			maoEnQueue(maoQosModule->cmdQueue, cmd);

			pthread_mutex_unlock(maoQosModule->pMutex);

			pthread_cond_signal(maoQosModule->pCond);

			maoLog("INFO, cmd is enqueued, signal is sent", maoQosModule->myBoss->name);

		}
		maoLog("INFO, quit Protocol Communication", maoQosModule->myBoss->name);

		close(connectSocket);
		connectSocket = -1;

		free(controllerIP);
		controllerIP = NULL;
	}

	maoLog("INFO, Socket Thread is turning off", maoQosModule->myBoss->name);

	return NULL;
	//pthread_exit(NULL);
}

/**
 * Mao Qos Protocol v0.9
 * protocolBuf should be free outside
 * @return: calloc inside, should be free outside
 */
char * maoParseCmdProtocol(char * protocolBuf){

	char * cmd = (char*) calloc(1, strlen(protocolBuf)+1);

	strcpy(cmd, protocolBuf);

	return cmd;
}

/**
 * outside should check needShutdown immediately
 * @ return should not be used in normal way, except -1
 */
int maoSocketRecv(int * connectSocket, char * buf, int wantBytes, int * needShutdown){

	int dataRecvLen = 0;
	int recvRet = 0;
	while ( ((-1 != recvRet)||((-1 == recvRet)&&(11 == errno))) && dataRecvLen != wantBytes && 0 == *needShutdown ) {

		recvRet = recv(*connectSocket, buf+recvRet, wantBytes-recvRet, 0);
		if(recvRet > 0){
			dataRecvLen += recvRet;
		}
	}

	if (-1 == recvRet) {
		return -1;
	}else{
		if(dataRecvLen != wantBytes){
			maoLog("Error, maoSocketRecv length not equal", NULL);
		}
		return dataRecvLen;
	}
}


