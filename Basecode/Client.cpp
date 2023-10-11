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

    int sockfd = socket( PF_INET , SOCK_STREAM , 0);
    assert(sockfd >= 0);

    ret = connect(sockfd , (sockaddr*)&address ,sizeof(address));
    if(ret < 0){std::cout <<"connection failed\n";close(sockfd); return 1;}

    int pipefd[2];

    ret = pipe(pipefd);
    assert(ret != -1);
    
    epoll_event events[20];
    int epollfd = epoll_create( 5 );

    addfd(epollfd , sockfd);
    addfd(epollfd , STDIN_FILENO);

    bool stop_server = false;
    while (!stop_server)
    {
        int number = epoll_wait(epollfd , events , 20 , -1);
        if(number < 0){std::cout << "epoll failure\n";break;}
        for(int i = 0 ; i < number ; i++)
        {
            if(events[i].data.fd == sockfd && events[i].events & EPOLLRDHUP)
            {
                std::cout<< "server close the connection\n";
                break;
            }
            else if(events[i].data.fd == sockfd && events[i].events & EPOLLIN)
            {
                char buf[1024];
                memset(buf , '\0' , sizeof(buf));
                ret = recv( sockfd , buf , sizeof(buf) , 0);
                if(ret == 0)
                {
                    std::cout << "server close\n";
                    stop_server = true;
                    break;
                }
                std::cout << "content is : " <<buf <<"\n";
            }
            else if(events[i].data.fd == STDIN_FILENO && events[i].events & EPOLLIN)
            {
                // std::cout << "case STDIN\n";
                ret = splice(STDIN_FILENO , NULL , pipefd[1] , NULL , 3072 , SPLICE_F_MORE | SPLICE_F_MOVE);
                ret = splice(pipefd[0] , NULL , sockfd , NULL , 3072 , SPLICE_F_MORE | SPLICE_F_MOVE);
            }
        }

    }
    

close(sockfd);
return 1;

}