//
// Created by Yuting on 2021/11/21.
//

#include <cassert>
#include <iostream>
#include "client.h"
#include <csignal>

using namespace std;

//cache存活情况
unordered_map<int,bool> live;

int writeToCache(client &c,unordered_map<string,int> &port_socket,string port,unordered_map<int,const char*> &socket_port,char data[8192]){

    const char* ip;
    int cache;

    for(auto it=cache_port.begin();it!=cache_port.end();it++){
        if(it->second==port){
            cache=it->first;
            ip=cache_ip[it->first];
        }
    }
    if(live[cache]){
        if(c.write_nbytes(port_socket[port],data,strlen(data),port)==-1){
            int socket = c.start_conn(ip, port.c_str());
            assert(socket >= 0);
            if(socket>0){
                socket_port[socket]=port.c_str();
                port_socket[port.c_str()]=socket;
                c.write_nbytes(port_socket[port.c_str()],data,strlen(data),port);
                return 1;
            }else{
                live[cache]= false;
                string GetK=data;
                if(split(GetK,":")[0]=="Put"){
                    ALERT("[客户端]",断开与master之间的连接);
                    abort();
                }
                MYSQL *mysql = nullptr;
                MYSQL_RES *res;
                connectionRAII mysqlcon(&mysql, c.connPool);

                string query="SELECT v FROM cache"+ to_string(cache)+" where k="+ split(GetK,":")[1];
                mysql_query(mysql, query.c_str());

                res = mysql_store_result(mysql);
                int flag=0;

                int num_fields = mysql_num_fields(res);
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)))
                {
                    for(int i = 0; i < num_fields; i++)
                    {
                        if(row[i] != NULL){
                            ALERT("[客户端]",cache%d已故障%c从mysql数据库查得%s元素值为%s, cache,',',split(GetK,":")[1].c_str(),row[i]);
                            flag=1;
                        }
                    }
                }
                if(res != NULL){
                    mysql_free_result(res);
                }
                if(flag==0){
                    ALERT("[客户端]",cache%d已故障%c查询mysql数据库%c未找到元素%s,cache,',',',',split(GetK,":")[1].c_str());
                }
                mysql_close(mysql);
                return 0;
            }
        }
    }else{
        live[cache]= false;
        string GetK=data;
        if(split(GetK,":")[0]=="Put"){
            ALERT("[客户端]",断开与master之间的连接);
            abort();
        }
        MYSQL *mysql = nullptr;
        MYSQL_RES *res;
        connectionRAII mysqlcon(&mysql, c.connPool);
        string query="SELECT v FROM cache"+ to_string(cache)+" where k="+ split(GetK,":")[1];
        mysql_query(mysql, query.c_str());

        res = mysql_store_result(mysql);
        int flag=0;

        int num_fields = mysql_num_fields(res);
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)))
        {
            for(int i = 0; i < num_fields; i++)
            {
                if(row[i] != NULL){
                    ALERT("[客户端]",cache%d已故障%c从mysql数据库查得%s元素值为%s, cache,',',split(GetK,":")[1].c_str(),row[i]);
                    flag=1;
                }
            }
        }
        if(res != NULL){
            mysql_free_result(res);
        }
        if(flag==0){
            ALERT("[客户端]",cache%d已故障%c查询mysql数据库%c未找到元素%s,cache,',',',',split(GetK,":")[1].c_str());
        }
        mysql_close(mysql);
        return 0;
    }
    return 1;
}

