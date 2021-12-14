//
// Created by user on 2021/11/22.
//

#ifndef DISTRIBUTED_CACHE_CONFIG_H
#define DISTRIBUTED_CACHE_CONFIG_H

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
#define client_cache_Num  3
#define backlog 1024

std::unordered_map<int,const char *> cache_port={
        {0,master_port},
        {1,cache1_port},
        {2,cache2_port},
        {3,cache3_port},
        {4,cache4_port}
};
std::unordered_map<int,const char *> cache_ip={
        {0,master_ip},
        {1,cache1_ip},
        {2,cache2_ip},
        {3,cache3_ip},
        {4,cache4_ip}
};
#define LRUcacheMaxsize 20
#define keylen 6
#define valuelen 11

#define keepalive 600//单位：s
#define interval_data 300//单位：ms
#define maxSize 4096//epoll并发客户端数

#define heartbeat 60

#define dbtime 5
#define pressure 20

#define mysqlurl "localhost"
#define mysqlUser "root"
#define mysqlPassWord "Fe747698!"
#define dbName "cache"
#define mysqlPort 3306
#define mysqlMaxConn 10
#define mysqlMaxConn_client 100

#define dbMaster_add "../master/dbMaster.json"
#define dbClient_add "../client/dbClient.json"

#endif //DISTRIBUTED_CACHE_CONFIG_H
