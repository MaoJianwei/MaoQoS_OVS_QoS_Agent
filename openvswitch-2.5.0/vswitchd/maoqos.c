#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /*struct sockaddr_in*/
#include <arpa/inet.h>  /*inet_pton */
#include <error.h>

#include <stdio.h>
#include <unistd.h>

#include "maoqos.h"




int maoLog(enum MaoLogLevel level, char * logStr, char * log_master_name) {

	if( level<1 || level>4 ){
		maoLog(WARNING, "log level out of scope", NULL);
		return 3;
	}

	if (NULL == logStr){
		maoLog(WARNING, "logStr is NULL", NULL);
		return 2;
	}

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
		maoLog(WARNING, "log is NULL", NULL);
		free(logName);
		return 1;
	}

	char * levelStr = NULL;

	switch(level){
	case INFO:
		levelStr = "INFO";
		break;
	case DEBUG:
		levelStr = "DEBUG";
		break;
	case WARNING:
		levelStr = "WARNING";
		break;
	case ERROR:
		levelStr = "ERROR";
		break;
	default:
		levelStr = "UNKNOWN";
	}

	char * logTemp = (char *) calloc(1,strlen(logStr) + strlen(log_master_name) + strlen(levelStr) + 2 +2 + 1 + 1); // +2 for ", "   +2 for ": "    +1 for \n    +1 for \0
	sprintf(logTemp, "%s, %s: %s\n", log_master_name, levelStr, logStr);

	fputs(logTemp, log);

	fclose(log);

	free(logTemp);
	free(logName);
	return 0;
}

/**
 * Attention! need free outside!
 * result > 0 for success.
 */
char * mao_bridge_get_port_name(struct bridge * br)
{

    struct port *port, *next;
    char * bridge_ports = (char*) calloc(1,1024);
    int pointer = 0;


    int result = 0;
    HMAP_FOR_EACH_SAFE (port, next, hmap_node, &br->ports) {

    	result++;

    	strcpy(bridge_ports + pointer, port->name);
        pointer += strlen(port->name);
        *( bridge_ports + (pointer)++ ) = ',';
    }

    if( result == 0 ){
    	maoLog(WARNING, "port is not ready", br->name);
    	free(bridge_ports);
    	bridge_ports = NULL;
    	return NULL;
    }

    *(bridge_ports + pointer - 1) = '\n';
    *(bridge_ports + pointer) = 0;

    return bridge_ports;
}


/**
 * Attention! need free outside!
 */
char * mao_bridge_get_controller_IP(struct bridge * br)
{
    if(NULL == br->ofproto){
    	maoLog(WARNING, "ofproto is NULL", br->name);
    	return NULL;
    }

    if(NULL == br->ofproto->connmgr){
    	maoLog(WARNING, "connmgr is NULL", br->name);
    	return NULL;
    }

    if(NULL == &(br->ofproto->connmgr->controllers)){
    	maoLog(WARNING, "controllers is NULL", br->name);
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
    maoLog(WARNING, "Exit each, return NULL.", br->name);
    free(controllerIP);
    return NULL;
}

/**
 * Attention! need free outside!
 */
char * mao_bridge_get_Bridge_ID(struct bridge * br){

	if(NULL == br || NULL == br->ofproto)
		return NULL;


	const int dpidStrLen = 16;

	uint64_t dpidInt = br->ofproto->datapath_id;

	char * dpid = (char*)calloc(17,1);

	for(int i = 0; i<dpidStrLen ; i++){
		sprintf(dpid+i,"%01X", (unsigned int)((dpidInt >> 4*(dpidStrLen -1 -i)) & 0xF));
	}

	return dpid;
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

}

/**
 * Bash Thread main function
 */
void * workBash(void * module) {


	//TODO: Initialize
	char * cmdHead = "echo 123 | sudo -S ";

	struct maoQos * maoQosModule = (struct maoQos *) module;

	maoLog(INFO, "Bash working...", maoQosModule->myBoss->name);



	while (0 == maoQosModule->needShutdown) {

		maoLog(INFO, "lock mutex", maoQosModule->myBoss->name);


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
			maoLog(WARNING, "cmd string is NULL", maoQosModule->myBoss->name);
			continue;
		}



		maoLog(INFO, "have got cmd string", maoQosModule->myBoss->name);
		maoLog(INFO, cmdPayload, maoQosModule->myBoss->name);

		char * cmdBash = (char*) calloc(1, strlen(cmdHead) + strlen(cmdPayload) + 1); // should not use sizeof
		strcat(cmdBash, cmdHead);
		strcat(cmdBash, cmdPayload);



		maoLog(INFO, "run bash", maoQosModule->myBoss->name);
		maoLog(INFO, cmdBash, maoQosModule->myBoss->name);

		FILE * bashPipe = popen(cmdBash, "r");
		pclose(bashPipe);

		maoLog(INFO, "run bash finish", maoQosModule->myBoss->name);



		free(cmdBash);
		free(cmdPayload);
	}

	//TODO: any Destroy

	maoLog(INFO, "Bash Thread is turning off", maoQosModule->myBoss->name);

	return NULL;
	//pthread_exit(NULL);
}

