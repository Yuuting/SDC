//
// Created by user on 2021/11/20.
//

#ifndef DISTRIBUTED_CACHE_CACHE_SOCKET_H
#define DISTRIBUTED_CACHE_CACHE_SOCKET_H

#include <cstdlib>//atoi
#include <netinet/in.h>//sockaddr_in
#include <cassert>//assert
#include <strings.h>//bzaro
#include <arpa/inet.h>//inet_pton
#include <sys/epoll.h>//epoll
#include <unistd.h>//close fd
#include <fcntl.h>//fcntl
#include <cstdio>//fprintf
#include <cstring>//strerror
#include <cerrno>//errno
#include <string>
#include <chrono>
#include <csignal>
#include <mutex>

#include "../common/utils.h"
#include "lru_cache.h"
#include "../log/colorlog.h"
#include "../config.h"

using namespace std;
using namespace chrono;
class cache_socket {
public:
    //判断要连接的是哪个cache
    int getCache(const char *cacheport);

    //cache服务开启于本机，输入cache端口号和最大连接数
    void work(const char *cacheport, int backLog, LRUCache<string, string> &cache);

    //处理服务端发送过来的消息
    string response(string recvmsg, LRUCache<string, string> &cache,const char *cacheport);

    //给master发送心跳包,此端口是master端口
    void sendHeartBeat(const char *masterip, const char *masterport_,const char *cacheport);
};

void cache_socket::sendHeartBeat(const char *masterip, const char *masterport_,const char *cacheport) {
    int port = atoi(masterport_);
    int cache_num = getCache(cacheport);
    string tempS="[cache"+ to_string(cache_num)+"] : ->[master]";
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, masterip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw std::range_error("socket was not created successfully");
    }

    if (connect(sockfd, (struct sockaddr *) &address, sizeof(address))<0) {
        ALERT(tempS.c_str(),Error: connect);
        return;
    }

    setnonblocking(sockfd);
    VERBOSE(tempS.c_str(),已连接master);
    while(1){
        char buffer[255];
        memset(buffer, '\0', sizeof(buffer));
        string heartBeat="[cache"+ to_string(cache_num)+"]存活";
        strcpy(buffer,heartBeat.c_str());

        auto bytes_write = send(sockfd, buffer, strlen(buffer), 0);
        if (bytes_write < 0) {
            ALERT(tempS.c_str(), 写入socket%d的消息失败, sockfd);
        } else if (bytes_write == 0) {
            ALERT(tempS.c_str(), 写入socket%d的消息失败, sockfd);
        }else{
            INFO(tempS.c_str(), 写入%d bytes消息到socket%d中%c内容是%s, strlen(buffer), sockfd,',',buffer);
        }
        sleep(heartbeat);
    }

}

int cache_socket::getCache(const char *cacheport) {
    for (auto it = cache_port.begin(); it != cache_port.end(); ++it) {
        if (it->second == cacheport) { return  it->first; }
    }
}

