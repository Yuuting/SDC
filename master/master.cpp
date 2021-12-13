//
// Created by user on 2021/12/11.
//
#include "master.h"
int main(){
    system("if [ -e ../master/dbMaster.json ] \n then \n rm -f ../master/dbMaster.json && touch ../master/dbMaster.json \n fi");
    master m;
    m.recvHeartBeat(master_port,backlog);
}