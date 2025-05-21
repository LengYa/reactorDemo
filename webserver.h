#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include <iostream>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>
#include <ctime>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>


#define BUFFER_SIZE 1024
#define CONNECTIONS_SIZE 1024

int EventRegister(int fd, int event);
int Accept_cb(int fd);
int Recv_cb(int fd);
int Send_cb(int fd);
int SetEvent(int fd, int event, int flag);
int Init_Server(unsigned short port);

typedef int (*RCALLBACK)(int fd);

typedef struct ConnectSt{
    char rbuffer[BUFFER_SIZE];
    char wbuffer[BUFFER_SIZE];

    int fd;
    int rlen;
    int wlen;
    int status;       //0--发送头，1--发送body，2--关闭连接

    RCALLBACK send_callback;        //对应EPOLLOUT事件
    union {
        RCALLBACK recv_callback;    //对应EPOLLIN事件
        RCALLBACK accept_callback;  //对应EPOLLIN事件
    }recv_or_accept;  
}Conne;

int Http_Request(Conne *c);
int Http_Response(Conne *c);

#endif
