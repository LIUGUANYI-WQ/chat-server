#include "include/MySQLConnectionPool.h"
#include "include/RedisManager.h"
#include "include/DBExecutor.h"
#include "store/MysqlUserStore.h"
#include "store/RedisUserStore.h"
#include "store/RedisChatStore.h"
#include "service/AuthService.h"
#include "service/ChatService.h"
#include "net/ChatServer.h"
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>

int main() {
    if (!MySQLConnectionPool::instance().init("127.0.0.1", "root", "123456", "test_db", 3306, 8)) {
        LOG_ERROR << "Failed to initialize MySQL connection pool";
        return 1;
    }

    if (!RedisManager::instance().init("127.0.0.1", 6379)) {
        LOG_ERROR << "Failed to initialize Redis manager";
        return 1;
    }

    muduo::net::EventLoop loop;

    DBExecutor::instance().init(&loop, 4);

    store::MysqlUserStore mysqlUserStore;
    store::RedisUserStore redisUserStore;
    store::RedisChatStore redisChatStore;

    service::AuthService authService(&mysqlUserStore, &redisUserStore, &loop);
    service::ChatService chatService(&redisChatStore, &mysqlUserStore, &loop);

    muduo::net::InetAddress listenAddr(8888);
    net::ChatServer server(&loop, listenAddr, &authService, &chatService);
    server.start();

    LOG_INFO << "Chat server started on port 8888";
    loop.loop();

    DBExecutor::instance().shutdown();
    RedisManager::instance().shutdown();
    MySQLConnectionPool::instance().shutdown();

    return 0;
}