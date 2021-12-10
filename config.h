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
#define master_ip "127.0.0.1"
#define master_port "4545"
#define backlog 1024

#define LRUcacheMaxsize 20
#define keylen 6
#define valuelen 11

#define keepalive 60//单位：s
#define interval_data 300//单位：ms
#define maxSize 4096//epoll并发客户端数

#define heartbeat 5

#define pressure 20

#define mysqlurl "localhost"
#define mysqlUser "root"
#define mysqlPassWord "Fe747698!"
#define dbName "cache"
#define mysqlPort 3306
#define mysqlMaxConn 50


#endif //DISTRIBUTED_CACHE_CONFIG_H
