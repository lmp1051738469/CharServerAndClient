#define _GUN_SOURCE 1
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
#include"Message.h"
#include"EPOLLMINE.h"

int setnonblocking(int fd){
    int old_option = fcntl( fd , F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd , F_SETFL , new_option);
    return old_option;
}

void addfd(int epollfd , int fd)
{
    epoll_event events;
    events.data.fd = fd ;
    events.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd , EPOLL_CTL_ADD , fd , &events);
    setnonblocking(fd);
}


int main(int argc , char* argv[])
{
    if(argc <= 3 ){
        std::cout << "usage : " << basename(argv[0]) << "ip_address port_number name\n";
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int ID = atoi(argv[3]);
    printf("ID : %d\n" , ID);
    int ret = 0;
    sockaddr_in address;
    bzero(&address , sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET , ip , &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket( PF_INET , SOCK_STREAM , 0);
    assert(sockfd >= 0);

    ret = connect(sockfd , (sockaddr*)&address ,sizeof(address));
    if(ret < 0){std::cout <<"connection failed\n";close(sockfd); return 1;}

    int pipefd[2];

    ret = pipe(pipefd);
    assert(ret != -1);
    EPOLL_MINE EPOLL(5 , 20 , sockfd , ID);
    EPOLL.run_client();

    // printf("id\tcontent\n");
    // printf("%d \t: ",ID);


    close(sockfd);
    return 1;

}