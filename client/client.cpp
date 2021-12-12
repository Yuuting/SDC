//
// Created by user on 2021/11/21.
//

#include <cassert>
#include <iostream>
#include "client.h"

using namespace std;

int main(int argc,char *argv[]) {
    client c;

    char data[8192];
    char buf[8192];
    assert(argc==1||argc==4);
    if(argc==1){
        int c_socket1 = c.start_conn(cache1_ip, cache1_port);
        assert(c_socket1>0);
        int c_socket2 = c.start_conn(cache2_ip, cache2_port);
        assert(c_socket2>0);
        int c_socket3 = c.start_conn(cache3_ip, cache3_port);
        assert(c_socket3>0);
        int master_sock=c.start_conn(master_ip,master_port);
        assert(master_sock>0);

        unordered_map<int,const char*> socket_port={
                {c_socket1,cache1_port},
                {c_socket2,cache2_port},
                {c_socket3,cache3_port},
                {master_sock,master_port}
        };

        unordered_map<string,int> port_socket={
                {cache1_port,c_socket1},
                {cache2_port,c_socket2},
                {cache3_port,c_socket3},
                {master_port,master_sock}
        };

        while (true) {
            msleep(interval_data);
            string k_v = c.k_v_gen(keylen, valuelen);
            string mess = c.message(k_v);
            memset(data, '\0', sizeof(data));
            strcpy(data, mess.c_str());
            c.write_nbytes(master_sock, data, strlen(data),socket_port[master_sock]);
            while(1){
                int ret=c.read_once(master_sock, buf, sizeof(buf),socket_port[master_sock]);
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
    }else if(argc==4){
        int socket;
        unordered_map<int,const char*> port_socket;
        if(atoi(argv[2])==1){
            socket=c.start_conn(cache1_ip, cache1_port);
            port_socket.insert(make_pair(socket,cache1_port));
        }else if(atoi(argv[2])==2){
            socket=c.start_conn(cache2_ip, cache2_port);
            port_socket.insert(make_pair(socket,cache2_port));
        }else if(atoi(argv[2])==3){
            socket=c.start_conn(cache3_ip, cache3_port);
            port_socket.insert(make_pair(socket,cache3_port));
        }
        assert(socket>0);
        memset(data, '\0', sizeof(data));
        string sendmessage;
        sendmessage.append(argv[1]);
        sendmessage.append(":");
        sendmessage.append(argv[3]);
        strcpy(data,sendmessage.c_str());
        c.write_nbytes(socket, data, strlen(data),port_socket[socket]);
        while(1){
            int ret=c.read_once(socket, buf, sizeof(buf),port_socket[socket]);
            if(ret>0){
                break;
            }else if(ret==0){
                break;
            }
        }
    }
}

