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

#define MAX_EVENT 1024
#define MAX_USERS 5
static int pipefd[2];

int setnonblocking(int fd){
    int old_option = fcntl( fd , F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd , F_SETFL , new_option);
    return old_option;
}

void addfd(int epollfd , int fd){
    epoll_event events;
    events.data.fd = fd ;
    events.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd , EPOLL_CTL_ADD , fd , &events);
    setnonblocking(fd);
}

void sig_hanlder( int sig) 
{
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1] , (char*)&msg , 1 , 0); //将信号写入通道，通知程序处理
    errno = save_errno;
}

void add_sig( int sig){
    struct sigaction sa;
    memset(&sa , '\0' , sizeof(sa));
    sa.sa_handler = sig_hanlder;
    sa.sa_flags |= SA_RESTART;
    sigfillset( &sa.sa_mask);
    assert(sigaction(sig , &sa , NULL) != -1);
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
    ret = socketpair(PF_UNIX , SOCK_STREAM , 0 , pipefd);
    assert(ret != -1);

    epoll_event events[MAX_EVENT];
    int epollfd = epoll_create(20);

    addfd(epollfd , listenfd);
    setnonblocking(pipefd[1]);
    addfd(epollfd , pipefd[0]);

    add_sig(SIGHUP);
    add_sig(SIGCHLD);
    add_sig(SIGTERM);
    add_sig(SIGINT);

    std::map<int , user_ds> users;
    int user_count = 0;
    bool stop_server = false;
    while (!stop_server)
    {
        int number = epoll_wait(epollfd , events , MAX_EVENT , -1);
        if(number < 0){std::cout << "epoll failure\n";break;}
        for( int i = 0 ; i < number ; i++)
        {
            // std::cout << "case 1\n";
            // auto fd = events[i].data.fd;
            // auto event = events[i].events;
            if(events[i].data.fd == listenfd && events[i].events & EPOLLIN)
            {
                //user is get
                sockaddr_in client_address;
                socklen_t size = sizeof(client_address);
                int connfd = accept(listenfd , (sockaddr*)&client_address , &size);
                if(connfd < 0){std::cout << "errno is : "<< errno <<'\n';}
                if(user_count >= MAX_USERS)
                {
                    std::string str("too many users");
                    auto info = str.c_str();
                    send(events[i].data.fd , info , strlen(info) , 0);
                }
                std::cout << "have a new user , we have "<< user_count+1 << " ueser\n";
                user_ds user;
                memset(&user , '\0' , sizeof(user));
                setnonblocking(connfd);
                addfd(epollfd , connfd);
                user.fd = connfd;
                users.insert({connfd , user});
                user_count++;
            }
            else if(events[i].events & EPOLLERR)
            {
                // std::cout << "case 2\n";
                std::cout <<"get an error from " << events[i].data.fd << '\n';
                char errors[100];
                memset( errors , '\0' , sizeof(errors));
                socklen_t length = sizeof(errors);
                if(getsockopt(events[i].data.fd , SOL_SOCKET,SO_ERROR,&errors , &length) < 0)
                {
                    std::cout << "get socket option failed\n";
                }
                continue;;
            }
            else if (events[i].events & EPOLLRDHUP)
            {
                std::cout << "case 3\n";
                //客户端关闭连接，则服务器删除改用户
                users.erase(events[i].data.fd);
                close(events[i].data.fd);
                user_count--;
            }
            else if(events[i].events & EPOLLIN)
            {
                // std::cout << "case 4\n";
                //有信息到来
                char buf[1024];
                memset(buf , '\0' , sizeof(buf));
                ret = recv(events[i].data.fd , buf , sizeof(buf) , 0);
                std::cout << "get " << ret <<" bytes, from " << events[i].data.fd << " say : " <<"\n"; 
                printf("%s\n" , buf);
                if(ret < 0)
                {
                    //操作出错，关闭连接
                    users.erase(events[i].data.fd);
                    close(events[i].data.fd);
                    user_count--;
                }
                else if (ret == 0)
                {
                    // std::cout << "case 3\n";
                    //客户端关闭连接，则服务器删除改用户
                    users.erase(events[i].data.fd);
                    close(events[i].data.fd);
                    user_count--;
                }
                else 
                {
                    //转发信息
                    for(auto c : users)
                    {
                        if(c.second.fd == events[i].data.fd)
                        {
                            continue;
                        }
                        ret = send(c.second.fd , buf , strlen(buf) , 0);
                        // if (ret == 0)
                        // {
                        //     //客户关闭连接                           
                        // }
                    }
                }
            }
            else if( events[i].data.fd == pipefd[0] && events[i].events & EPOLLIN)
            {
                int sig;
                char signals[1024];
                ret = recv(events[i].data.fd , signals , sizeof(signals) , 0);
                if(ret == -1)
                {
                    continue;
                }
                else if(ret == 0)
                {
                    continue;
                }
                else
                {
                    for (int z = 0 ; z < ret ; z++)
                    {
                        switch(signals[z])
                        {
                            case SIGCHLD:
                            case SIGHUP:
                            {
                                continue;
                            }
                            case SIGTERM:
                            case SIGINT:
                            {
                                stop_server = true;
                            }
                        }
                    }
                }
            }
            else
            {

            }
            
        }
    }
    
close(listenfd);
close(pipefd[0]);
close(pipefd[1]);
return 0;
}