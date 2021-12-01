//
// Created by user on 2021/11/24.
//
#include <unistd.h>
#include "cstdlib"
#include <iostream>

using namespace std;

int main() {
    system("if [ -e ../log/client.log ] \n then \n rm -f ../log/client.log \n fi");
    system("if [ -e ../log/cache1.log ] \n then \n rm -f ../log/cache1.log \n fi");

    pid_t status;
    status= fork();
    if(status == -1)
    {
        printf("Create ChildProcess Errror!\n");
        exit(1);
    }
    else if(status == 0)
    {
        sleep(3);
        cout<<"正在打印client日志......"<<endl;
        system("./client 2>&1 | sed -u -r 's/\\x1B\\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g' | tee ../log/client.log");
    }
    else
    {
        cout<<"正在打印cache日志......"<<endl;
        system("./cache 2>&1 | sed -u -r 's/\\x1B\\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g' | tee ../log/cache1.log");
    }
}