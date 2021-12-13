//
// Created by user on 2021/11/21.
//

#include <cassert>
#include <iostream>
#include "client.h"

using namespace std;

int main(int argc,char *argv[]) {
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
                    c.write_nbytes(port_socket[port],data,strlen(data),port);
                    while(1){
                        int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                        if(ret>0){
                            break;
                        }else if(ret==0){
                            break;
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
                    c.write_nbytes(port_socket[port],data,strlen(data),port);
                    while(1){
                        int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                        if(ret>0){
                            break;
                        }else if(ret==0){
                            break;
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
                    c.write_nbytes(port_socket[port],data,strlen(data),port);
                    while(1){
                        int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                        if(ret>0){
                            break;
                        }else if(ret==0){
                            break;
                        }
                    }
                }catch (exception &e){
                    flag= false;
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        int ret=c.read_once(port_socket[cache_port[0]], buf, sizeof(buf),cache_port[0]);
                        if(ret>0){
                            buf[ret]='\0';
                            string port=buf;
                            c.write_nbytes(port_socket[port],data,strlen(data),port);
                            while(1){
                                int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                                if(ret>0){
                                    break;
                                }else if(ret==0){
                                    break;
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
                    c.write_nbytes(port_socket[port],data,strlen(data),port);
                    while(1){
                        int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                        if(ret>0){
                            break;
                        }else if(ret==0){
                            break;
                        }
                    }
                }catch (exception &e){
                    flag= false;
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        int ret=c.read_once(port_socket[cache_port[0]], buf, sizeof(buf),cache_port[0]);
                        if(ret>0){
                            buf[ret]='\0';
                            string port=buf;
                            c.write_nbytes(port_socket[port],data,strlen(data),port);
                            while(1){
                                int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                                if(ret>0){
                                    break;
                                }else if(ret==0){
                                    break;
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
                    c.write_nbytes(port_socket[port],data,strlen(data),port);
                    while(1){
                        int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                        if(ret>0){
                            break;
                        }else if(ret==0){
                            break;
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
                    c.write_nbytes(port_socket[port],data,strlen(data),port);
                    while(1){
                        int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                        if(ret>0){
                            break;
                        }else if(ret==0){
                            break;
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
                    c.write_nbytes(port_socket[port],data,strlen(data),port);
                    while(1){
                        int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                        if(ret>0){
                            break;
                        }else if(ret==0){
                            break;
                        }
                    }
                }catch (exception &e){
                    flag= false;
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        int ret=c.read_once(port_socket[cache_port[0]], buf, sizeof(buf),cache_port[0]);
                        if(ret>0){
                            buf[ret]='\0';
                            string port=buf;
                            c.write_nbytes(port_socket[port],data,strlen(data),port);
                            while(1){
                                int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                                if(ret>0){
                                    break;
                                }else if(ret==0){
                                    break;
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
                    c.write_nbytes(port_socket[port],data,strlen(data),port);
                    while(1){
                        int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                        if(ret>0){
                            break;
                        }else if(ret==0){
                            break;
                        }
                    }
                }catch (exception &e){
                    flag= false;
                    memset(data, '\0', sizeof(data));
                    strcpy(data, mess.c_str());
                    c.write_nbytes(port_socket[cache_port[0]], data, strlen(data),cache_port[0]);
                    while(1){
                        int ret=c.read_once(port_socket[cache_port[0]], buf, sizeof(buf),cache_port[0]);
                        if(ret>0){
                            buf[ret]='\0';
                            string port=buf;
                            c.write_nbytes(port_socket[port],data,strlen(data),port);
                            while(1){
                                int ret=c.read_once(port_socket[port], buf, sizeof(buf),port);
                                if(ret>0){
                                    break;
                                }else if(ret==0){
                                    break;
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

