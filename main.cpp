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

    MYSQL* conn = MySQLConnectionPool::instance().getConnection();
    if (conn) {
        const char* createUsersTable =
            "CREATE TABLE IF NOT EXISTS users ("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "username VARCHAR(50) UNIQUE NOT NULL,"
            "password VARCHAR(100) NOT NULL,"
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";

        if (mysql_query(conn, createUsersTable)) {
            LOG_ERROR << "Create users table failed: " << mysql_error(conn);
        }

        const char* createChatMessagesTable =
            "CREATE TABLE IF NOT EXISTS chat_messages ("
            "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
            "room_id VARCHAR(64) NOT NULL DEFAULT '',"
            "sender_uid BIGINT NOT NULL,"
            "receiver_uid BIGINT NOT NULL DEFAULT 0,"
            "content TEXT NOT NULL,"
            "msg_type TINYINT DEFAULT 1,"
            "created_at TIMESTAMP(3) DEFAULT CURRENT_TIMESTAMP(3),"
            "INDEX idx_room_id (room_id),"
            "INDEX idx_sender_uid (sender_uid),"
            "INDEX idx_private (sender_uid, receiver_uid))";

        if (mysql_query(conn, createChatMessagesTable)) {
            LOG_ERROR << "Create chat_messages table failed: " << mysql_error(conn);
        }

        const char* createFriendsTable =
            "CREATE TABLE IF NOT EXISTS friends ("
            "user_id BIGINT NOT NULL,"
            "friend_id BIGINT NOT NULL,"
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
            "PRIMARY KEY (user_id, friend_id),"
            "INDEX idx_friend (friend_id))";

        if (mysql_query(conn, createFriendsTable)) {
            LOG_ERROR << "Create friends table failed: " << mysql_error(conn);
        }
        MySQLConnectionPool::instance().releaseConnection(conn);
    }

    if (!RedisManager::instance().init("127.0.0.1", 6379)) {
        LOG_ERROR << "Failed to initialize Redis manager";
        return 1;
    }

    muduo::net::EventLoop loop;

    DBExecutor::instance().init(&loop, 4);

    store::MysqlUserStore mysqlUserStore(&MySQLConnectionPool::instance());
    store::RedisUserStore redisUserStore(&RedisManager::instance(), &mysqlUserStore);
    store::RedisChatStore redisChatStore(&RedisManager::instance());

    service::AuthService authService(&redisUserStore);
    service::ChatService chatService(&redisChatStore, &mysqlUserStore, &loop);

    muduo::net::InetAddress listenAddr(8888);
    net::ChatServer server(&loop, listenAddr, &authService, &chatService, &redisUserStore);
    server.start();

    LOG_INFO << "Chat server started on port 8888";
    loop.loop();

    DBExecutor::instance().shutdown();
    RedisManager::instance().shutdown();
    MySQLConnectionPool::instance().shutdown();

    return 0;
}