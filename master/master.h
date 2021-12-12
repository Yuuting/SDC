//
// Created by user on 2021/12/10.
//

#ifndef DISTRIBUTED_CACHE_MASTER_H
#define DISTRIBUTED_CACHE_MASTER_H

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
#include <unordered_map>

#include "../common/utils.h"
#include "../log/colorlog.h"
#include "../config.h"
#include "ConsistentHash.h"
#include "../common/utils.h"
#include "../common/json.hpp"

using namespace std;
using namespace chrono;
using namespace nlohmann;

class master{
public:
    //构造函数，开启一致性哈希功能
    explicit master(): hash(10){
        hash.addNode(cache1_port);
        hash.addNode(cache2_port);
        hash.addNode(cache3_port);
    }
    //master服务开启于本机，输入cache端口号和最大连接数
    void recvHeartBeat(const char *masterPort, int backLog);

    //一致性哈希处理客户端发来的消息
    string cache_com(string recvmsg);
private:
    ConsistentHash hash;
    json dbMaster;
};
void master::recvHeartBeat(const char *masterPort, int backLog) {
    int port = atoi(masterPort);
    VERBOSE("[master]",%s,"启动");

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

    VERBOSE("[master]",%s,"listening...");

    setnonblocking(listenfd);
    int epollfd= epoll_create(5);
    addfd(epollfd,listenfd);
    if(epollfd<0){
        perror("epoll create");
    }
    struct epoll_event events[maxSize];
    int conn;
    unordered_map<int,int> socket_cache;
    while (true) {
        int number=epoll_wait(epollfd,events,maxSize,0);
        if(number<0 && errno!=EINTR)
        {
            perror("epoll failed.");
            break;
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
                    VERBOSE("[master]", connect %s:%i,clientIP,ntohs(clientAddr.sin_port));
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
                        ALERT("[master]", %s, buf);
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
                                ALERT("[master]", %s, "读取socket出错");
                                break;
                            } else {
                                removefd(epollfd, sockfd);
                                ALERT("[master]", %s, "连接超时");
                                break;
                            }
                        }else if(len==0){
                            //cache坏的时候会发个空包
                            if(socket_cache[sockfd]==0){
                                WARNING("[master]发现",client退出);
                            }else{
                                ALERT("[master]发现",cache%i故障%s,socket_cache[sockfd],",开始重新划分数据分布");
                            }
                            removefd(epollfd, sockfd);
                            break;
                        }
                        buf[len] = '\0';
                        INFO("[master]",收到消息%s%s,",",buf);

                        string message=buf;
                        if(starts_with(message,"Put")|| starts_with(message,"Get")){
                            socket_cache.insert(make_pair(sockfd,0));
                            string sendMsg = cache_com(message);
                            memset(buf, '\0', sizeof(buf));
                            strcpy(buf, sendMsg.c_str());
                            write(sockfd, buf, sizeof(buf));
                            memset(buf, '\0', len);
                        }else{
                            socket_cache.insert(make_pair(sockfd,buf[6]-'0'));
                        }

                        memset(buf, '\0', len);
                    }
                }
            }
        }
    }
    close(listenfd);
}

string master::cache_com(string recvmsg){
    string sendmsg;
    string value;
    try{
        string pre=split(recvmsg, ":")[0];
        if (pre == "Get") {
            string key = split(recvmsg, ":")[1];

            try{
                value=dbMaster[key];
                int cache;
                for (auto it = cache_port.begin(); it != cache_port.end(); ++it) {
                    if (it->second == value) { cache = it->first; }
                }
                WARNING("[master]",发现key在cache%d中,cache);
            }catch (exception &e){
                value="";
                WARNING("[master]",未发现此key);
            }
            sendmsg = value;
        } else if (pre == "Put") {
            string key=split(split(recvmsg, ":")[1], ",")[0];
            string port=hash.virtualInsert(key);
            sendmsg=port;
            dbMaster[key]=port;
            ofstream o(dbMaster_add);
            o << std::setw( 4 )<<dbMaster << std::endl;
            int cache;
            for (auto it = cache_port.begin(); it != cache_port.end(); ++it) {
                if (it->second == port) { cache = it->first; }
            }
            SUCCESS("[master]", 决定将此key放入cache%d中, cache);
        }
    }catch (exception &e){
        sendmsg="接收到客户端的异常信息，准备断开连接......";
    }
    return sendmsg;
}
#endif //DISTRIBUTED_CACHE_MASTER_H
