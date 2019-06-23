#include "vec_router.h"
#define DEBUG

//cost Table and route table?
// no need cost table, because never used ??
// need neighbor nexthop to be transmitted. I won't update dis vec that route from me 

/* Section
        variables used for dis_vec routing
 */

//cost table
int costTable[256];
int takeASleep = 0;

// if it is neighbor
int IsNeighbor[256];

//costs to each neighbor
int globalCost[256];
/* 
    Linkstate:
        actually linkstate is just the network order of the globalCost
        constantly send dis_vec to neighbors, its size is 4 bytes * 256= 1024 bytes
*/
int32_t linkState[256];

// next hops for each IP
int nexthops[256];

/* 
    END Section 
*/

//myself's ID
int globalMyID = 0;

//last time you heard from each node.
struct timeval globalLastHeartbeat[256];
struct timeval lastChangedTime[256];
struct timeval lastChangeTime;

//UDP socket, to be bound to 10.1.1.globalMyID, port 7777
int globalSocketUDP;

//addresses sending to 10.1.1.0 - 255, port 7777
struct sockaddr_in globalNodeAddrs[256];


char logbuffer[BUFFERSIZE];

int stopBeforeStable = 0;

FILE* logfile;

/* Function
    debug print
*/
void debug_print(const char *msg){
    #ifdef DEBUG
        printf("%s\n",msg);
    #endif
}

void print_cost_table(){
    sprintf(logbuffer,"COSTTABLE at %d:",globalMyID);
    debug_print(logbuffer);
    for(int i=0;i<256;i++){
        if(globalCost[i] != -1){
            sprintf(logbuffer,"TO %d is %d,next %d",i,globalCost[i],nexthops[i]);
            debug_print(logbuffer);
        }
    }
    debug_print("");

}


/*
    Function:   min of 2 int
*/
int min2(int a,int b){
    if(a<b) return a;
    return b;
}

/* Fucntion 
    broad to only my neighbor
    buf:content, length:buf size
*/
void broadToNeighbor(const void* buf, int length){
    int i;
    for( i=0 ; i<256 ; i++){
        if(IsNeighbor[i] == 1 && i != globalMyID){
            sendto(globalSocketUDP, buf,length,0,
            (struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[1]));
        }
    }
}


/* Fucntion 
    broadcast to all IP
    Using this to send linkstate as heartbeat
    buf:linkstate, length:buf size
*/
void hackyBroadcast(const void* buf, int length,int target)
{
	// int i;
	// for(i=0;i<256;i++)
		if(target != globalMyID) //(although with a real broadcast you would also get the packet yourself)
        {
            sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*)&globalNodeAddrs[target], sizeof(globalNodeAddrs[target]));
        }
			
}
/*
    Function
    If nexthop is i, then the link don't tell i
*/

void broadCastGrained(){
    for(int target = 0; target<256;target++){
        if(target == globalMyID){
            continue;
        }
        int32_t tmpLinkState[256];
        for(int i=0;i<256;i++){
            if(nexthops[i] == target){
                tmpLinkState[i] = htonl(-1);
            }
            else{
                tmpLinkState[i] = linkState[i];
            }
        }
        hackyBroadcast(tmpLinkState,BUFFERSIZE,target);
    }
}


/*
    Function:
        Check if a node is alive,
        if last heartbeats more than 2RTT(something larger than 600), deeded as dead
*/

