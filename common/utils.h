//
// Created by user on 2021/11/22.
//

#ifndef DISTRIBUTED_CACHE_UTILS_H
#define DISTRIBUTED_CACHE_UTILS_H

#include <iostream>
#include <vector>
#include <ctime>
#include <cerrno>

using namespace std;


/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec) {
    struct timespec ts;
    int res;

    if (msec < 0) {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

//分隔字符串
vector<string> split(const string &str, const string &delim) {
    vector<string> res;
    if ("" == str) return res;
    //先将要切割的字符串从string类型转换为char*类型
    char *strs = new char[str.length() + 1]; //不要忘了
    strcpy(strs, str.c_str());

    char *d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char *p = strtok(strs, d);
    while (p) {
        string s = p; //分割得到的字符串转换为string类型
        res.push_back(s); //存入结果数组
        p = strtok(NULL, d);
    }

    return res;

}

//工具函数，设置一个文件描述符为非阻塞
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd)//添加事件
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void Close(int fd) {
    int rc;

    if ((rc = close(fd)) < 0)
        perror("Close error");
}

void removefd(int epollfd, int fd)//删除事件
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    Close(fd);
}
//判断字符串s1是否以s2开头
bool starts_with(const string& s1, const string& s2) {
    return s2.size() <= s1.size() && s1.compare(0, s2.size(), s2) == 0;
}
#endif //DISTRIBUTED_CACHE_UTILS_H
