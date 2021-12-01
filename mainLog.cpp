//
// Created by user on 2021/11/25.
//
#include <cstdlib>
#include <iostream>
using namespace std;
int main(){
    cout<<"正在打印主日志......"<<endl;
    system("if [ -e ../log/mainLog.log ] \n then \n rm -f ../log/mainLog.log \n fi");
    system("./partLog > ../log/mainLog.log 2>&1");
}