//
// Created by Yuting on 2021/11/28.
//

#ifndef DISTRIBUTED_CACHE_LOCKER_H
#define DISTRIBUTED_CACHE_LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem
{
public:
    //POSIX提供的信号量原语，为了获取共享资源，进程还需要进行以下的操作。
    /*
     * 测试控制该资源的信号量
     * 若此信号量的值为正，则进程可以使用该资源。在这种情况下，进程会将信号量值减1，表示它使用了一个资源单位。
     * 否则，若此信号量的值为0，则进程进入休眠状态，直至信号量的值大于0.进程被唤醒后，它返回步骤1.
     */
    sem()
    {
        //sem_init函数用于创建信号量，初始化由sem指向的信号对象，并给它一个初始的整数值value
        //pshared控制信号量的类型，值为0代表该信号量用于多线程间的同步，值如果大于0表示可以共享，用于多个相关进程间的同步
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }

    bool wait()
    {   //阻塞函数，测试所指定信号量的值，它的操作是原子的，若sem value大于0，则该信号量值减去1并立即返回。
        //若sem value=0，则阻塞直到sem value>0,此时立即减去1，然后返回。
        return sem_wait(&m_sem) == 0;
    }
    bool post()
    {   //把指定的信号量sem的值加1，唤醒正在等待该信号量的任意线程
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};
class locker
{
public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

class cond
{
public:
    cond()
    {//创建=0创建完成
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond()
    {//析构destroy
        pthread_cond_destroy(&m_cond);
    }
    bool wait(pthread_mutex_t *m_mutex)
    {//等待条件变量满足，把获得的锁释放掉 成功返回0
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_mutex);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    //唤醒至少一个线程
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    //唤醒等待该条件满足的所有进程
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    //static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

#endif //DISTRIBUTED_CACHE_LOCKER_H