int main(int argc,char *argv[]) {

    for(auto it=cache_port.begin();it!=cache_port.end();it++){
        live[it->first]=true;
    }
    client c;
    //本地缓存有效标志位
    bool flag= false;

    char data[8192];
    char buf[8192];
    string argv1;
    if(argc==2){
        argv1.append(argv[1]);
    }
    assert(argc == 1 || (argc == 2 && argv1 == "fill"));
    unordered_map<int,const char*> socket_port;
    unordered_map<string,int> port_socket;

    for (int i = 0; i <= client_cache_Num; i++) {
        int socket = c.start_conn(cache_ip[i], cache_port[i]);
        assert(socket > 0);
        socket_port.insert(make_pair(socket, cache_port[i]));
        port_socket.insert(make_pair(cache_port[i], socket));

    }

    if(argc==2 && argv1=="fill"){
        while (true) {
            msleep(interval_data);
            string k_v = c.k_v_gen(keylen, valuelen);
            string mess = c.message(k_v);
            if(flag && (split(mess,":")[0]=="Get")){
                WARNING("[客户端]",缓存有效读请求);
                try{
                    string port=c.dbClient[split(mess,":")[1]];
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int res = writeToCache(c,port_socket,port,socket_port,data);
                    //c.write_nbytes(port_socket[port],data,strlen(data),port);
                    if(res==1){
                        while(1){
                            int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                            if(ret>0){
                                break;
                            }else if(ret==0){
                                break;
                            }
                        }
                    }
                }catch(exception &e){
                    SUCCESS("[客户端]", %s元素不在cache中, split(mess,":")[1].c_str());
                }
            }else if(!flag && (split(mess,":")[0]=="Get")){
                WARNING("[客户端]",缓存无效读请求%c向master请求新的数据分布,',');
                c.pull_db();
                flag=true;
                try{
                    string port=c.dbClient[split(mess,":")[1]];
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int res = writeToCache(c,port_socket,port,socket_port,data);
                    //c.write_nbytes(port_socket[port],data,strlen(data),port);
                    if(res==1){
                        while(1){
                            int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                            if(ret>0){
                                break;
                            }else if(ret==0){
                                break;
                            }
                        }
                    }
                }catch(exception &e){
                    SUCCESS("[客户端]", %s元素不在cache中, split(mess,":")[1].c_str());
                }
            }else if(flag && (split(mess,":")[0]=="Put")){
                WARNING("[客户端]",缓存有效写请求);
                try{
                    string port=c.dbClient[split(split(mess,":")[1],",")[0]];
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int res = writeToCache(c,port_socket,port,socket_port,data);
                    //c.write_nbytes(port_socket[port],data,strlen(data),port);
                    if(res==1){
                        while(1){
                            int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                            if(ret>0){
                                break;
                            }else if(ret==0){
                                break;
                            }
                        }
                    }
                }catch (exception &e){
                    flag= false;
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int write_master=c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        if(write_master==-1){
                            ALERT("[client]",正在重连master%s,"......");
                            sleep(clientmasterTime);
                            int socket_=c.start_conn(cache_ip[0],cache_port[0]);
                            if(socket_==0){
                                continue;
                            }
                            port_socket[cache_port[0]]=socket_;
                            write_master=c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                        }else{
                            break;
                        }
                    }
                    //c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        int ret=c.read_once(port_socket[cache_port[0]], buf, sizeof(buf),cache_port[0]);
                        if(ret>0){
                            buf[ret]='\0';
                            string port=buf;
                            int res = writeToCache(c,port_socket,port,socket_port,data);
                            //c.write_nbytes(port_socket[port],data,strlen(data),port);
                            if(res==1){
                                while(1){
                                    int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                                    if(ret>0){
                                        break;
                                    }else if(ret==0){
                                        break;
                                    }
                                }
                            }
                            break;
                        }else if(ret==0){
                            break;
                        }
                    }
                }
            }else{
                WARNING("[客户端]",缓存无效写请求%c向master请求新的数据分布,',');
                c.pull_db();
                flag=true;
                try{
                    string port=c.dbClient[split(split(mess,":")[1],",")[0]];
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int res = writeToCache(c,port_socket,port,socket_port,data);
                    //c.write_nbytes(port_socket[port],data,strlen(data),port);
                    if(res==1){
                        while(1){
                            int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                            if(ret>0){
                                break;
                            }else if(ret==0){
                                break;
                            }
                        }
                    }
                }catch (exception &e){
                    flag= false;
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int write_master=c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        if(write_master==-1){
                            ALERT("[client]",正在重连master%s,"......");
                            sleep(clientmasterTime);
                            int socket_=c.start_conn(cache_ip[0],cache_port[0]);
                            if(socket_==0){
                                continue;
                            }
                            port_socket[cache_port[0]]=socket_;
                            write_master=c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                        }else{
                            break;
                        }
                    }
                    //c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        int ret=c.read_once(port_socket[cache_port[0]], buf, sizeof(buf),cache_port[0]);
                        if(ret>0){
                            buf[ret]='\0';
                            string port=buf;
                            int res = writeToCache(c,port_socket,port,socket_port,data);
                            //c.write_nbytes(port_socket[port],data,strlen(data),port);
                            if(res==1){
                                while(1){
                                    int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                                    if(ret>0){
                                        break;
                                    }else if(ret==0){
                                        break;
                                    }
                                }
                            }
                            break;
                        }else if(ret==0){
                            break;
                        }
                    }
                }
            }
        }
    }else if(argc==1){
        string mess;
        while(1){
            cout<<"SDC>";
            cin>>mess;
            if(flag && (split(mess,":")[0]=="Get")){
                WARNING("[客户端]",缓存有效读请求);
                try{
                    string port=c.dbClient[split(mess,":")[1]];
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int res = writeToCache(c,port_socket,port,socket_port,data);
                    //c.write_nbytes(port_socket[port],data,strlen(data),port);
                    if(res==1){
                        while(1){
                            int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                            if(ret>0){
                                break;
                            }else if(ret==0){
                                break;
                            }
                        }
                    }
                }catch(exception &e){
                    SUCCESS("[客户端]", %s元素不在cache中, split(mess,":")[1].c_str());
                }
            }else if(!flag && (split(mess,":")[0]=="Get")){
                WARNING("[客户端]",缓存无效读请求%c向master请求新的数据分布,',');
                c.pull_db();
                flag=true;
                try{
                    string port=c.dbClient[split(mess,":")[1]];
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int res = writeToCache(c,port_socket,port,socket_port,data);
                    //c.write_nbytes(port_socket[port],data,strlen(data),port);
                    if(res==1){
                        while(1){
                            int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                            if(ret>0){
                                break;
                            }else if(ret==0){
                                break;
                            }
                        }
                    }
                }catch(exception &e){
                    SUCCESS("[客户端]", %s元素不在cache中, split(mess,":")[1].c_str());
                }
            }else if(flag && (split(mess,":")[0]=="Put")){
                WARNING("[客户端]",缓存有效写请求);
                try{
                    string port=c.dbClient[split(split(mess,":")[1],",")[0]];
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int res = writeToCache(c,port_socket,port,socket_port,data);
                    //c.write_nbytes(port_socket[port],data,strlen(data),port);
                    if(res==1){
                        while(1){
                            int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                            if(ret>0){
                                break;
                            }else if(ret==0){
                                break;
                            }
                        }
                    }
                }catch (exception &e){
                    flag= false;
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int write_master=c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        if(write_master==-1){
                            ALERT("[client]",正在重连master%s,"......");
                            sleep(clientmasterTime);
                            int socket_=c.start_conn(cache_ip[0],cache_port[0]);
                            if(socket_==0){
                                continue;
                            }
                            port_socket[cache_port[0]]=socket_;
                            write_master=c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                        }else{
                            break;
                        }
                    }
                    //c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        int ret=c.read_once(port_socket[cache_port[0]], buf, sizeof(buf),cache_port[0]);
                        if(ret>0){
                            buf[ret]='\0';
                            string port=buf;
                            int res = writeToCache(c,port_socket,port,socket_port,data);
                            //c.write_nbytes(port_socket[port],data,strlen(data),port);
                            if(res==1){
                                while(1){
                                    int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                                    if(ret>0){
                                        break;
                                    }else if(ret==0){
                                        break;
                                    }
                                }
                            }
                            break;
                        }else if(ret==0){
                            break;
                        }
                    }
                }
            }else{
                WARNING("[客户端]",缓存无效写请求%c向master请求新的数据分布,',');
                c.pull_db();
                flag=true;
                try{
                    string port=c.dbClient[split(split(mess,":")[1],",")[0]];
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int res = writeToCache(c,port_socket,port,socket_port,data);
                    //c.write_nbytes(port_socket[port],data,strlen(data),port);
                    if(res==1){
                        while(1){
                            int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                            if(ret>0){
                                break;
                            }else if(ret==0){
                                break;
                            }
                        }
                    }
                }catch (exception &e){
                    flag= false;
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    int write_master=c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        if(write_master==-1){
                            ALERT("[client]",正在重连master%s,"......");
                            sleep(clientmasterTime);
                            int socket_=c.start_conn(cache_ip[0],cache_port[0]);
                            if(socket_==0){
                                continue;
                            }
                            port_socket[cache_port[0]]=socket_;
                            write_master=c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                        }else{
                            break;
                        }
                    }
                    //c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        int ret=c.read_once(port_socket[cache_port[0]], buf, sizeof(buf),cache_port[0]);
                        if(ret>0){
                            buf[ret]='\0';
                            string port=buf;
                            int res = writeToCache(c,port_socket,port,socket_port,data);
                            //c.write_nbytes(port_socket[port],data,strlen(data),port);
                            if(res==1){
                                while(1){
                                    int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                                    if(ret>0){
                                        break;
                                    }else if(ret==0){
                                        break;
                                    }
                                }
                            }
                            break;
                        }else if(ret==0){
                            break;
                        }
                    }
                }
            }
        }
    }
}

