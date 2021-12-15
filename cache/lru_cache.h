//
// Created by Yuting on 2021/11/21.
//

#ifndef DISTRIBUTED_CACHE_LRU_CACHE_H
#define DISTRIBUTED_CACHE_LRU_CACHE_H

#include <list>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <string>
#include "../log/colorlog.h"
#include "../config.h"
#include "../mysql/connectionRAII.h"

using namespace std;

template<typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t maxsize) : max_cache_size(maxsize) {
        if (max_cache_size == 0) {
            throw invalid_argument{"Size of the cache should be non-zero"};
        }
        //连接池初始化
        connPool = connection_pool::GetInstance();
        connPool->init(mysqlurl, mysqlUser, mysqlPassWord, dbName,mysqlPort,mysqlMaxConn);
    }

    ~LRUCache() noexcept {
        Clear();
    }

    //放入
    void Put(const Key &key, const Value &value) {
        lock_guard<mutex> lock(safe_op);
        auto value_it = key_finder.find(key);
        if (value_it == key_finder.end()) {
            if (lru_queue.size() + 1 > max_cache_size) {
                WARNING("此cache的lru队列状态",已满);
                Erase();
            }
            Insert(key, value);
        } else {
            Update(key, value);
        }
    }

    //取出
    const Value &Get(const Key &key) {
        lock_guard<mutex> lock(safe_op);
        auto value_it = key_finder.find(key);

        if (value_it == key_finder.end()) {
            throw std::range_error{key + "不在缓存队列中"};
        } else {
            lru_queue.splice(lru_queue.begin(), lru_queue, find(lru_queue.begin(), lru_queue.end(), key));
            return value_it->second;
        }
    }

    //把map中的数据写入数据库
    int writeToMysql(int cacheOrder) {
        MYSQL *mysql = nullptr;
        connectionRAII mysqlcon(&mysql, connPool);
        string statement = "TRUNCATE TABLE cache" + to_string(cacheOrder);
        mysql_query(mysql, statement.c_str());
        for (int i = 0; i < int(max_cache_size); i++) {
            try {
                auto k = *std::next(lru_queue.begin(), i);
                auto v = key_finder[k];
                statement = "INSERT INTO cache" + to_string(cacheOrder) + " VALUES (" + k + "," + v + ")";
                mysql_query(mysql, statement.c_str());
            }catch(exception &e){
                return 0;
            }
        }
        return 1;
    }

    //用来打印信息
    void GetElem(const Key &key) {
        try {
            cout << Get(key) << endl;
        } catch (exception &e) {
            cout << e.what() << endl;
        }
    }

protected:
    void Insert(const Key &key, const Value &value) {
        lru_queue.emplace_front(key);
        key_finder.emplace(make_pair(key, value));
    }

    void Erase() {
        auto last = lru_queue.back();
        lru_queue.pop_back();
        key_finder.erase(last);
    }

    void Update(const Key &key, const Value &value) {
        lru_queue.splice(lru_queue.begin(), lru_queue, find(lru_queue.begin(), lru_queue.end(), key));
        key_finder[key] = value;
    }

    void Clear() {
        lock_guard<mutex> lock(safe_op);
        lru_queue.clear();
        key_finder.clear();
    }

private:
    //双向链表用来存储lru key队列
    list <Key> lru_queue;
    //map容器用来存储key和value的对应关系
    unordered_map<Key, Value> key_finder;
    //锁
    mutex safe_op;
    //队列大小
    size_t max_cache_size;
    //一个cache拥有一个数据库连接池
    connection_pool *connPool;
};

#endif //DISTRIBUTED_CACHE_LRU_CACHE_H
