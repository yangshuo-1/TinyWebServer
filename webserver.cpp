#include "webserver.h"

WebServer::WebServer()
{
    //http_conn类对象
    users = new http_conn[MAX_FD];

    //root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);

    //定时器
    users_timer = new client_data[MAX_FD];
}

WebServer::~WebServer()
{
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
}

void WebServer::init(int port, string user, string passWord, string databaseName, int log_write, 
                     int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model)
{
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}

void WebServer::trig_mode()
{
    //LT + LT
    if (0 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }
    //LT + ET
    else if (1 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }
    //ET + LT
    else if (2 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }
    //ET + ET
    else if (3 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }
}

void WebServer::log_write()
{
    if (0 == m_close_log)
    {
        //初始化日志
        if (1 == m_log_write)
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 800);
        else
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 0);
    }
}

// 数据库连接池初始化 
void WebServer::sql_pool()
{
    //初始化数据库连接池
    m_connPool = connection_pool::GetInstance();
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num, m_close_log);

    //初始化数据库读取表
    users->initmysql_result(m_connPool);
}
// 线程池初始化 
void WebServer::thread_pool()
{
    //线程池
    m_pool = new threadpool<http_conn>(m_actormodel, m_connPool, m_thread_num);
}

// 初始化一个监听套接字 
void WebServer::eventListen()
{
    //网络编程基础步骤
    // 创建监听套接字，第一个参数：使用IPv4地址，第二个：使用TCP流套接字 
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    // TCP连接断开的时候调用closesocket函数，有优雅断开和强制断开两种方式 

    //优雅关闭连接 ？？？ 
    if (0 == m_OPT_LINGER)
    {
        // 底层会将未发送完成的数据发送完成后再释放资源 优雅退出
        struct linger tmp = {0, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    else if (1 == m_OPT_LINGER)
    {
        //这种方式下，在调用closesocket的时候不会立刻返回，内核会延迟一段时间，这个时间就由l_linger得值来决定。
        //如果超时时间到达之前，发送完未发送的数据(包括FIN包)并得到另一端的确认，closesocket会返回正确，socket描述符优雅性退出。
        //否则，closesocket会直接返回 错误值，未发送数据丢失，socket描述符被强制性退出。需要注意的时，如果socket描述符被设置为非堵塞型，则closesocket会直接返回值。
        struct linger tmp = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    int ret = 0;
    // 创建地址结构体，用于存储套接字的地址信息 
    struct sockaddr_in address;
    bzero(&address, sizeof(address));   // 清零 
    // 设置地址族、IP地址和端口号
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    // 允许重用本地地址和端口 
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    // 将套接字绑定到套接字地址和端口上 
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    // 调用listen函数使套接字开始监听连接，第二个参数制定了套接字的监听队列长度 
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    utils.init(TIMESLOT);

    //epoll创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    // 创建一个新的 epoll 实例，这个实例可以用来监听多个文件描述符上的事件。
    // size 参数指定了 epoll 实例能够监听的文件描述符的数量
    // 返回值是一个 int 类型的文件描述符，这个文件描述符代表了新创建的 epoll 实例。
    // 如果函数调用成功，返回的文件描述符是一个非负数；如果调用失败，则返回 -1，并且可以通过 errno 变量来获取错误信息。
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    // 将套接字添加到epoll时间表 
    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);
    http_conn::m_epollfd = m_epollfd;       // qqqq这是啥？传递给了httpcon类的静态成员变量，为什么要传递？
                                            // ---把epoll实例文件描述符赋值给http_conn类中的静态变量， 后续直接在epoll实例里对实例进行操作，比如添加监听的文件描述符 


    /*
        具体的做法是，信号处理函数使用管道将信号传递给主循环，信号处理函数往管道的写端写入信号值，
        主循环则从管道的读端读出信号值，使用I/O复用系统调用来监听管道读端的可读事件，
        这样信号事件与其他文件描述符都可以通过epoll来监测，从而实现统一处理。
    */

    // 创建一对Unix域套接字。这对套接字通常用于进程间通信（IPC）。
    // m_pipefd是一个数组，存储了两个新的文件描述符。
    // m_pipefd[0]是读端，m_pipefd[1]是写端
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    
    // 将写端设置为非阻塞，qqqqq为什么呢？

    // 管道文件为阻塞读和阻塞写的时候，如果先读，陷入阻塞，等待写操作；如果先写，陷入阻塞，等待读操作
    // send是将信息发送给套接字缓冲区，如果缓冲区满了，则会阻塞，
    // 这时候会进一步增加信号处理函数的执行时间，为此，将其修改为非阻塞。
    // 若管道满了会立即返回非零值
    utils.setnonblocking(m_pipefd[1]);

    //设置管道读端挂上树，监听读事件
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);
    //将SIGPIPE信号处理为SIG_IGN，忽略SIGPIPE信号 
    utils.addsig(SIGPIPE, SIG_IGN);

    // 传递给主循环的信号值，这里只关注SIGALRM（alarm函数）和SIGTERM（程序异常终止）
    // 设置这两种信号的处理函数为sig_handler 
    utils.addsig(SIGALRM, utils.sig_handler, false);
    utils.addsig(SIGTERM, utils.sig_handler, false);

    // 每隔TIMESLOT时间触发SIGALRM信号
    alarm(TIMESLOT);

    //工具类,信号和描述符基础操作
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

void WebServer::timer(int connfd, struct sockaddr_in client_address)
{
    users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_close_log, m_user, m_passWord, m_databaseName);

    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    users_timer[connfd].timer = timer;
    utils.m_timer_lst.add_timer(timer);
}

//若有数据传输，则将定时器往后延迟3个TIMESLOT
//并对新的定时器在链表上的位置进行调整
void WebServer::adjust_timer(util_timer *timer)
{
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);

    LOG_INFO("%s", "adjust timer once");
}
// 坏连接的处理
void WebServer::deal_timer(util_timer *timer, int sockfd)
{
    timer->cb_func(&users_timer[sockfd]);
    if (timer)
    {
        utils.m_timer_lst.del_timer(timer);
    }

    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}

