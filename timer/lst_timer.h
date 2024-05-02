/*
由于非活跃连接占用了连接资源，严重影响服务器的性能，通过实现一个服务器定时器，处理这种非活跃连接，释放连接资源。
利用alarm函数周期性地触发SIGALRM信号,该信号的信号处理函数利用管道通知主循环执行定时器链表上的定时任务.

统一事件源
基于升序链表的定时器
处理非活动连接


*/


#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"

class util_timer;
// 客户端信息 
struct client_data
{
    sockaddr_in address;                                // 客户端的地址信息 
    int sockfd;                                         // 客户端的套接字描述符
    util_timer *timer;                                  // 指向关联的定时器对象
};
// 定时器 
class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire;                                      // 定时器的超时时间 
    
    void (* cb_func)(client_data *);                    // 定时器超时时调用的回调函数指针 
    client_data *user_data;                             // 指向包含客户端信息的client_data结构体
    util_timer *prev;                                   // 定时器排序链表中的前一个
    util_timer *next;                                   // 定时器排序链表中的下一个
};
// 排序定时器链表 
class sort_timer_lst
{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer);                  // 添加一个定时器 
    void adjust_timer(util_timer *timer);               // 调整链表中某个定时器的时间 
    void del_timer(util_timer *timer);                  // 删除一个定时器 
    void tick();                                        // 定时器的滴答函数，用于检查和处理到期的定时器

private:
    void add_timer(util_timer *timer, util_timer *lst_head);    // 将定时器插入到合适的位置 

    util_timer *head;
    util_timer *tail;
};

// 提供了一系列实用工具函数，用于管理非阻塞套接字、注册 epoll 事件、处理信号和定时器任务
class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;                           // 管道文件描述符，与WebServer一致
    sort_timer_lst m_timer_lst;                     // 定时器排序链表
    static int u_epollfd;                           // epoll文件描述符。与WebServer一致
    int m_TIMESLOT;                                 // 最小超时单位 
};

// 回调函数？为啥定义在类外，类内用函数指针调用？
void cb_func(client_data *user_data);

#endif
