#ifndef REDIS_MANAGER_H
#define REDIS_MANAGER_H

#include <hiredis/hiredis.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <queue>
#include <memory>
#include <string>

class RedisManager {
public:
    static RedisManager& instance();

    bool init(const std::string& host, int port, int maxConnections = 4);
    bool set(const std::string& key, const std::string& value, int ttlSeconds = 3600);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    std::string generateToken(const std::string& username);
    bool validateToken(const std::string& token, std::string& username);
    void shutdown();

private:
    RedisManager();
    ~RedisManager();

    RedisManager(const RedisManager&) = delete;
    RedisManager& operator=(const RedisManager&) = delete;

    redisContext* getConnection();
    void releaseConnection(redisContext* conn);
    redisContext* createConnection();
    void destroyConnection(redisContext* conn);

    std::queue<redisContext*> connections_;
    muduo::MutexLock mutex_;
    bool running_;
    int maxConnections_;
    int currentConnections_;

    std::string host_;
    int port_;
};

#endif // REDIS_MANAGER_H