// 处理客户连接 
bool WebServer::dealclientdata()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    // LT
    if (0 == m_LISTENTrigmode)
    {
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        if (connfd < 0)
        {
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if (http_conn::m_user_count >= MAX_FD)
        {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connfd, client_address);
    }
    // ET
    else
    {
        while (1)
        {
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            if (connfd < 0)
            {
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (http_conn::m_user_count >= MAX_FD)
            {
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}
// signal的处理函数，它不需要上队列。这里是通过管道的方式来告知WebServer。管道由epoll监控
// timeout：是否发生超时；stop_server：是否停止服务器 
bool WebServer::dealwithsignal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    int sig;
    char signals[1024];


    //从管道读端读出信号值，成功返回字节数，失败返回-1
    //正常情况下，这里的ret返回值总是1，只有14和15两个ASCII码对应的字符
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1)
    {
        return false;
    }
    else if (ret == 0)
    {
        return false;
    }
    else
    {
        // 根据信号值，有不同的处理逻辑 
        for (int i = 0; i < ret; ++i)
        {
            /*
            对于接收到的每个信号字符，使用 switch 语句根据其值执行不同的操作。
            如果信号值是 SIGALRM（通常用于定时器信号），将 timeout 设置为 true。
            如果信号值是 SIGTERM（用于终止程序的信号），将 stop_server 设置为 true。
            */
            switch (signals[i])
            {
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}
/*
处理从客户端套接字读取数据的事件。

存在两个模式的切换：reactor与preactor（同步io模拟出的）

区别是对于数据的读取者是谁，对于reactor是同步线程来完成，整个读就绪放在请求列表上；
而对于preactor则是由主线程，也就是当前的WebServer进行一次调用，读取后将读完成纳入请求队列上。
*/
void WebServer::dealwithread(int sockfd)
{
    // 获取sockfd关联的计时器 
    util_timer *timer = users_timer[sockfd].timer;

    //reactor 反应器
    if (1 == m_actormodel)
    {
        // 如果存在定时器 
        if (timer)
        {
            adjust_timer(timer);
        }

        // 若监测到读事件，将该事件放入请求队列
        // qqqqq，users+sockfd对应的http_conn放入线程池 
        m_pool->append(users + sockfd, 0);

        while (true)
        {
            // 查看是否有改进标志 
            if (1 == users[sockfd].improv)
            {
                // 此时两个同时为1，说明读or写出错了
                // 计时器事件发生，处理计时器事件 
                if (1 == users[sockfd].timer_flag)
                {
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else
    {
        // proactor 先行器
        if (users[sockfd].read_once())
        {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            //若监测到读事件，将该事件放入请求队列
            m_pool->append_p(users + sockfd);

            if (timer)
            {
                adjust_timer(timer);
            }
        }
        // 如果读取失败，处理计时器事件
        else
        {
            deal_timer(timer, sockfd);
        }
    }
}

// 处理套接字写事件 
void WebServer::dealwithwrite(int sockfd)
{
    // 获取计时器 
    util_timer *timer = users_timer[sockfd].timer;
    //reactor
    if (1 == m_actormodel)
    {
        if (timer)
        {
            adjust_timer(timer);
        }

        m_pool->append(users + sockfd, 1);

        while (true)
        {
            if (1 == users[sockfd].improv)
            {
                if (1 == users[sockfd].timer_flag)
                {
                    // 处理坏连接 
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else
    {
        //proactor
        if (users[sockfd].write())
        {
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            if (timer)
            {
                adjust_timer(timer);
            }
        }
        else
        {
            deal_timer(timer, sockfd);
        }
    }
}

/*
    只要程序不关，就一直不退出

*/
void WebServer::eventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server)
    {
        // 监测发生时间的文件描述符 ，如果没有事件发生会一直等着 
        // 如果成功，返回发生的事件数量，即填充到events数组的epoll_event结构体的数量 
        // 如果错误，返回-1 
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        // 轮询有时间产生的文件描述符 
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;

            // 处理新到的客户连接
            if (sockfd == m_listenfd)
            {
                bool flag = dealclientdata();
                if (false == flag)
                    continue;
            }
            /*
            EPOLLRDHUP：对端断开连接，或者发生了一个使流式套接字（如TCP）半关闭的事件
            EPOLLHUP：当挂断（hang up）事件发生时，这个标志会被设置。在TCP连接中，这通常意味着对端执行了一个正常的关闭序列，发送了一个FIN包
            EPOLLERR：当底层的I/O通道出现错误时，包括网络错误、硬件故障或其他底层传输问题
            */
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                // 服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            // 处理信号
            // 管道读端对应文件描述符发生读事件
            // 因为统一了事件源，信号处理当成读事件来处理，因为读端也放入epoll实例了，可以用epoll监控是否发生
            // 怎么统一？就是信号回调函数哪里不立即处理而是写到：pipe的写端
            else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = dealwithsignal(timeout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            //处理客户连接上接收到的数据 
            else if (events[i].events & EPOLLIN)
            {
                dealwithread(sockfd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                dealwithwrite(sockfd);
            }
        }
        if (timeout)
        {
            utils.timer_handler();

            LOG_INFO("%s", "timer tick");

            timeout = false;
        }
    }
}
