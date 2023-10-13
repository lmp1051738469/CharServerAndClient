#include "EPOLLMINE.h"

int pipefd[2]; //传输信号的管道

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


int EPOLL_MINE::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void EPOLL_MINE::addfd(int fd)
{
    epoll_event event_tem;
    event_tem.data.fd = fd;
    event_tem.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event_tem);
    setnonblocking(fd);
}

int EPOLL_MINE::client_remsg(epoll_event &event)
{
    // 有消息到

    char buf[1024];
    memset(buf, '\0', sizeof(buf));
    int ret = 0;
    ret = recv(sockfd, buf, sizeof(buf), 0);
    if (ret == 0)
    {
        std::cout << "server close\n";
        return 0;
    }
    // std::cout << "content is : " <<buf <<"\n";
    std::stringstream ss(buf);
    Message msg;
    msg.read_content(ss);
    printf("%d say : %s\n", msg.get_id(), msg.str_c());
    return 1;
}

int EPOLL_MINE::client_sdmsg(epoll_event &event)
{
    //有消息发
    char buf[1024];
    memset(buf , '\0' , sizeof(buf));
    
    int ret = read( STDIN_FILENO , buf , sizeof(buf));
    // printf("ret = %d buf : %s",ret ,buf);

    Message msg(ID , (std::string)buf);

    // printf("%d say : %s",msg.get_id(),msg.str_c());

    std::stringstream ss;
    msg.send_content(ss);
    std::string temp = ss.str();
    auto send_ = temp.c_str();
    // printf("send_ : %s , size %ld",send_,strlen(send_));
    ret = send(sockfd , send_ , strlen(send_) , 0);
    return 1;
}

int EPOLL_MINE::run_client()
{
    addfd(sockfd);
    addfd(STDIN_FILENO);
    bool stop_server = false;
    while (!stop_server)
    {
        int number = epoll_wait(epollfd, events, Max_number_event, -1);
        if (number < 0)
        {
            std::cout << "epoll failure\n";
            break;
        }
        for (int i = 0; i < number; ++i)
        {
            if (events[i].data.fd == sockfd && events[i].events & EPOLLRDHUP)
            {
                std::cout << "server close the connection\n";
                break;
            }
            else if (events[i].data.fd == sockfd && events[i].events & EPOLLIN)
            {
                // 有消息到
                if(client_remsg(events[i]) == 0) 
                {
                    std::cout << "有信息解析错误\n";
                    stop_server = true;
                    break;
                }
            }
            else if(events[i].data.fd == STDIN_FILENO && events[i].events & EPOLLIN)
            {
                //标准输入端有信息到
                if(client_sdmsg(events[i]) == 1) continue;
            }           
        }
    }
    close(pipefd[0]);
    close(pipefd[1]);
    return 1;
}

int EPOLL_MINE::run_server(int MAX_USERS)
{   
    int ret = socketpair(PF_UNIX , SOCK_STREAM , 0 , pipefd);
    assert(ret != -1);
    addfd(sockfd);
    setnonblocking(pipefd[1]);
    addfd(pipefd[0]);

    add_sig(SIGHUP);
    add_sig(SIGCHLD);
    add_sig(SIGTERM);
    add_sig(SIGINT);
       
    int user_count = 0;
    bool stop_server = false;
    while (!stop_server)
    {
        int number = epoll_wait(epollfd, events, Max_number_event, -1);
        if (number < 0)
        {
            std::cout << "epoll failure\n";
            break;
        }
        for (int i = 0; i < number; ++i)
        {
            if (events[i].data.fd == sockfd && events[i].events & EPOLLIN)
            {
                //有新连接到来
                sockaddr_in client_address;
                socklen_t size = sizeof(client_address);
                int connfd = accept(sockfd , (sockaddr*)&client_address , &size);
                if(connfd < 0){std::cout << "errno is : "<< errno <<'\n';}
                if(user_count >= MAX_USERS)
                {
                    std::string str("too many users");
                    auto info = str.c_str();
                    send(events[i].data.fd , info , strlen(info) , 0);
                }
                std::cout << "have a new user , we have "<< user_count+1 << " ueser\n";
                USER_DS user;
                memset(&user , '\0' , sizeof(user));
                setnonblocking(connfd);
                addfd(connfd);
                user.fd = connfd;
                users.insert({connfd , user});
                user_count++;
            }
            else if (events[i].events & EPOLLERR)
            {
                std::cout <<"get an error from " << events[i].data.fd << '\n';
                char errors[100];
                memset( errors , '\0' , sizeof(errors));
                socklen_t length = sizeof(errors);
                if(getsockopt(events[i].data.fd , SOL_SOCKET,SO_ERROR,&errors , &length) < 0)
                {
                    std::cout << "get socket option failed\n";
                }
                continue;
            }
            else if(events[i].events & EPOLLRDHUP)
            {
                // std::cout << "case 3\n";
                //客户端关闭连接，则服务器删除改用户
                users.erase(events[i].data.fd);
                close(events[i].data.fd);
                user_count--;
                printf("%d left the house, we have %d users",events[i].data.fd,user_count);
            }    
            else if(events[i].events & EPOLLIN)
            {
                // std::cout << "case 4\n";
                //有信息到来
                char buf[1024];
                memset(buf , '\0' , sizeof(buf));
                int ret = recv(events[i].data.fd , buf , sizeof(buf) , 0);
                // std::cout << "get " << ret <<" bytes, from " << events[i].data.fd << " say : " <<"\n"; 
                // printf("%s\n" , buf);
                if(ret < 0)
                {
                    //操作出错，关闭连接
                    users.erase(events[i].data.fd);
                    close(events[i].data.fd);
                    user_count--;
                    printf("%d was lefted the house, we have %d users",events[i].data.fd,user_count);
                }
                else if (ret == 0)
                {
                    // std::cout << "case 3\n";
                    //客户端关闭连接，则服务器删除改用户
                    users.erase(events[i].data.fd);
                    close(events[i].data.fd);
                    user_count--;
                    printf("%d left the house, we have %d users",events[i].data.fd,user_count);

                }
                else 
                {
                    Message msg;
                    std::stringstream ss(buf);
                    msg.read_content(ss);
                    printf("%d say : %s\n",msg.get_id() , msg.str_c());
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
                int ret = recv(events[i].data.fd , signals , sizeof(signals) , 0);
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
        }
    }
    return 1;
}