//
// Created by user on 2021/12/12.
//
#include "ConsistentHash.h"
int main(){
    ConsistentHash hash(10);
    hash.addNode("127.0.0.1");
    hash.addNode("127.0.0.2");
    hash.addNode("127.0.0.3");
    hash.addNode("127.0.0.4");
    map<string,int> stats;

    for(int i=0;i<1000;++i){
        stats[hash.virtualInsert(to_string(i))]++;

    }
    for(auto &s:stats){
        cout<< s.first <<" "<<setprecision(2)<< s.second/(1000*1.0)<<endl;
    }
    return 0;
}