void cache_socket::work(const char *cacheport, int backLog, LRUCache<string, string> &cache) {
    int port = atoi(cacheport);
    int cache_num = getCache(cacheport);
    string tempS="[cache"+ to_string(cache_num)+"]";
    VERBOSE(tempS.c_str(),%s,"启动");

    // socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        ALERT("Error",%s,"socket");
        return;
    }
    int ret = 0;
    int reuse = 1;
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuse, sizeof(int));
    if (ret < 0) {
        perror("setsockopt");
        _exit(-1);
    }
    /*
     *每一个进程有一个独立的监听socket，并且bind相同的ip:port，独立的listen()和accept()；提高接收连接的能力。（例如nginx多进程同时监听同一个ip:port）
    解决的问题：
    （1）避免了应用层多线程或者进程监听同一ip:port的“惊群效应”。
    （2）内核层面实现负载均衡，保证每个进程或者线程接收均衡的连接数。
    （3）只有effective-user-id相同的服务器进程才能监听同一ip:port
     */

    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT,(const void *)&reuse , sizeof(int));
    if (ret < 0) {
        perror("setsockopt");
        _exit(-1);
    }


    // bind
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        ALERT("Error",%s,"bind");
        return;
    }
    // listen
    if (listen(listenfd, backLog) == -1) {
        ALERT("Error",%s,"listen");
        return;
    }

    VERBOSE(tempS.c_str(),%s,"listening...");

    setnonblocking(listenfd);
    int epollfd= epoll_create(5);
    addfd(epollfd,listenfd);
    if(epollfd<0){
        perror("epoll create");
    }
    struct epoll_event events[maxSize];

    int conn;
    while (true) {
        int number=epoll_wait(epollfd,events,maxSize,keepalive*1000);
        if(number<0 && errno!=EINTR)
        {
            perror("epoll failed.");
            break;
        }
        else if(number==0){
            if(conn>0){
                //数据库写数据的逻辑部分放在这里，这里表示超时断连时往mysql中写一次
                VERBOSE(tempS.c_str(), %s, "将cache中的数据写入mysql数据库中");
                cache.writeToMysql(cache_num);
                removefd(epollfd, conn);
                conn=0;
                VERBOSE(tempS.c_str(), %s, "客户端未访问时间过长，服务端自动断开与客户端之间的连接");
            }else if(conn==0){
                //数据库写数据的逻辑部分放在这里，这里表示服务器cache处于空闲状态,keepalive*1000时间写入数据库
                VERBOSE(tempS.c_str(), %s, "将cache中的数据写入mysql数据库中");
                cache.writeToMysql(cache_num);
            }
        }

        for (int i = 0; i < number; i++) {
            if (events[i].events & EPOLLERR) {
                perror("epoll_wait returned EPOLLERR");
            }
            if (events[i].data.fd == listenfd) {
                if (events[i].events & EPOLLIN) {
                    // accept

                    char clientIP[INET_ADDRSTRLEN] = "";
                    struct sockaddr_in clientAddr;
                    socklen_t clientAddrLen = sizeof(clientAddr);
                    conn = accept(listenfd, (struct sockaddr *) &clientAddr, &clientAddrLen);

                    if (conn < 0) {
                        ALERT("Error",%s,"accept");
                        continue;
                    }
                    if (conn > 0) {
                        addfd(epollfd, conn);
                    }
                    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
                    VERBOSE(tempS.c_str(), connect %s:%i,clientIP,ntohs(clientAddr.sin_port));
                }
            } else {
                int sockfd = events[i].data.fd;

                if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {//出错或者断开
                    removefd(epollfd, sockfd);
                } else {
                    if (events[i].events & EPOLLOUT) {//可写
                        char buf[255];
                        strcpy(buf, "客户端异常，准备断开连接......\n");
                        int n = write(sockfd, buf, sizeof(buf));
                        ALERT(tempS.c_str(), %s, buf);
                        if (n > 0) {
                            epoll_event _epev{};
                            _epev.events = EPOLLIN;
                            _epev.data.fd = events[i].data.fd;
                            epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &_epev);
                        }
                    }
                    if (events[i].events & EPOLLIN) {
                        char buf[255];
                        memset(buf, 0, sizeof(buf));
                        //非阻塞模式没有很大的用处
                        struct timeval tv;
                        tv.tv_sec = keepalive;
                        tv.tv_usec = 0;
                        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
                        struct linger sl;
                        sl.l_onoff = 1;
                        sl.l_linger = 0;
                        setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
                        int len = read(sockfd, buf, sizeof(buf));
                        if (len < 0) {
                            if (errno == EWOULDBLOCK) {
                                removefd(epollfd, conn);
                                ALERT(tempS.c_str(), %s, "读取socket出错");
                                break;
                            } else {
                                removefd(epollfd, sockfd);
                                ALERT(tempS.c_str(), %s, "连接超时");
                                break;
                            }
                        }

                        buf[len] = '\0';

                        string sendMsg = response(buf, cache, cacheport);
                        if (sendMsg == "接收到客户端的异常信息，准备断开连接......") {
                            removefd(epollfd, sockfd);
                            conn=0;
                            //数据库写数据的逻辑部分放在这里，这里表示有连接即将断连
                            VERBOSE(tempS.c_str(), %s, "将cache中的数据写入mysql数据库中");
                            cache.writeToMysql(cache_num);
                            VERBOSE(tempS.c_str(), %s, "断开与客户端之间的连接");
                            break;
                        }
                        memset(buf, '\0', sizeof(buf));
                        strcpy(buf, sendMsg.c_str());
                        write(sockfd, buf, sizeof(buf));
                        memset(buf, '\0', len);
                    }
                }
            }
        }
    }
    close(listenfd);
}

string cache_socket::response(string recvmsg, LRUCache<string, string> &cache,const char *cacheport) {
    string sendmsg;
    int cache_num = getCache(cacheport);
    string prefix;
    prefix+="[cache";
    prefix+= to_string(cache_num);
    prefix+="]";
    string success_prefix=prefix+string(" : 接收到客户端消息");
    if(recvmsg==""){
        ALERT(success_prefix.c_str(),%s,"客户端断开连接");
        return "接收到客户端的异常信息，准备断开连接......";
    }else if(!(starts_with(recvmsg,"Put:")||starts_with(recvmsg,"Get:"))){
        return "输入的读写命令错误";
    }
    SUCCESS(success_prefix.c_str(),%s,recvmsg.c_str());
    try{
        string pre=split(recvmsg, ":")[0];
        if (pre == "Get") {
            try {
                string getElem = cache.Get(split(split(recvmsg, ":")[1], ",")[0]);
                sendmsg = split(split(recvmsg, ":")[1], ",")[0] + "在LRU缓存队列中，值为" + getElem;
                WARNING(prefix.append(" : 有该(key，value)对，回复client端").c_str(),%s,sendmsg.c_str());
            } catch (exception &e) {
                sendmsg = e.what();
                ALERT(prefix.append(" : 无该(key，value)对，回复client端").c_str(),%s,sendmsg.c_str());
            }

        } else if (pre == "Put") {
            cache.Put(split(split(recvmsg, ":")[1], ",")[0], split(split(recvmsg, ":")[1], ",")[1]);
            sendmsg += '(' + split(split(recvmsg, ":")[1], ",")[0] + "," + split(split(recvmsg, ":")[1], ",")[1];
            sendmsg.append(")被加入LRU缓存队列中");
            INFO(prefix.append(" : 将该(key，value)对放入缓存中，回复client端").c_str(),%s,sendmsg.c_str());
        }
    }catch (exception &e){
        sendmsg="接收到客户端的异常信息，准备断开连接......";
    }
    return sendmsg;
}

#endif //DISTRIBUTED_CACHE_CACHE_SOCKET_H
