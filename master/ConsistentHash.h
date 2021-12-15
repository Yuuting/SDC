//
// Created by Yuting on 2021/12/12.
//

#ifndef DISTRIBUTED_CACHE_CONSISTENTHASH_H
#define DISTRIBUTED_CACHE_CONSISTENTHASH_H
#include <bits/stdc++.h>

using namespace  std;

class ConsistentHash{
private:
    unordered_set<string> physicalServer; // 真实的物理机器
    map<std::size_t , string> serverNodes; // hash 值和端口号映射
    int virtualNodeNum;
    hash<string> hashStr; // 自带的 hash 函数
public:
    ConsistentHash(int vnum):virtualNodeNum(vnum){};
    void addNode(const string& port){
        physicalServer.insert(port);
        // 加入物理节点的时候也加入虚拟节点
        for(int i=0;i<virtualNodeNum;++i){
            stringstream key;
            key<<port<<"#"<<i;
            serverNodes.insert({hashStr(key.str()),port});
        }
    }
    void delNode(const string &port){
        physicalServer.erase(port);
        for(int i=0;i<virtualNodeNum;++i){
            stringstream key;
            key<<port<<"#"<<i;
            serverNodes.erase(hashStr(key.str()));
        }
    }
    // 模拟插入字符串到 hash string
    string virtualInsert(string data){
        stringstream key;
        key<<data;
        size_t hashKey=hashStr(key.str());
        auto iter=serverNodes.lower_bound(hashKey);
        if(iter==serverNodes.end()){
            return serverNodes.begin()->second;
        }
        return iter->second;
    }
};

#endif //DISTRIBUTED_CACHE_CONSISTENTHASH_H
