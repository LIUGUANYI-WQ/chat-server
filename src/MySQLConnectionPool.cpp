#include "MySQLConnectionPool.h"
#include <cstring>

MySQLConnectionPool::MySQLConnectionPool()
    : cond_(mutex_), running_(false), maxConnections_(8), currentConnections_(0) {}

MySQLConnectionPool::~MySQLConnectionPool() {
    shutdown();
}

MySQLConnectionPool& MySQLConnectionPool::instance() {
    static MySQLConnectionPool instance;
    return instance;
}

bool MySQLConnectionPool::init(const std::string& host, const std::string& user,
                                const std::string& password, const std::string& dbName,
                                unsigned int port, int maxConnections) {
    host_ = host;
    user_ = user;
    password_ = password;
    dbName_ = dbName;
    port_ = port;
    maxConnections_ = maxConnections;
    running_ = true;

    for (int i = 0; i < maxConnections_ / 2; ++i) {
        MYSQL* conn = createConnection();
        if (conn) {
            connections_.push(conn);
            currentConnections_++;
        }
    }

    return currentConnections_ > 0;
}

MYSQL* MySQLConnectionPool::getConnection() {
    muduo::MutexLockGuard lock(mutex_);
    while (connections_.empty() && running_) {
        if (currentConnections_ < maxConnections_) {
            MYSQL* conn = createConnection();
            if (conn) {
                currentConnections_++;
                return conn;
            }
        }
        cond_.wait();
    }

    if (!running_) {
        return nullptr;
    }

    MYSQL* conn = connections_.front();
    connections_.pop();

    if (mysql_ping(conn)) {
        LOG_WARN << "MySQL connection lost, reconnecting...";
        mysql_close(conn);
        conn = createConnection();
        if (!conn) {
            currentConnections_--;
        }
    }

    return conn;
}

void MySQLConnectionPool::releaseConnection(MYSQL* conn) {
    if (!conn) {
        return;
    }

    muduo::MutexLockGuard lock(mutex_);
    if (running_) {
        connections_.push(conn);
        cond_.notify();
    } else {
        destroyConnection(conn);
    }
}

void MySQLConnectionPool::shutdown() {
    muduo::MutexLockGuard lock(mutex_);
    running_ = false;
    cond_.notifyAll();

    while (!connections_.empty()) {
        MYSQL* conn = connections_.front();
        connections_.pop();
        destroyConnection(conn);
        currentConnections_--;
    }
}

MYSQL* MySQLConnectionPool::createConnection() {
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) {
        LOG_ERROR << "MySQL init failed";
        return nullptr;
    }

    if (!mysql_real_connect(conn, host_.c_str(), user_.c_str(), password_.c_str(),
                            dbName_.c_str(), port_, nullptr, 0)) {
        LOG_ERROR << "MySQL connect failed: " << mysql_error(conn);
        mysql_close(conn);
        return nullptr;
    }

    return conn;
}

void MySQLConnectionPool::destroyConnection(MYSQL* conn) {
    if (conn) {
        mysql_close(conn);
    }
}
