#include <iostream>
#include "lru_cache.h"
#include "cache_socket.h"
#include "../config.h"
#include <thread>

void Thread1(LRUCache<string,string> &lruCache,cache_socket &c,const char* &cachePort){

    pid_t status;
    status= fork();
    if(status == -1)
    {
        printf("Create ChildProcess Errror!\n");
        exit(1);
    }
    else if(status == 0)
    {
        c.sendHeartBeat(master_ip,master_port,cachePort);
    }
    else
    {
        c.work(cachePort,backlog,lruCache);
    }
}

void Thread2(LRUCache<string,string> &lruCache,int cache){
    while(1){
        lruCache.writeToMysql(cache);
        string prefix="[cache"+ to_string(cache)+"]";
        VERBOSE(prefix.c_str(), %s, "将cache中的数据写入mysql数据库中");
        sleep(dbtime);
    }

}

int main(int argc,char* argv[]) {
    assert(argc==2);
    int cache=atoi(argv[1]);
    const char* cachePort=cache_port[cache];
    LRUCache<string,string> lruCache(LRUcacheMaxsize);
    cache_socket c;
    thread t1(Thread1,ref(lruCache),ref(c),ref(cachePort));
    thread t2(Thread2, ref(lruCache), ref(cache));
    t1.join();
    t2.join();
}