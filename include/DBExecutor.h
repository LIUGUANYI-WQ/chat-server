#ifndef INCLUDE_DBEXECUTOR_H
#define INCLUDE_DBEXECUTOR_H

#include "../store/types.h"
#include <muduo/net/EventLoop.h>
#include <muduo/base/ThreadPool.h>
#include <functional>
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

    void asyncSaveChatMessage(const store::ChatMsg& msg,
                              muduo::net::EventLoop* callerLoop,
                              const ResultCallback& callback);

    void asyncAddFriend(uint64_t uid, uint64_t friendUid,
                        muduo::net::EventLoop* callerLoop,
                        const ResultCallback& callback);

private:
    DBExecutor();
    ~DBExecutor();

    bool doRegister(const std::string& username, const std::string& password);
    bool doLogin(const std::string& username, const std::string& password);
    bool doSaveChatMessage(const store::ChatMsg& msg);

    muduo::ThreadPool threadPool_;
    bool running_;
};

#endif // INCLUDE_DBEXECUTOR_H