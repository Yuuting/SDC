//
// Created by Yuting on 2021/11/24.
//
#include <unistd.h>
#include "cstdlib"
#include <iostream>

using namespace std;

int main() {
    system("if [ -e ../log/client.log ] \n then \n rm -f ../log/client.log \n fi");
    system("if [ -e ../log/cache1.log ] \n then \n rm -f ../log/cache1.log \n fi");
    system("if [ -e ../log/cache2.log ] \n then \n rm -f ../log/cache2.log \n fi");
    system("if [ -e ../log/cache3.log ] \n then \n rm -f ../log/cache3.log \n fi");
    system("if [ -e ../log/master.log ] \n then \n rm -f ../log/master.log \n fi");

    pid_t status, status1, status2, status3;
    status = fork();
    if (status == -1) {
        printf("Create ChildProcess Error!\n");
        exit(1);
    } else if (status == 0) {

        status1 = fork();
        if (status1 == -1) {
            printf("Create ChildProcess Error!\n");
            exit(1);
        } else if (status1 == 0) {
            status2 = fork();
            if (status2 == -1) {
                printf("Create ChildProcess Error!\n");
                exit(1);
            } else if (status2 == 0) {
                cout << "正在打印master日志......" << endl;
                system("./master 2>&1 | sed -u -r 's/\\x1B\\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g' | tee ../log/master.log");
            } else {
                sleep(1);
                cout << "正在打印cache1日志......" << endl;
                system("./cache 1 2>&1 | sed -u -r 's/\\x1B\\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g' | tee ../log/cache1.log");
            }
        } else {
            sleep(2);
            cout << "正在打印cache2日志......" << endl;
            system("./cache 2 2>&1 | sed -u -r 's/\\x1B\\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g' | tee ../log/cache2.log");
        }

    } else {
        status3 = fork();
        if (status3 == -1) {
            printf("Create ChildProcess Error!\n");
            exit(1);
        } else if (status3 == 0) {
            sleep(3);
            cout << "正在打印cache3日志......" << endl;
            system("./cache 3 2>&1 | sed -u -r 's/\\x1B\\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g' | tee ../log/cache3.log");
        } else {
            sleep(4);
            cout << "正在打印client日志......" << endl;
            system("./client fill 2>&1 | sed -u -r 's/\\x1B\\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g' | tee ../log/client.log");
        }

    }
}