void checkAlive(){
    struct timeval timeInterval;
	gettimeofday(&timeInterval,0);
	int i;
	char logLine[80];
	for(i=0;i<256;i++){
        // check if a link between myself and neighbor is down
		if(IsNeighbor[i]==1){
			double elapsedTime = (timeInterval.tv_sec - globalLastHeartbeat[i].tv_sec)  * 1000;
			if(elapsedTime > 1500){ // if larger than 2 sleeping time, then something wrong, it is dead
                sprintf(logbuffer," Node %d find node %d died after %lf!",globalMyID, i,elapsedTime);
                debug_print(logbuffer);
                costTable[i] = -1;
                for(int j=0;j<256;j++){
                    if(j == globalMyID ) continue;
                    //All nodes route through i fails, change link state j
                    if(nexthops[j] == i){
                        nexthops[j] = j;
                        globalCost[j] = costTable[j];
                        linkState[j] = htonl(globalCost[j]);
                    }
                }
				print_cost_table();
				IsNeighbor[i]=0;
                //tell neighbors this link dead
                broadCastGrained();
				// hackyBroadcast(linkState, BUFFERSIZE);
			}
		}
	}
}

void my_sleep(int time){
    struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = time * 1000 * 1000; 
    nanosleep(&sleepFor, 0);
        
}

/*  
    Function:
        Annoce to neighbor the dis_vec every 300ms
        Also check neighbor's aliveness
*/
void* announceToNeighbors(void* params)
{
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 100 * 1000 * 1000; //100 ms
    
	while(1)
	{
        if(takeASleep){
            my_sleep(200);
        }
        checkAlive();
        broadCastGrained();
		// hackyBroadcast(linkState, BUFFERSIZE);
		nanosleep(&sleepFor, 0);
        
	}
}

/*  
    Function:
       checkAlive every 300 ms
*/
void* checkAliveNeighbors(void* params)
{
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 300 * 1000 * 1000; //300 ms
	while(1)
	{
	    checkAlive();
		nanosleep(&sleepFor, 0);
	}
}


