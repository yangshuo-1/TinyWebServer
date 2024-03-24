#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
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
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

// https://blog.csdn.net/ailunlee/article/details/90600174
// https://blog.csdn.net/zhaojiazb/article/details/130492949?spm=1001.2101.3001.6650.11&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-11-130492949-blog-113184114.235%5Ev43%5Epc_blog_bottom_relevance_base7&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-11-130492949-blog-113184114.235%5Ev43%5Epc_blog_bottom_relevance_base7&utm_relevant_index=18

class http_conn
{
public:
    // qqqqq：这是什么的filename长度？ 
    static const int FILENAME_LEN = 200;
    // 读缓存池大小 
    static const int READ_BUFFER_SIZE = 2048;
    // 写缓存池大小 
    static const int WRITE_BUFFER_SIZE = 1024;
    // 报文的请求方法
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };

    // 主状态机的状态  
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,    // 处理请求行 
        CHECK_STATE_HEADER,             // 处理请求头
        CHECK_STATE_CONTENT             // 处理请求体
    };
    // 报文解析的结果 
    enum HTTP_CODE
    {
        NO_REQUEST,             // 请求不完整，需要继续读取请求报文数据
        GET_REQUEST,            // 获得了完整的HTTP请求
        BAD_REQUEST,            // HTTP请求报文有语法错误或请求资源为目录
        NO_RESOURCE,            // 请求资源不存在
        FORBIDDEN_REQUEST,      // 请求资源禁止访问，没有读取权限
        FILE_REQUEST,           // 请求资源可以正常访问
        INTERNAL_ERROR,         // 服务器内部错误
        CLOSED_CONNECTION       //
    };
    // 从状态机的状态 
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);
    // 报文解析函数，返回解析的情况 
    void process();

    bool read_once();
    bool write();
    // 
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;                                 // 
    int improv;                                     // 


private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;                           // epoll实例的文件描述符 
    static int m_user_count;                        // 
    MYSQL *mysql;                                   // 
    int m_state;                                    // 读为0, 写为1

private:
    int m_sockfd;                                   // 
    sockaddr_in m_address;                          // 
    char m_read_buf[READ_BUFFER_SIZE];              // 读缓存区
    long m_read_idx;                                // 读缓存区已读取数据的索引
    long m_checked_idx;                             // 当前从状态机在buffer中读取的位置
    int m_start_line;                               // 数据行在buffer的起始位置
    char m_write_buf[WRITE_BUFFER_SIZE];            // 写缓存区
    int m_write_idx;                                //
    CHECK_STATE m_check_state;                      // 主状态机的状态
    METHOD m_method;                                // 解析http请求行，获取的请求方式
    char m_real_file[FILENAME_LEN];                 // http请求要访问的文件名-do_request中获取
    char *m_url;                                    // url-在请求头中获取
    char *m_version;                                // http版本-在请求头中获取
    char *m_host;                                   // 端口号-在请求头中获取
    long m_content_length;                          // 请求消息体长度-在请求头中获取
    bool m_linger;                                  // 长连接标志 ---qqqq啥意思？
    char *m_file_address;                           // 文件内容所在的内存区域指针-在do_request中赋值
    struct stat m_file_stat;                        // 
    struct iovec m_iv[2];                           // 
    int m_iv_count;                                 // 
    int cgi;                                        // 是否启用的POST
    char *m_string;                                 // 存储请求体数据
    int bytes_to_send;                              // 
    int bytes_have_send;                            // 
    char *doc_root;                                 // 文件的根目录？从初始化那里获取的

    map<string, string> m_users;
    int m_TRIGMode;                                 // 触发模式 
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif
