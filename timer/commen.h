#ifndef COMMEN_H_
#define COMMEN_H_

#include <netinet/in.h>
#include <functional>

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

private:
    time_t expire;              // 超时时间
    ClientData *user_data;      // 客户端数据
    Handler_ptr cd_func;               // 回调函数
};

#endif