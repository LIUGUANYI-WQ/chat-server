#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/TcpConnection.h>

#include "MySQLConnectionPool.h"
#include "DBExecutor.h"
#include "RedisManager.h"

#include <string>
#include <unordered_map>
#include <mutex>

using namespace muduo;
using namespace muduo::net;

int main() {
    EventLoop loop;

    if (!MySQLConnectionPool::instance().init("localhost", "root", "123456",
                                                "testdb", 3306, 8)) {
        LOG_ERROR << "Failed to initialize MySQL connection pool";
        return -1;
    }

    MYSQL* conn = MySQLConnectionPool::instance().getConnection();
    if (conn) {
        const char* createTable =
            "CREATE TABLE IF NOT EXISTS users ("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "username VARCHAR(50) UNIQUE NOT NULL,"
            "password VARCHAR(100) NOT NULL,"
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";

        if (mysql_query(conn, createTable)) {
            LOG_ERROR << "Create table failed: " << mysql_error(conn);
        }
        MySQLConnectionPool::instance().releaseConnection(conn);
    }

    if (!RedisManager::instance().init("127.0.0.1", 6379, 4)) {
        LOG_ERROR << "Failed to initialize Redis connection pool";
        return -1;
    }

    DBExecutor::instance().init(&loop, 4);

    InetAddress addr(8888);
    TcpServer server(&loop, addr, "LoginServer");
    server.setThreadNum(4);

    server.setConnectionCallback([](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_INFO << "Client connected: " << conn->peerAddress().toIpPort();
        } else {
            LOG_INFO << "Client disconnected: " << conn->peerAddress().toIpPort();
        }
    });

    server.setMessageCallback([&loop](const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
        std::string msg = buf->retrieveAllAsString();

        size_t firstSpace = msg.find(" ");
        if (firstSpace == std::string::npos) {
            conn->send("ERROR: Invalid command format\n");
            return;
        }

        std::string cmd = msg.substr(0, firstSpace);

        std::weak_ptr<TcpConnection> weakConn(conn);

        if (cmd == "TOKEN_LOGIN") {
            std::string token = msg.substr(firstSpace + 1);
            size_t newline = token.find("\n");
            if (newline != std::string::npos) {
                token = token.substr(0, newline);
            }

            LOG_INFO << "Received token login request";

            std::string username;
            if (RedisManager::instance().validateToken(token, username)) {
                std::string response = "SUCCESS: Token login successful, username: " + username + "\n";
                conn->send(response);
                LOG_INFO << "Token login successful for user: " << username;
            } else {
                conn->send("ERROR: Invalid or expired token\n");
                LOG_INFO << "Token login failed: invalid token";
            }
            return;
        }

        if (cmd == "REGISTER" || cmd == "LOGIN") {
            size_t secondSpace = msg.find(" ", firstSpace + 1);
            if (secondSpace == std::string::npos) {
                conn->send("ERROR: Invalid command format\n");
                return;
            }

            std::string username = msg.substr(firstSpace + 1, secondSpace - firstSpace - 1);
            std::string password = msg.substr(secondSpace + 1);

            size_t newline = password.find("\n");
            if (newline != std::string::npos) {
                password = password.substr(0, newline);
            }

            LOG_INFO << "Received request: " << cmd << " " << username;

            auto callback = [weakConn, cmd](bool success, const std::string& message) {
                TcpConnectionPtr conn = weakConn.lock();
                if (conn && conn->connected()) {
                    std::string response;
                    if (success) {
                        response = "SUCCESS: " + message + "\n";
                    } else {
                        response = "ERROR: " + message + "\n";
                    }
                    conn->send(response);
                    LOG_INFO << message;
                }
            };

            if (cmd == "REGISTER") {
                DBExecutor::instance().asyncRegister(username, password, &loop, callback);
            } else if (cmd == "LOGIN") {
                DBExecutor::instance().asyncLoginWithToken(username, password, &loop, callback);
            }
        } else {
            conn->send("ERROR: Unknown command\n");
        }
    });

    server.start();
    LOG_INFO << "Server started on port 8888";
    loop.loop();

    DBExecutor::instance().shutdown();
    MySQLConnectionPool::instance().shutdown();
    RedisManager::instance().shutdown();

    return 0;
}