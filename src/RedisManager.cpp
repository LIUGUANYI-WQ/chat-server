#include "RedisManager.h"
#include <random>
#include <sstream>

RedisManager::RedisManager()
    : running_(false)
    , maxConnections_(4)
    , currentConnections_(0)
    , port_(6379) {
}

RedisManager::~RedisManager() {
    shutdown();
}

RedisManager& RedisManager::instance() {
    static RedisManager instance;
    return instance;
}

bool RedisManager::init(const std::string& host, int port, int maxConnections) {
    if (running_) {
        return true;
    }

    host_ = host;
    port_ = port;
    maxConnections_ = maxConnections;
    running_ = true;

    for (int i = 0; i < maxConnections_; ++i) {
        redisContext* conn = createConnection();
        if (conn) {
            connections_.push(conn);
            ++currentConnections_;
        }
    }

    LOG_INFO << "RedisManager initialized with " << currentConnections_ << " connections";
    return currentConnections_ > 0;
}

redisContext* RedisManager::createConnection() {
    redisContext* ctx = redisConnect(host_.c_str(), port_);
    if (ctx && ctx->err) {
        if (ctx) {
            redisFree(ctx);
        }
        return nullptr;
    }
    return ctx;
}

void RedisManager::destroyConnection(redisContext* conn) {
    if (conn) {
        redisFree(conn);
        --currentConnections_;
    }
}

redisContext* RedisManager::getConnection() {
    muduo::MutexLockGuard lock(mutex_);
    if (!running_ || connections_.empty()) {
        if (!running_) {
            return nullptr;
        }
        redisContext* conn = createConnection();
        return conn;
    }

    redisContext* conn = connections_.front();
    connections_.pop();

    if (conn->err) {
        redisFree(conn);
        conn = createConnection();
    }

    return conn;
}

void RedisManager::releaseConnection(redisContext* conn) {
    if (!conn) {
        return;
    }

    muduo::MutexLockGuard lock(mutex_);
    if (currentConnections_ < maxConnections_) {
        if (!conn->err) {
            connections_.push(conn);
        } else {
            redisFree(conn);
            --currentConnections_;
        }
    } else {
        redisFree(conn);
        --currentConnections_;
    }
}

bool RedisManager::set(const std::string& key, const std::string& value, int ttlSeconds) {
    redisContext* conn = getConnection();
    if (!conn) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "SET %s %s", key.c_str(), value.c_str());
    if (!reply) {
        releaseConnection(conn);
        return false;
    }

    bool success = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);

    if (success && ttlSeconds > 0) {
        redisReply* expireReply = (redisReply*)redisCommand(conn, "EXPIRE %s %d", key.c_str(), ttlSeconds);
        if (expireReply) {
            freeReplyObject(expireReply);
        }
    }

    releaseConnection(conn);
    return success;
}

std::string RedisManager::get(const std::string& key) {
    redisContext* conn = getConnection();
    if (!conn) {
        return "";
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "GET %s", key.c_str());
    if (!reply) {
        releaseConnection(conn);
        return "";
    }

    std::string value;
    if (reply->type == REDIS_REPLY_STRING) {
        value = std::string(reply->str, reply->len);
    }

    freeReplyObject(reply);
    releaseConnection(conn);
    return value;
}

bool RedisManager::del(const std::string& key) {
    redisContext* conn = getConnection();
    if (!conn) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "DEL %s", key.c_str());
    if (!reply) {
        releaseConnection(conn);
        return false;
    }

    bool success = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    releaseConnection(conn);
    return success;
}

std::string RedisManager::generateToken(const std::string& username) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream token;
    token << username << "_";
    for (int i = 0; i < 32; ++i) {
        token << std::hex << dis(gen);
    }

    std::string tokenStr = token.str();
    set(tokenStr, username, 86400);

    return tokenStr;
}

bool RedisManager::validateToken(const std::string& token, std::string& username) {
    std::string storedUsername = get(token);
    if (!storedUsername.empty()) {
        username = storedUsername;
        return true;
    }
    return false;
}

// 新增：集合操作
bool RedisManager::sadd(const std::string& key, const std::string& value) {
    redisContext* conn = getConnection();
    if (!conn) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "SADD %s %s", key.c_str(), value.c_str());
    if (!reply) {
        releaseConnection(conn);
        return false;
    }

    bool success = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    releaseConnection(conn);
    return success;
}

bool RedisManager::srem(const std::string& key, const std::string& value) {
    redisContext* conn = getConnection();
    if (!conn) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "SREM %s %s", key.c_str(), value.c_str());
    if (!reply) {
        releaseConnection(conn);
        return false;
    }

    bool success = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    releaseConnection(conn);
    return success;
}

bool RedisManager::sismember(const std::string& key, const std::string& value) {
    redisContext* conn = getConnection();
    if (!conn) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "SISMEMBER %s %s", key.c_str(), value.c_str());
    if (!reply) {
        releaseConnection(conn);
        return false;
    }

    bool result = reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
    freeReplyObject(reply);
    releaseConnection(conn);
    return result;
}

std::vector<std::string> RedisManager::smembers(const std::string& key) {
    std::vector<std::string> result;
    redisContext* conn = getConnection();
    if (!conn) {
        return result;
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "SMEMBERS %s", key.c_str());
    if (!reply) {
        releaseConnection(conn);
        return result;
    }

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            if (reply->element[i]->type == REDIS_REPLY_STRING) {
                result.emplace_back(reply->element[i]->str, reply->element[i]->len);
            }
        }
    }

    freeReplyObject(reply);
    releaseConnection(conn);
    return result;
}

// 新增：列表操作
bool RedisManager::lpush(const std::string& key, const std::string& value) {
    redisContext* conn = getConnection();
    if (!conn) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "LPUSH %s %s", key.c_str(), value.c_str());
    if (!reply) {
        releaseConnection(conn);
        return false;
    }

    bool success = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    releaseConnection(conn);
    return success;
}

bool RedisManager::ltrim(const std::string& key, int start, int stop) {
    redisContext* conn = getConnection();
    if (!conn) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "LTRIM %s %d %d", key.c_str(), start, stop);
    if (!reply) {
        releaseConnection(conn);
        return false;
    }

    bool success = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    releaseConnection(conn);
    return success;
}

std::vector<std::string> RedisManager::lrange(const std::string& key, int start, int stop) {
    std::vector<std::string> result;
    redisContext* conn = getConnection();
    if (!conn) {
        return result;
    }

    redisReply* reply = (redisReply*)redisCommand(conn, "LRANGE %s %d %d", key.c_str(), start, stop);
    if (!reply) {
        releaseConnection(conn);
        return result;
    }

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            if (reply->element[i]->type == REDIS_REPLY_STRING) {
                result.emplace_back(reply->element[i]->str, reply->element[i]->len);
            }
        }
    }

    freeReplyObject(reply);
    releaseConnection(conn);
    return result;
}

void RedisManager::shutdown() {
    if (!running_) {
        return;
    }

    running_ = false;

    muduo::MutexLockGuard lock(mutex_);
    while (!connections_.empty()) {
        redisContext* conn = connections_.front();
        connections_.pop();
        if (conn) {
            redisFree(conn);
        }
    }
    currentConnections_ = 0;
}