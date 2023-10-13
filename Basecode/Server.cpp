#include<cstdio>
#include<iostream>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<sys/errno.h>
#include<signal.h>
#include<cassert>
#include<fcntl.h>
#include<cstring>
#include<map>
#include"EPOLLMINE.h"

#define ROOT 0
#define MAX_EVENT 1024
#define MAX_USERS 5

int setnonblocking(int fd){
    int old_option = fcntl( fd , F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd , F_SETFL , new_option);
    return old_option;
}

struct user_ds
{
    int fd;
};


int main(int argc , char* argv[])
{
    if(argc <= 2 ){
        std::cout << "usage : " << basename(argv[0]) << "ip_address port_number\n";
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    sockaddr_in address;
    bzero(&address , sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET , ip , &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket( PF_INET , SOCK_STREAM , 0);
    assert(listenfd >= 0);
    ret = bind(listenfd , (sockaddr*)&address, (socklen_t)sizeof(address));
    if(ret == -1){
        std::cout << "errno is : " << errno << '\n';
        return 1;
    }
    ret = listen(listenfd , 20);
    assert(ret != -1);
    EPOLL_MINE EPOLL( 5 , 20 , listenfd , ROOT);
    EPOLL.run_server(10);
    
close(listenfd);

return 0;
}