#include "lst_timer.h"
#include "../http/http_conn.h"

sort_timer_lst::sort_timer_lst()
{
    head = NULL;
    tail = NULL;
}
sort_timer_lst::~sort_timer_lst()
{
    util_timer *tmp = head;
    while (tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

// 添加一个定时器 
void sort_timer_lst::add_timer(util_timer *timer)
{
    // 如果timer为nullptr直接返回 
    if (!timer)
    {
        return;
    }
    // 判断当前链表是否为空 
    if (!head)
    {
        head = tail = timer;
        return;
    }
    // 如果当前计时器的超时时间小于头结点，将其插入到头部位置 
    if (timer->expire < head->expire)
    {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    // 否则调用这个方法将其插入到合适的位置 
    add_timer(timer, head);
}
// 调整新定时器在链表上的位置 
void sort_timer_lst::adjust_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    util_timer *tmp = timer->next;
    // 不需要调整的话直接返回 
    if (!tmp || (timer->expire < tmp->expire))
    {
        return;
    }
    // 如果需要调整的话要先删除，然后插入到合适的位置 
    // 如果是头节点，重新设置头节点然后插入到合适的位置 
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    // 如果不是头节点，先删除重新设置前后结点的指向 然后插入到合适的位置 
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}
// 在链表中删除定时器 
void sort_timer_lst::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    // 如果只有一个节点 
    if ((timer == head) && (timer == tail))
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    // 如果是头节点 
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    // 如果是尾节点 
    if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}
// 处理定时事件，遍历整个定时器链表，查找有没有超时的定时器，并执行相应的回调函数
void sort_timer_lst::tick()
{
    if (!head)
    {
        return;
    }
    // 获取当前时间 
    time_t cur = time(NULL);
    util_timer *tmp = head;
    // 遍历链表，判断当前时间是否超过了节点过期时间 
    while (tmp)
    {
        // 没有超过则结束循环 
        if (cur < tmp->expire)
        {
            break;
        }
        // 执行回调函数 
        tmp->cb_func(tmp->user_data);
        // 更新头节点 
        head = tmp->next;
        if (head)
        {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head)
{
    // 定义两个指针分别指向链表头节点和下一个头节点 
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    // 遍历链表 
    while (tmp)
    {
        // 找到合适的位置，就直接插入 
        if (timer->expire < tmp->expire)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    // 遍历到结尾也没有找到，就插入当作尾节点
    if (!tmp)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
/*设置文件描述符为非阻塞模式的目的在于避免在读写操作时被阻塞而导致程序无法响应其他事件。
例如，在网络编程中，若一个 socket 连接处于阻塞状态，那么当没有数据可读或者可写时，
程序会一直停留在相应的读或写操作上，从而无法处理其他连接请求或者信号等事件。*/
int Utils::setnonblocking(int fd)
{
    // 获取当前文件描述符的设置 
    int old_option = fcntl(fd, F_GETFL);
    // 添加非阻塞设置
    int new_option = old_option | O_NONBLOCK;
    // 应用到文件描述符上 
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    // ET
    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    // 当设置了 EPOLLONESHOT 标志位后，该事件将变为一次性事件，确保每个socket连接只处理一次。
    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // 将fd设置为非阻塞模式 
    setnonblocking(fd);
}

// SIGALRM信号处理函数，这里会将信号发送到管道写端 
void Utils::sig_handler(int sig)
{
    // 异步信号，这可能会改变全局变量 errno 的值
    // 为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    // 将信号编号 msg 发送到管道的写端 u_pipefd[1]
    send(u_pipefd[1], (char *)&msg, 1, 0);
    // 将原来的errno值恢复，确保不影响其他代码中errno的使用
    errno = save_errno;
}

// 设置信号函数，指定信号处理函数handler
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;

    // 如果 restart 参数为 true，则设置 sa_flags 成员的 SA_RESTART 标志。
    // 这意味着如果信号处理函数执行期间一个系统调用被信号中断，系统调用将自动重启而不是返回错误。
    // 即当信号处理函数返回时，系统会自动重启被该信号中断的系统调用
    if (restart)
        sa.sa_flags |= SA_RESTART;
    
    // 在信号处理期间，除了正在处理的信号之外，所有其他信号都将被阻塞
    sigfillset(&sa.sa_mask);

    // 调用 sigaction 函数将新的信号处理设置应用到指定的信号上。
    assert(sigaction(sig, &sa, NULL) != -1);
}

// 定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    // 遍历定时器链表，执行超时任务并删除过期节点 
    m_timer_lst.tick();
    // m_TIMESLOT发送一个alarm信号 
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    // 将user_data->sockfd从epoll监听集合中删除 
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    // 关闭ser_data->sockfd套接字 
    close(user_data->sockfd);
    // http连接的计数器-1
    http_conn::m_user_count--;
}
