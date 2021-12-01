#include <iostream>
#include "lru_cache.h"
#include "cache_socket.h"
#include "../config.h"
#include <thread>

void Thread_cache1(){
    LRUCache<string,string> lruCache1(LRUcacheMaxsize);
    cache_socket c1;
    c1.work(cache1_port,backlog,lruCache1);
}

void Thread_cache2(){
    sleep(1);
    LRUCache<string,string> lruCache2(LRUcacheMaxsize);
    cache_socket c2;
    c2.work(cache2_port,backlog,lruCache2);
}

void Thread_cache3(){
    sleep(2);
    LRUCache<string,string> lruCache3(LRUcacheMaxsize);
    cache_socket c3;
    c3.work(cache3_port,backlog,lruCache3);
}

int main() {
    thread t1(Thread_cache1);
    thread t2(Thread_cache2);
    thread t3(Thread_cache3);
    t1.join();
    t2.join();
    t3.join();
}