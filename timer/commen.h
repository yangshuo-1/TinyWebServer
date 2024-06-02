#ifndef COMMEN_H_
#define COMMEN_H_

#include <netinet/in.h>
#include <functional>

#define MAX_SKIPLIST_LEVEL  32
#define SKIPLIST_P 0.25

class Timer;

// 客户数据
struct ClientData{
    sockaddr_in address;        // 客户端地址信息
    int sock_fd;                // 客户端套接字描述符
    Timer *cliend_timer;        // 客户端对应的定时器
};

// 回调函数
using Handler_ptr = std::function<void(ClientData*)>;

// 定时器
class Timer{
public:
    Timer(){};
    time_t getExpire() {return expire;};

    Handler_ptr cd_func;               // 回调函数
private:
    time_t expire;              // 超时时间
    ClientData *user_data;      // 客户端数据
    
};

#endif