/**
 * Socket Thread main function
 */
void * workSocket(void * module) {

	//TODO: Initialize

	struct maoQos * maoQosModule = (struct maoQos *) module;


	maoLog(INFO, "Socket working...", maoQosModule->myBoss->name);


	while (0 == maoQosModule->needShutdown) {

		maoLog(INFO, "Get controller's IP...", maoQosModule->myBoss->name);

		char * controllerIP = NULL;
		char * dpid = NULL;
		int connectSocket = -1;
		while (0 == maoQosModule->needShutdown) {

			controllerIP = mao_bridge_get_controller_IP(maoQosModule->myBoss);

			if (NULL == controllerIP) {
				maoLog(INFO, "Wait controller's IP...", maoQosModule->myBoss->name);
				sleep(1);
				continue;
			}
			maoLog(INFO, "Get controller's IP OK!", maoQosModule->myBoss->name);

			if (0 != maoQosModule->needShutdown) {
				break;
			}


			char * ports = NULL;
			while(0 == maoQosModule->needShutdown){

				ports = mao_bridge_get_port_name(maoQosModule->myBoss);
				if(ports == NULL){
					sleep(1);
					continue;
				}
				break;
			}
			maoLog(INFO, "Get ports OK!", maoQosModule->myBoss->name);
			maoLog(INFO, ports, maoQosModule->myBoss->name);

			if (0 != maoQosModule->needShutdown) {
				break;
			}


			dpid = mao_bridge_get_Bridge_ID(maoQosModule->myBoss);
			if (NULL == dpid) {

				free(controllerIP);
				controllerIP = NULL;

				free(ports);
				ports = NULL;

				maoLog(WARNING, "Wait my DPID...", maoQosModule->myBoss->name);

				sleep(1);
				continue;
			}
			maoLog(INFO, "Get My DPID OK!", maoQosModule->myBoss->name);
			maoLog(INFO, dpid, maoQosModule->myBoss->name);

//			sprintf(brMac, "%02X:%02X:%02X:%02X:%02X:%02X",
//					maoQosModule->myBoss->default_ea.ea[0],
//					maoQosModule->myBoss->default_ea.ea[1],
//					maoQosModule->myBoss->default_ea.ea[2],
//					maoQosModule->myBoss->default_ea.ea[3],
//					maoQosModule->myBoss->default_ea.ea[4],
//					maoQosModule->myBoss->default_ea.ea[5]);
//
//			maoLog("INFO, Get My MAC OK!", maoQosModule->myBoss->name);
//			maoLog(brMac, maoQosModule->myBoss->name);


			struct sockaddr_in controllerSockaddr = { 0 };
			controllerSockaddr.sin_family = AF_INET;
			controllerSockaddr.sin_port = htons(5511);

			int ptonRet = inet_pton(AF_INET, controllerIP, &(controllerSockaddr.sin_addr));
			if (1 != ptonRet) {

				free(controllerIP);
				controllerIP = NULL;

				free(dpid);
				dpid = NULL;

				free(ports);
				ports = NULL;

				maoLog(ERROR, "convert IP fail", maoQosModule->myBoss->name);

				sleep(1);
				continue;
			}
			maoLog(INFO, "convert IP OK!", maoQosModule->myBoss->name);


			connectSocket = socket(AF_INET, SOCK_STREAM, 0);
			int connectRet = connect(connectSocket,(struct sockaddr*) (&controllerSockaddr),sizeof(struct sockaddr_in));
			if(-1 == connectRet){

				free(controllerIP);
				controllerIP = NULL;

				free(dpid);
				dpid = NULL;

				free(ports);
				ports = NULL;

				close(connectSocket);
				connectSocket = -1;

				maoLog(WARNING, "connect fail", maoQosModule->myBoss->name);

				sleep(1);
				continue;
			}
			maoLog(INFO, "connect controller OK!", maoQosModule->myBoss->name);



			struct timeval recvTimeout = { 0, 500 };
			int setRet = setsockopt(connectSocket, SOL_SOCKET, SO_RCVTIMEO,(char*) &recvTimeout, sizeof(struct timeval));
			if(-1 == setRet){

				free(controllerIP);
				controllerIP = NULL;

				free(dpid);
				dpid = NULL;

				free(ports);
				ports = NULL;

				close(connectSocket);
				connectSocket = -1;

				maoLog(ERROR, "set timeout fail", maoQosModule->myBoss->name);

				sleep(1);
				continue;
			}
			maoLog(INFO, "set timeout OK!", maoQosModule->myBoss->name);


			int dataLen = strlen(dpid);
			int dataSentLen = 0;
			int sendRet = 0;
			while(-1 != sendRet && dataSentLen != dataLen){
				sendRet = send(connectSocket, dpid+dataSentLen, dataLen - dataSentLen, 0);
				dataSentLen += sendRet;
			}
			if(-1 == sendRet){

				free(controllerIP);
				controllerIP = NULL;

				free(dpid);
				dpid = NULL;

				free(ports);
				ports = NULL;

				close(connectSocket);
				connectSocket = -1;

				maoLog(ERROR, "send DPID fail", maoQosModule->myBoss->name);

				sleep(1);
				continue;
			}
			maoLog(INFO, "send DPID OK!", maoQosModule->myBoss->name);

			dataLen = strlen(ports);
			dataSentLen = 0;
			sendRet = 0;
			maoLog(INFO, "send port ...", maoQosModule->myBoss->name);
			while (-1 != sendRet && dataSentLen != dataLen) {
				sendRet = send(connectSocket, ports + dataSentLen,
						dataLen - dataSentLen, 0);
				dataSentLen += sendRet;
			}
			if (-1 == sendRet) {

				free(controllerIP);
				controllerIP = NULL;

				free(dpid);
				dpid = NULL;

				free(ports);
				ports = NULL;

				close(connectSocket);
				connectSocket = -1;

				maoLog(ERROR, "send port fail", maoQosModule->myBoss->name);

				sleep(1);
				continue;
			}
			maoLog(INFO, "send port OK!", maoQosModule->myBoss->name);

			break;
		}
		maoLog(INFO, "quit Connect and Hello", maoQosModule->myBoss->name);




		// Mao Qos Protocol v0.9
		while (0 == maoQosModule->needShutdown) {

			maoLog(INFO, "wait for protocol length", maoQosModule->myBoss->name);

			// ------ recv lengthBehind
			char * lenBuf = (char*) calloc(1, 4);
			int maoRecvRet = maoSocketRecv(&connectSocket, lenBuf, 4, &(maoQosModule->needShutdown), maoQosModule->myBoss->name);// TODO: Further deal, robustness
			if (-1 == maoRecvRet) {

				maoLog(WARNING, "recv protocol length fail", maoQosModule->myBoss->name);

				free(lenBuf);
				lenBuf = NULL;

				break; // different from above, because the above will re-init the socket
			} else if (0 == maoRecvRet) {

				maoLog(WARNING, "Controller Peer shutdown!", maoQosModule->myBoss->name);

				free(lenBuf);
				lenBuf = NULL;

				break;
			}

			maoLog(INFO, "parse protocol length...", maoQosModule->myBoss->name);


			int cmdLength = 0;
			memcpy(&cmdLength, lenBuf, 4);
			free(lenBuf);
			lenBuf = NULL;

			char recvrecv[28] = {0};
			sprintf(recvrecv, "INFO, protocol length: %d", cmdLength);
			maoLog(INFO, recvrecv, maoQosModule->myBoss->name);


			//wait length cost much time, so we should check needShutdown here
			if (0 != maoQosModule->needShutdown) {
				break;
			}


			maoLog(INFO, "wait protocol content", maoQosModule->myBoss->name);

			// ------ recv cmd string
			char * protocolBuf = (char*) calloc(1, cmdLength+1);
			maoRecvRet = maoSocketRecv(&connectSocket, protocolBuf, cmdLength, &(maoQosModule->needShutdown), maoQosModule->myBoss->name); // TODO: Further deal, robustness
			if (-1 == maoRecvRet) {

				free(protocolBuf);
				protocolBuf = NULL;

				maoLog(WARNING, "recv cmd string fail", maoQosModule->myBoss->name);

				break; // different from above, because the above will re-init the socket
			}
			maoLog(INFO, protocolBuf, maoQosModule->myBoss->name);

			maoLog(INFO, "start parse cmd", maoQosModule->myBoss->name);


			char * cmd = maoParseCmdProtocol(protocolBuf);

			free(protocolBuf);
			protocolBuf = NULL;


			maoLog(INFO, "cmd ready for enqueue...", maoQosModule->myBoss->name);
			maoLog(INFO, cmd, maoQosModule->myBoss->name);


			pthread_mutex_lock(maoQosModule->pMutex);

			maoEnQueue(maoQosModule->cmdQueue, cmd);

			pthread_mutex_unlock(maoQosModule->pMutex);

			pthread_cond_signal(maoQosModule->pCond);


			maoLog(INFO, "cmd is enqueued, signal is sent", maoQosModule->myBoss->name);

		}
		maoLog(INFO, "quit Protocol Communication", maoQosModule->myBoss->name);

		close(connectSocket);
		connectSocket = -1;

		free(dpid);
		dpid = NULL;

		free(controllerIP);
		controllerIP = NULL;
	}

	maoLog(INFO, "Socket Thread is turning off", maoQosModule->myBoss->name);

	return NULL;
	//pthread_exit(NULL);
}

/**
 * interface for extension in the future
 *
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
int maoSocketRecv(int * connectSocket, char * buf, int wantBytes, int * needShutdown, char * log_master_name){

	int dataRecvLen = 0;
	int recvRet = 0;

	char recvrecv[100] = {0};
	sprintf(recvrecv, "start recv %d, %d, %d", dataRecvLen, recvRet, wantBytes);
	maoLog(DEBUG, recvrecv, log_master_name);

	while (((-1 != recvRet) || ((-1 == recvRet) && (11 == errno)))
			&& dataRecvLen != wantBytes && 0 == *needShutdown) {

		recvRet = recv(*connectSocket, buf + dataRecvLen, wantBytes - dataRecvLen, 0);

		if (recvRet > 0) {
			dataRecvLen += recvRet;

			char recvrecv11[10] = { 0 };
			sprintf(recvrecv11, "%d", recvRet);
			maoLog(DEBUG, recvrecv11, log_master_name);

			maoLog(DEBUG, buf, log_master_name);

		} else if (0 == recvRet) {
			// peer shutdown
			break;
		} else if (11 == errno) {
			//maoLog(DEBUG, "recv wait again...", log_master_name);
			usleep(100000);
		}else{
			maoLog(ERROR,"no deal case of RECV ret!", log_master_name);
		}
	}

	memset(recvrecv, 0, 100);
	sprintf(recvrecv, "end recv %d, %d, %d", dataRecvLen, recvRet, wantBytes);
	maoLog(DEBUG, recvrecv, log_master_name);

	if (-1 == recvRet) {
		return -1;
	}else if(0 == recvRet){
		return 0;
	}else{
		if(dataRecvLen != wantBytes && 0 == *needShutdown){
			maoLog(ERROR, "maoSocketRecv length not equal", log_master_name);
		}else if(1 == *needShutdown){
			maoLog(WARNING, "maoSocketRecv while shutdown", log_master_name);
		}
		return dataRecvLen;
	}
}


