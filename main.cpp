#include "include/MySQLConnectionPool.h"
#include "include/RedisManager.h"
#include "store/MysqlUserStore.h"
#include "store/RedisUserStore.h"
#include "store/RedisChatStore.h"
#include "service/AuthService.h"
#include "service/ChatService.h"
#include "net/ChatServer.h"
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <memory>

int main() {
    // 初始化日志
    muduo::Logger::setLogLevel(muduo::Logger::INFO);

    // 初始化 MySQL
    MySQLConnectionPool& mysqlPool = MySQLConnectionPool::instance();
    if (!mysqlPool.init("127.0.0.1", "root", "123456", "test_db", 3306, 8)) {
        LOG_ERROR << "Failed to initialize MySQL pool";
        return 1;
    }

    // 初始化 Redis
    RedisManager& redisMgr = RedisManager::instance();
    if (!redisMgr.init("127.0.0.1", 6379, 4)) {
        LOG_ERROR << "Failed to initialize Redis manager";
        return 1;
    }

    // 创建 store 层实例
    auto mysqlUserStore = std::make_unique<store::MysqlUserStore>(&mysqlPool);
    auto redisUserStore = std::make_unique<store::RedisUserStore>(&redisMgr, mysqlUserStore.get());
    auto redisChatStore = std::make_unique<store::RedisChatStore>(&redisMgr);

    // 创建 service 层实例
    auto authService = std::make_unique<service::AuthService>(redisUserStore.get());
    auto chatService = std::make_unique<service::ChatService>(redisChatStore.get(), redisUserStore.get());

    // 创建 server
    muduo::net::EventLoop loop;
    muduo::net::InetAddress listenAddr(8888);
    net::ChatServer server(&loop, listenAddr, authService.get(), chatService.get());

    server.start();
    LOG_INFO << "ChatServer started on port 8888";
    loop.loop();

    return 0;
}