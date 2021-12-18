/*
** Copyright (C) 2018 Wang Yaofu
** All rights reserved.
**
**Author:Wang Yaofu voipman@qq.com
**Description: The source file of class EndPointMgr.
*/
#include "EndPointMgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

EndPointMgr::EndPointMgr() {

}

EndPointMgr::~EndPointMgr() {
    mEndpointList.clear();
}

bool EndPointMgr::addEndPoint(int port) {
    SEndPointInfo clientInfo;
    bzero(&clientInfo.sockaddr, sizeof(sockaddr));
    bzero(&clientInfo.clientaddr, sizeof(sockaddr));
    clientInfo.failTimes = 0;
    clientInfo.fd = 0;
    clientInfo.port = port;
    clientInfo.sockaddr.sin_family = AF_INET;
    clientInfo.sockaddr.sin_port = htons(port);
    clientInfo.sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientInfo.fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (bind(clientInfo.fd, (struct sockaddr *) &(clientInfo.sockaddr), sizeof(clientInfo.sockaddr)) == -1) {
        perror("bind error");
        exit(1);
    }
        sockaddr_in from;socklen_t   len;
		len = sizeof(struct sockaddr);
    char mesg[30] = {0};
    int n=recvfrom(clientInfo.fd, mesg, socklen_t(30), 0, (struct sockaddr *)&from,&len);
    printf("recv %d bytes news \n",n);
    printf("IP:%s\n", (char *)inet_ntoa(from.sin_addr));
	printf("Port:%d\n", ntohs(from.sin_port)); 

    clientInfo.clientaddr.sin_addr.s_addr=inet_addr(inet_ntoa(from.sin_addr));
    clientInfo.clientaddr.sin_port=from.sin_port;
		
    mEndpointList.push_back(clientInfo);

    return true;
}

int EndPointMgr::sendData(unsigned char *send_buf, size_t len_sendbuf) {
    auto itor = mEndpointList.begin();
    int ret = 0;
    while(itor != mEndpointList.end()) {
        printf("IP:%s\n", (char *)inet_ntoa(itor->clientaddr.sin_addr));
	    printf("Port:%d\n", ntohs(itor->clientaddr.sin_port)); 
        if ((ret = sendto(itor->fd, send_buf, len_sendbuf, 0, (struct sockaddr *)&itor->clientaddr, sizeof(itor->clientaddr))) < 0) {
            itor->failTimes++;
            printf("itor->failTimes is %d \n",itor->failTimes);
            printf("error: %s\n", strerror(errno));
		    //err_sys("sendto error");
        }
        ++itor;
    }
    return ret;
}