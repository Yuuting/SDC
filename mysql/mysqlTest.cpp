//
// Created by user on 2021/11/28.
//
#include "connectionRAII.h"
#include "sqlconn.h"

int main(){
    connection_pool *connPool=connection_pool::GetInstance();
    connPool->init("localhost","root","Fe747698!","yourdb",3306,5);

    MYSQL *mysql = NULL;
    MYSQL_RES *res;
    connectionRAII mysqlcon(&mysql, connPool);
    mysql_query(mysql, "SELECT passwd FROM user where username=123");
    res = mysql_store_result(mysql);

    int num_fields = mysql_num_fields(res);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        for(int i = 0; i < num_fields; i++)
        {
            if(row[i] != NULL)
                cout << row[i] << endl;
            else
                cout << "NULL" << endl;
        }
    }
    if(res != NULL)
        mysql_free_result(res);

    mysql_close(mysql);
}