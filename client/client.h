//
// Created by user on 2021/11/21.
//

#ifndef DISTRIBUTED_CACHE_CLIENT_H
#define DISTRIBUTED_CACHE_CLIENT_H

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
#include <iostream>//cout
#include <random>
#include <unordered_map>
#include <functional>
#include <chrono>

#include "../common/utils.h"
#include "../log/colorlog.h"
#include "../config.h"

using namespace std;


class client {
public:
    //判断要连接的是哪个cache
    int getCache(const char *cacheport);

    //决定使用哪个socket通信，后期根据本地缓存进行决定
    int choose(int socket1,int socket2,int socket3);

    //随机生成等长的key和value
    string k_v_gen(int key_length, int value_length);

    //根据k_v产生发送给服务器cache的消息
    string message(string k_v);

    //连接特定服务器和端口
    int start_conn(const char *ip, const char *port_);

    //将消息写入socket
    int write_nbytes(int sockfd, const char *buffer, int len ,const char *cacheport);

    //从socket中读入消息
    int read_once(int sockfd, char *buffer, int len ,const char *cacheport);

private:
    unordered_map<string, string> k_v_map;

};

int client::choose(int socket1,int socket2,int socket3){
    int random_num=(rand() % 3)+ 1;
    if(random_num==1){
        return socket1;
    }else if(random_num==2){
        return socket2;
    }else{
        return socket3;
    }
}

int client::getCache(const char *cacheport){
    if (cacheport == cache1_port) {
        return 1;
    } else if (cacheport == cache2_port) {
        return 2;
    } else if (cacheport == cache3_port) {
        return 3;
    }
}

string client::k_v_gen(int key_length, int value_length) {
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    uniform_int_distribution<int> distribution(0, 9);
    auto dice = std::bind(distribution, generator);

    string random_key;
    string random_value;
    for (int i = 0; i < key_length; i++) {
        random_key += to_string(dice());
    }
    for (int i = 0; i < value_length; i++) {
        random_value += to_string(dice());
    }
    return random_key + "," + random_value;

}

string client::message(string k_v) {
    vector<string> kv = split(k_v, ",");

    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    uniform_int_distribution<int> distribution(0, 2);
    auto dice = std::bind(distribution, generator);

    string mes;
    static int flag = 0;
    if (flag == 0) {
        mes = "Put:" + k_v;
        k_v_map.insert(make_pair(kv[0], kv[1]));
        flag += 1;
    } else {
        if (dice() == 0) {
            uniform_int_distribution<int> distribution_map(0, k_v_map.size() - 1);
            auto dice_map = std::bind(distribution_map, generator);
            auto random_it = std::next(std::begin(k_v_map), dice_map());
            mes = "Get:" + random_it->first;
        } else {
            mes = "Put:" + k_v;
            k_v_map.insert(make_pair(kv[0], kv[1]));
        }
    }

    return mes;
}

int client::start_conn(const char *ip, const char *port_) {
    int port = atoi(port_);
    int cache_num = getCache(port_);
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw std::range_error("socket was not created successfully");
    }

    if (connect(sockfd, (struct sockaddr *) &address, sizeof(address))<0) {
        ALERT("[客户端]",Error: connect);
        return 0;
    }

    setnonblocking(sockfd);
    VERBOSE("[客户端]",已连接cache%i,cache_num);
    return sockfd;
}

int client::read_once(int sockfd, char *buffer, int len ,const char *cacheport) {
    int bytes_read = 0;
    memset(buffer, '\0', len);
    int cache_num = getCache(cacheport);
    bytes_read = recv(sockfd, buffer, len, 0);
    if (bytes_read == -1) {
        //ALERT("[客户端]", 读取来自socket % d的消息失败-- -- -- -- -- -- -, sockfd);
        return -1;
    } else if (bytes_read == 0) {
        ALERT("[客户端]", 读取来自cache%d的消息失败, cache_num);
        return 0;
    }else{
        SUCCESS("[客户端]", 读取%d bytes来自cache%d的消息%s内容是:%s, bytes_read, cache_num, ",", buffer);
        return 1;
    }
}

int client::write_nbytes(int sockfd, const char *buffer, int len ,const char *cacheport) {
    int bytes_write = 0;
    int cache_num = getCache(cacheport);
    bytes_write = send(sockfd, buffer, len, 0);
    if (bytes_write < 0) {
        ALERT("[客户端]", 写入cache%d的消息失败, cache_num);
        return -1;
    } else if (bytes_write == 0) {
        ALERT("[客户端]", 写入cache%d的消息失败, cache_num);
        return -1;
    }else{
        INFO("[客户端]", 写入%d bytes消息到cache%d中%c内容是%s, len, cache_num,',',buffer);
        return 1;
    }
}

#endif //DISTRIBUTED_CACHE_CLIENT_H
