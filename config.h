//
// Created by Yuting on 2021/11/22.
// 配置文件

#ifndef DISTRIBUTED_CACHE_CONFIG_H
#define DISTRIBUTED_CACHE_CONFIG_H

//各个元件ip和端口
#define cache1_ip "127.0.0.1"
#define cache1_port "6767"
#define cache2_ip "127.0.0.1"
#define cache2_port "7878"
#define cache3_ip "127.0.0.1"
#define cache3_port "8989"
#define cache4_ip "127.0.0.1"
#define cache4_port "5656"
#define master_ip "127.0.0.1"
#define master_port "4545"

//初始化client连接cache数量
#define client_cache_Num  3
//listen队列的长度
#define backlog 1024
//序号和端口的映射
std::unordered_map<int,const char *> cache_port={
        {0,master_port},
        {1,cache1_port},
        {2,cache2_port},
        {3,cache3_port},
        {4,cache4_port}
};

//序号和ip的映射
std::unordered_map<int,const char *> cache_ip={
        {0,master_ip},
        {1,cache1_ip},
        {2,cache2_ip},
        {3,cache3_ip},
        {4,cache4_ip}
};

//LRU最大长度
#define LRUcacheMaxsize 20

//自动生成key的大小
#define keylen 6
//自动生成value的大小
#define valuelen 11

//epoll wait等待超时时间
#define keepalive 600//单位：s

//fill模式发送数据间隔
#define interval_data 300//单位：ms

//epoll并发客户端数
#define maxSize 4096

//发送心跳时间间隔
#define heartbeat 5

//写入数据库时间间隔
#define dbtime 5

//cache重连master的时间间隔
#define cachemasterTime 10

//client重连master的时间间隔
#define clientmasterTime 30

//压测client数量
#define pressure 20

//mysql相关
#define mysqlurl "localhost"
#define mysqlUser "root"
#define mysqlPassWord "Fe747698!"
#define dbName "cache"
#define mysqlPort 3306
#define mysqlMaxConn 10
#define mysqlMaxConn_client 100//client池子设大些

//master数据分布存放地址
#define dbMaster_add "../master/dbMaster.json"

//client数据分布存放地址
#define dbClient_add "../client/dbClient.json"

#endif //DISTRIBUTED_CACHE_CONFIG_H
