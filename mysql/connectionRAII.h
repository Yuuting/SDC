//
// Created by user on 2021/11/28.
//
#include "sqlconn.h"
#ifndef DISTRIBUTED_CACHE_CONNECTIONRAII_H
#define DISTRIBUTED_CACHE_CONNECTIONRAII_H

class connectionRAII{

public:
    connectionRAII(MYSQL **con, connection_pool *connPool);
    ~connectionRAII();

private:
    MYSQL *conRAII;
    connection_pool *poolRAII;
};


connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool){
    *SQL = connPool->GetConnection();

    conRAII = *SQL;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
    poolRAII->ReleaseConnection(conRAII);
}

#endif //DISTRIBUTED_CACHE_CONNECTIONRAII_H
