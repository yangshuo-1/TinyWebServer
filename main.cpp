#include "config.h"

int main(int argc, char *argv[])
{
    //需要修改的数据库信息,登录名,密码,库名
    string user = "root";
    string passwd = "123456";
    string databasename = "mydb";

    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    //初始化
    server.init(config.PORT, user, passwd, databasename, config.LOGWrite, 
                config.OPT_LINGER, config.TRIGMode,  config.sql_num,  config.thread_num, 
                config.close_log, config.actor_model);
    

    //日志
    // y:单例模式的日志初始化，根据config判断日志是否打开，打开的什么模式 
    server.log_write();

    //数据库
    // y：数据库连接池初始化，init根据上表的用户名和密码登陆 
    server.sql_pool();

    //线程池
    // y：初始化线程池 线程类型是http_conn
    server.thread_pool();

    //触发模式
    // y：选择触发模式，LT和ET的排列组合
    server.trig_mode();

    //监听
    // webserver中：服务器通过socket监听用户请求，客户端尝试连接服务端的的某个port，socket监听该port
    // 监听到的连接会排队等待接受。服务器通过eppoll实现对监听socket和连接socket的同时监听。
    // 本项目中是怎么监听的呢？
    server.eventListen();

    //运行
    server.eventLoop();

    return 0;
}