void listenForNeighbors()
{

	char fromAddr[100];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;

    //recvd info
	unsigned char recvBuf[BUFFERSIZE];

    //line for logfile
    char tmpLine[BUFFERSIZE];

	while(1)
	{
		theirAddrLen = sizeof(theirAddr);
        memset(&recvBuf[0],0,sizeof(recvBuf));
	    int numbytes;
		if ((numbytes = recvfrom(globalSocketUDP, recvBuf, BUFFERSIZE , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1)
		{
			perror("recvfrom:");
			exit(1);
		}
		recvBuf[numbytes] = '\0';
        
		inet_ntop(AF_INET, &theirAddr.sin_addr, fromAddr, 100);
		short int heardFrom = -1;

		
        int somethingChanged = 0;

        // if messageh is our format, receive it or forward it 
        if(!strncmp(recvBuf,"dnes",4)){
            int length;
            // because message <= 100 bits, and there are 6bits header, so it will not exceed 106 bits, plus one '\0'  
			char message[BUFFERSIZE];	
			char payLoad[BUFFERSIZE];	

            short int networkDestID;
           
            memcpy(&networkDestID, &recvBuf[4],2);
            // network to host destID
			short int hostDestID = ntohs(networkDestID);

            // sprintf(logbuffer,"message dst host id is %hu\n", hostDestID);
            // debug_print(logbuffer);
            strcpy(payLoad, &recvBuf[6]);

            if(globalMyID == hostDestID){
                sprintf(logbuffer,"%d receiving message",globalMyID);
                debug_print(logbuffer);

                sprintf(tmpLine, "receive packet message %s\n", payLoad);
				fwrite(tmpLine, 1, strlen(tmpLine), logfile);
            }
            else{
                if(globalCost[hostDestID] == -1) {		
                    debug_print("cost -1 unreachable\n");

                    sprintf(tmpLine, "unreachable dest %d\n", hostDestID);
                    fwrite(tmpLine, 1, strlen(tmpLine), logfile);
                }
                else{
                    // check if forward successfully
                    int payLoadSize = strlen(payLoad);
                    // sprintf(logbuffer,"payload size is %d",payLoadSize);
                    debug_print(logbuffer);

                    //TODO: is something wrong here?
                    if(sendto(globalSocketUDP, recvBuf, payLoadSize+6, 0,
                    (struct sockaddr*)&globalNodeAddrs[nexthops[hostDestID]], sizeof(globalNodeAddrs[nexthops[hostDestID]])) < 0){
                        perror("sendto()");
                        debug_print("send fail unreachable\n");

                        sprintf(tmpLine, "unreachable dest %d\n", hostDestID);
                        fwrite(tmpLine, 1, strlen(tmpLine), logfile);
                    }
                    else{
                       
                        sprintf(tmpLine, "forward packet dest %d nexthop %d message %s\n", hostDestID, nexthops[hostDestID],payLoad);
                        fwrite(tmpLine, 1, strlen(tmpLine), logfile);

                        sprintf(logbuffer, "%d forward packet dest %d nexthop %d,cost is %d, message %s\n",globalMyID, hostDestID, nexthops[hostDestID],globalCost[hostDestID], payLoad);
                         debug_print(tmpLine);
                    }
                }

            }
           
        }

        // packet from manager, reassemble packet to our format
        else if(!strncmp(recvBuf, "send", 4))
		{
            int length;
            // because message <= 100 bits, and there are 6bits header, so it will not exceed 106 bits, plus one '\0'  
			char message[BUFFERSIZE];	
			char payLoad[BUFFERSIZE];	

            short int networkDestID;
           
            memcpy(&networkDestID, &recvBuf[4],2);
            // network to host destID
			short int hostDestID = ntohs(networkDestID);
            

            //print payload to logfile
            strcpy(payLoad, &recvBuf[6]);
            //copy payload into message
            // write unreachable if cost == -1
			if(globalCost[hostDestID] == -1) {
                debug_print("cost -1 unreachable");							
				sprintf(tmpLine, "unreachable dest %d\n", hostDestID);
				fwrite(tmpLine, 1, strlen(tmpLine), logfile);
            }
			else{		
                // fowward, write send first to the newly constructed packet
				strcpy(message,"dnes");

                // write network order destID back to message
                // begins at 4 because send takes 4 bits
				memcpy(&message[4], &networkDestID, sizeof(short int));	
                strcpy(&message[6],payLoad);  
                int payLoadSize = strlen(payLoad);
                // sprintf(logbuffer,"payload size is %d",payLoadSize);
               

                // check if forward successfully
				if(sendto(globalSocketUDP, message, payLoadSize+6, 0,
				  (struct sockaddr*)&globalNodeAddrs[nexthops[hostDestID]], sizeof(globalNodeAddrs[nexthops[hostDestID]])) < 0){
					perror("sendto()");

                    debug_print("send fail unreachable");	
					sprintf(tmpLine, "unreachable dest %d\n", hostDestID);
					fwrite(tmpLine, 1, strlen(tmpLine), logfile);

                    debug_print("sending translated message failed");
				}
				// if reachable
				else{
                    sprintf(tmpLine, "sending packet dest %d nexthop %d message %s\n", hostDestID, nexthops[hostDestID], payLoad);
                    fwrite(tmpLine, 1, strlen(tmpLine), logfile);

                    sprintf(logbuffer,"At %d,sending packet dest %d nexthop %d cost %d ",globalMyID,hostDestID,nexthops[hostDestID],globalCost[hostDestID]);
                    debug_print(logbuffer);

				}
            }
		}

        // sent from manager, cost, destID and newCost. Need to send dst to change cost
		else if(!strncmp(recvBuf, "cost", 4))
		{

            //parse dstID and newCost
			short int networkNeighborID;
            memcpy(&networkNeighborID,&recvBuf[4],2);
            short int hostNeighborId = ntohs(networkNeighborID);
            int networkCost;
			memcpy(&networkCost,&recvBuf[6],4);
            int hostCost = ntohl(networkCost);
            costTable[hostNeighborId] = hostCost;
            
            sprintf(logbuffer,"at %d Manager says cost to %hu is  %d\n", globalMyID,hostNeighborId,hostCost);
            debug_print(logbuffer);

			globalCost[hostNeighborId] = hostCost;
            // update dis_vec. using network order cost
            linkState[hostNeighborId] = networkCost;
            somethingChanged = 1;
            //prepare for new cost sending
            char message[BUFFERSIZE];	
         
            //send new cost to neighbor
            // strcpy(message,"ncos");
            // int myNetId = htons(globalMyID);
            // memcpy(&recvBuf[4],&myNetId,2);
            //  memcpy(&recvBuf[6],&networkCost,4);
            // if(sendto(globalSocketUDP, message, 10, 0,
            //     (struct sockaddr*)&globalNodeAddrs[hostNeighborId], sizeof(globalNodeAddrs[hostNeighborId])) < 0){
            //     perror("sendto()");
            // }
            // else{
            //     sprintf(logbuffer,"at %d sends new cost %d to neighbor   %d\n", globalMyID,hostCost,hostNeighborId);
            //     debug_print(logbuffer);
            // }
		}

        // //receive new cost
        // else if(!strncmp(recvBuf, "ncos", 4))
		// {

        //     //parse dstID and newCost
		// 	short int networkNeighborID;
        //     memcpy(&networkNeighborID,&recvBuf[4],2);
        //     short int hostNeighborId = ntohs(networkNeighborID);
        //     int networkCost;
		// 	memcpy(&networkCost,&recvBuf[6],4);
        //     int hostCost = ntohl(networkCost);
        //     costTable[hostNeighborId] = hostCost;

        //     char message[BUFFERSIZE];	
        //     sprintf(logbuffer,"at %hd receiveis from %hd  says cost to  %d\n", globalMyID,hostNeighborId,hostCost);
        //     debug_print(logbuffer);
		// 	globalCost[hostNeighborId] = hostCost;
        //     // update dis_vec. using network order cost
        //     linkState[hostNeighborId] = networkCost;
        //     somethingChanged = 1;
        
		// }


        // Received dis_vec from neighbor
        else  
		{
			heardFrom = atoi(strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1);
            // sprintf(logbuffer, "dis_vec from %d",heardFrom);
            // debug_print(logbuffer);

			int i;
			IsNeighbor[heardFrom] = 1;

			//update cost from dis_vec, its now directly connected
           
			if(costTable[heardFrom] == -1){
                sprintf(logbuffer,"node %d find new direclty linked neighbor %d",globalMyID, heardFrom);
                debug_print(logbuffer);
                costTable[heardFrom] = 1;
				globalCost[heardFrom] = 1;		
				nexthops[heardFrom] = heardFrom;
				linkState[heardFrom] = htonl(globalCost[heardFrom]);
                somethingChanged = 1;
            }

            // will the cost be something apart from 1??    
        
            // int costToMe;
            // memcpy(&costToMe,&recvBuf[4*globalMyID],4);
            // costToMe = ntohl(costToMe);
           
            // //set up initial cost, if has neihbor, then default next hop is him
            // if(costToMe != -1 && globalCost[heardFrom] == -1){
            //     sprintf(logbuffer,"At %d optimize neighbor cost according to thei said: %d to me is %d",globalMyID,heardFrom,costToMe);
            //     debug_print(logbuffer);
            //     globalCost[heardFrom] = costToMe;
            //     nexthops[heardFrom] = heardFrom;
            //     linkState[heardFrom] = htonl(globalCost[heardFrom]);
            //     somethingChanged = 1;
            // }
                
            // read each dis_vec send from heardFrom,
            // linkstate is actually an edge so its their cost for pos i
            for(i=0;i<256;i++){
              

                //!! this is always 0, Not use this to route
                if(i == heardFrom || i == globalMyID ){
                    continue;
                }

                int theirCostFori;
                //extract cur dis_vec from recvbuf
                memcpy(&theirCostFori,&recvBuf[4*i],4);
                theirCostFori = ntohl(theirCostFori);
                
                if(theirCostFori != -1){
                    sprintf(logbuffer,"%d receivve from %d to %d is %d",globalMyID,heardFrom,i,theirCostFori);
                    debug_print(logbuffer);
                }
                

            
               
                // Find some unreachable nodes can now be reached with heardFrom 
                if( globalCost[i] == -1 and theirCostFori != -1){
                    struct timeval timeInterval;
                    gettimeofday(&timeInterval,0);
                    double elapsedTime = (timeInterval.tv_sec - lastChangedTime[i].tv_sec)  * 1000;
                    sprintf(logbuffer,"After %lf after last change\n",elapsedTime);
                    debug_print(logbuffer);
                    if(elapsedTime < 0.001){
                        my_sleep(5);
                    }
                    else{
                        sprintf(logbuffer,"At %d,%d said it to %d is %d",globalMyID,heardFrom,i,theirCostFori);
                        debug_print(logbuffer);
                        
                        globalCost[i] = globalCost[heardFrom] + theirCostFori;
                        /*
                            If next hop fo heardFrom is HeardFrom, then to previous unreachable nodes shall go from heardFrome
                            Otherwise go from next hops to heardFrom
                        */
                        if(nexthops[heardFrom] == heardFrom) nexthops[i] = heardFrom;
                        else nexthops[i] = nexthops[heardFrom];
                        somethingChanged = 1;
                    }
                    gettimeofday(&lastChangedTime[i],0);
                }
                
                /*
                    Route through this node
                    if there is a shorter path with the help of this nodes
                    this should not be else if, because it may be optimized 

                */
                if( theirCostFori != -1 and globalCost[i] != -1 and globalCost[i] > theirCostFori + globalCost[heardFrom]){
                   
                    sprintf(logbuffer,"At %d,%d said it to %d is %d",globalMyID,heardFrom,i,theirCostFori);
                    debug_print(logbuffer);
                    
                    globalCost[i] = theirCostFori + globalCost[heardFrom];
                    nexthops[i] = heardFrom;
                    somethingChanged = 1;
                }

                /*
                        For tie condition
                            LS: when choosing which node to move to the finished set next
                            if there is a tie, choose the lowest node ID
                            this situation only needs to update nexthops, dis_vec is no need to update
                */
                if( theirCostFori != -1 and globalCost[i] != -1 and globalCost[i] == theirCostFori + globalCost[heardFrom] ){
                    nexthops[i] = min2(nexthops[i],heardFrom);
                }
                /*
                    This is for a link goes down, his neighbor report this.
                    Or link distance increase we need to find better path.
                    Because our nexthop goes through this node, then we shall cut off this link and refind new path
                */
                //  if(nexthops[i] == heardFrom &&  (theirCostFori+globalCost[heardFrom] > globalCost[i] || globalCost[i] == -1)){
                if( nexthops[i] == heardFrom && globalCost[i] != -1 &&  (theirCostFori == -1 ||  theirCostFori+globalCost[heardFrom] > globalCost[i] ) ){
                    sprintf(logbuffer,"At %d, %d said its distance to %d changed to %d",globalMyID,heardFrom,i,theirCostFori);
                    debug_print(logbuffer);
                    globalCost[i] = costTable[i];
                    nexthops[i] = i;
                    somethingChanged = 1;
                }
                
                //update dis_vec
                linkState[i] = htonl(globalCost[i]);
            }
			//record that we heard from heardFrom just now.
            gettimeofday(&globalLastHeartbeat[heardFrom], 0);
		}

        if(somethingChanged){

            struct timeval timeInterval;
            gettimeofday(&timeInterval,0);
            double elapsedTime = (timeInterval.tv_sec - lastChangeTime.tv_sec)  * 1000;
            if(elapsedTime < 1){
                takeASleep = 1;
                my_sleep(50);
                takeASleep = 0;
            }
            sprintf(logbuffer,"elasepd %lf after last change\n",elapsedTime);
            debug_print(logbuffer);
            broadCastGrained();
            // hackyBroadcast(linkState,BUFFERSIZE);
            
            print_cost_table();
            gettimeofday(&lastChangeTime, 0);
        }
        // debug_print("write flush");
        //write to log file immediately
        fflush(logfile);
	}
	//(should never reach here)
	close(globalSocketUDP);
}






 
int main(int argc, char** argv)
{
	if(argc != 4)
	{
		fprintf(stderr, "Usage: ./vec_router mynodeid initialcostsfile logfile\n");
		exit(-1);
	}
	
	/* 
        Section:
            Set up node's ID, record its time and setup sockaddr_in
    */

	globalMyID = atoi(argv[1]);
	int i;
	for(i=0;i<256;i++)
	{
		gettimeofday(&globalLastHeartbeat[i], 0);
		gettimeofday(&lastChangedTime[i], 0);
		char tempaddr[100];
		sprintf(tempaddr, "10.1.1.%d", i);
		memset(&globalNodeAddrs[i], 0, sizeof(globalNodeAddrs[i]));
		globalNodeAddrs[i].sin_family = AF_INET;
		globalNodeAddrs[i].sin_port = htons(7777);
		inet_pton(AF_INET, tempaddr, &globalNodeAddrs[i].sin_addr);
	}
	/* End Section */

    gettimeofday(&lastChangeTime, 0);
	
	/*
        Section: 
            init cost and next hop,
            parse cost file
            setup log file

            If you don't find an entry for a node, default to cost 1. 
            We will only use positive costs – never 0 or negative.
    */
   //init 
    for(int i = 0; i < 256; i++)
    {
        IsNeighbor[i] = 0;
        globalCost[i] = -1;
        nexthops[i] = i;
        costTable[i] = -1;
    }
    //set self cost to 0
    // globalCost[globalMyID] = 0;

    FILE* initialcostsfile = fopen(argv[2], "r");
    if(initialcostsfile == NULL){
        fprintf(stderr,"cannot open initial cost file\n");
        exit(-1);
    }

    while (!feof(initialcostsfile)){  
        int nodeId, tmpCost;
		fscanf(initialcostsfile, "%d", &nodeId);   
		fscanf(initialcostsfile, "%d", &tmpCost);  
		globalCost[nodeId] = tmpCost;
        costTable[nodeId] = tmpCost; 
        //!! attention here node is not neighbor
            // IsNeighbor[nodeId] = 1

	}
    fclose(initialcostsfile);
    /* 
        Section:
            Set Up dis_vec used for gossiping
    */
   for(i=0;i<256;i++){
		int networkglobalCost = htonl(globalCost[i]);
		linkState[i] = networkglobalCost;
      
	}
    /* END Section */
    if((logfile = fopen(argv[3], "w+")) == NULL){
        fprintf(stderr,"not a valid logfile");
        exit(-1);
    }

    logfile = fopen(argv[3], "w+");
    /*
        End Section for initcostfile paring
    */
    
    /* 
        Section:
            socket() and bind() for mysocket
            // creating socket for myself and bind myself addr
    */
	if((globalSocketUDP=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}
	char myAddr[100];
	struct sockaddr_in bindAddr;
	sprintf(myAddr, "10.1.1.%d", globalMyID);	
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;

	bindAddr.sin_port = htons(7777);
	inet_pton(AF_INET, myAddr, &bindAddr.sin_addr);
	if(bind(globalSocketUDP, (struct sockaddr*)&bindAddr, sizeof(struct sockaddr_in)) < 0)
	{
		perror("bind");
		close(globalSocketUDP);
		exit(1);
	}
	/* 
        End Section for socket() and bind() for mysocket 
    */
            
	
    

	/* Section:
        Start Threads and begin listenning:
    */
	pthread_t announcerThread;
	pthread_create(&announcerThread, 0, announceToNeighbors, (void*)0);
	
    // pthread_t checkAliveThread;
	// pthread_create(&checkAliveThread, 0, checkAliveNeighbors, (void*)0);
	
	
	
	listenForNeighbors();
    /* 
        End Section 
    */
}