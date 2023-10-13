#include<cstdio>
#include<iostream>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<sys/errno.h>
#include<cassert>
#include<fcntl.h>
#include<cstring>
#include<map>
#include<signal.h>
#include"Message.h"

#ifndef _EPOLL_MINE_H_
#define _EPOLL_MINE_H_

struct USER_DS
{
    int ID;
    int fd;
};


class EPOLL_MINE
{

public:
    EPOLL_MINE() = default;
    EPOLL_MINE(int epoll_size , int Max_number_event_ , int sockfd_ , int name) 
    : Max_number_event(Max_number_event_),sockfd(sockfd_),ID(name)
    {epollfd = epoll_create(epoll_size); events = new epoll_event[Max_number_event];}
    ~EPOLL_MINE(){delete []events;}
    int run_client(); //主循环
    int run_server(int MAX_USERS);
    void addfd(int fd);


private : 
    epoll_event *events;
    int epollfd;
    int Max_number_event;
    int sockfd; //需要监听的端口 ， 服务器对应的listen ， 客户端对应的服务器socket
    int ID;

    int setnonblocking(int fd);
    std::map<int , USER_DS> users; //存储用户数据，服务器对应连接到该服务器所有的用户，客户端对应自己的好友列表，在传文件时使用
    inline int client_remsg(epoll_event &event);
    inline int client_sdmsg(epoll_event &event);
    // inline int chulihanshu4(epoll_event &event);
    // inline int chulihanshu5(epoll_event &event);

};
#endif