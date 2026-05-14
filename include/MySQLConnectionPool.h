#ifndef MYSQL_CONNECTION_POOL_H
#define MYSQL_CONNECTION_POOL_H

#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Condition.h>
#include <queue>
#include <memory>
#include <string>

class MySQLConnectionPool {
public:
    static MySQLConnectionPool& instance();

    bool init(const std::string& host, const std::string& user, 
              const std::string& password, const std::string& dbName,
              unsigned int port, int maxConnections = 8);

    MYSQL* getConnection();
    void releaseConnection(MYSQL* conn);
    void shutdown();

private:
    MySQLConnectionPool();
    ~MySQLConnectionPool();

    MySQLConnectionPool(const MySQLConnectionPool&) = delete;
    MySQLConnectionPool& operator=(const MySQLConnectionPool&) = delete;

    MYSQL* createConnection();
    void destroyConnection(MYSQL* conn);

    std::queue<MYSQL*> connections_;
    muduo::MutexLock mutex_;
    muduo::Condition cond_;
    bool running_;
    int maxConnections_;
    int currentConnections_;

    std::string host_;
    std::string user_;
    std::string password_;
    std::string dbName_;
    unsigned int port_;
};

class MySQLConnectionGuard {
public:
    explicit MySQLConnectionGuard(MYSQL* conn, MySQLConnectionPool* pool)
        : conn_(conn), pool_(pool) {}
    
    ~MySQLConnectionGuard() {
        if (conn_ && pool_) {
            pool_->releaseConnection(conn_);
        }
    }

    MYSQL* get() const { return conn_; }
    MYSQL* operator->() const { return conn_; }
    MYSQL& operator*() const { return *conn_; }

    MySQLConnectionGuard(const MySQLConnectionGuard&) = delete;
    MySQLConnectionGuard& operator=(const MySQLConnectionGuard&) = delete;

private:
    MYSQL* conn_;
    MySQLConnectionPool* pool_;
};

#endif // MYSQL_CONNECTION_POOL_H
