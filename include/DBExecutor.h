#ifndef DB_EXECUTOR_H
#define DB_EXECUTOR_H

#include "MySQLConnectionPool.h"
#include "RedisManager.h"
#include <muduo/net/EventLoop.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/base/Logging.h>
#include <functional>
#include <memory>
#include <string>

class DBExecutor {
public:
    using ResultCallback = std::function<void(bool success, const std::string& message)>;

    static DBExecutor& instance();

    void init(muduo::net::EventLoop* loop, int threadNum = 4);
    void shutdown();

    void asyncRegister(const std::string& username, const std::string& password,
                       muduo::net::EventLoop* callerLoop,
                       const ResultCallback& callback);

    void asyncLogin(const std::string& username, const std::string& password,
                    muduo::net::EventLoop* callerLoop,
                    const ResultCallback& callback);

    void asyncLoginWithToken(const std::string& username, const std::string& password,
                             muduo::net::EventLoop* callerLoop,
                             const ResultCallback& callback);

private:
    DBExecutor();
    ~DBExecutor();

    DBExecutor(const DBExecutor&) = delete;
    DBExecutor& operator=(const DBExecutor&) = delete;

    bool doRegister(const std::string& username, const std::string& password);
    bool doLogin(const std::string& username, const std::string& password);

    muduo::ThreadPool threadPool_;
    bool running_;
};

#endif // DB_EXECUTOR_H