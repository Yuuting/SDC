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

        unordered_map<int,const char*> port_socket={
                {c_socket1,cache1_port},
                {c_socket2,cache2_port},
                {c_socket3,cache3_port}
        };

        while (true) {
            msleep(interval_data);
            string k_v = c.k_v_gen(keylen, valuelen);
            string mess = c.message(k_v);
            memset(data, '\0', sizeof(data));
            strcpy(data, mess.c_str());
            int socket = c.choose(c_socket1, c_socket2, c_socket3);
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

