#include "DBExecutor.h"
#include "MySQLConnectionPool.h"
#include <cstring>
#include "RedisManager.h"
DBExecutor::DBExecutor() : running_(false) {}

DBExecutor::~DBExecutor() {
    shutdown();
}

DBExecutor& DBExecutor::instance() {
    static DBExecutor instance;
    return instance;
}

void DBExecutor::init(muduo::net::EventLoop* loop, int threadNum) {
    if (running_) {
        return;
    }
    running_ = true;
    threadPool_.start(threadNum);
    LOG_INFO << "DBExecutor initialized with " << threadNum << " threads";      
}

void DBExecutor::shutdown() {
    if (!running_) {
        return;
    }
    running_ = false;
    threadPool_.stop();
}

bool DBExecutor::doRegister(const std::string& username, const std::string& password) {
    MYSQL* conn = MySQLConnectionPool::instance().getConnection();
    if (!conn) {
        return false;
    }

    MySQLConnectionGuard guard(conn, &MySQLConnectionPool::instance());

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        return false;
    }

    const char* insertSql = "INSERT INTO users (username, password) VALUES (?, ?)";

    if (mysql_stmt_prepare(stmt, insertSql, strlen(insertSql))) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[2];
    memset(params, 0, sizeof(params));

    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (char*)username.c_str();
    params[0].buffer_length = username.length();

    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (char*)password.c_str();
    params[1].buffer_length = password.length();

    if (mysql_stmt_bind_param(stmt, params)) {
        mysql_stmt_close(stmt);
        return false;
    }

    bool success = !mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    return success;
}

bool DBExecutor::doLogin(const std::string& username, const std::string& password) {
    MYSQL* conn = MySQLConnectionPool::instance().getConnection();
    if (!conn) {
        return false;
    }

    MySQLConnectionGuard guard(conn, &MySQLConnectionPool::instance());

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        return false;
    }

    const char* selectSql = "SELECT password FROM users WHERE username = ?";    

    if (mysql_stmt_prepare(stmt, selectSql, strlen(selectSql))) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[1];
    memset(params, 0, sizeof(params));

    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (char*)username.c_str();
    params[0].buffer_length = username.length();

    if (mysql_stmt_bind_param(stmt, params)) {
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt)) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
    if (!result) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND resultBind[1];
    char dbPassword[101];
    unsigned long length;

    memset(resultBind, 0, sizeof(resultBind));
    resultBind[0].buffer_type = MYSQL_TYPE_STRING;
    resultBind[0].buffer = dbPassword;
    resultBind[0].buffer_length = 100;
    resultBind[0].length = &length;

    mysql_stmt_bind_result(stmt, resultBind);
    mysql_stmt_store_result(stmt);

    bool success = false;
    if (mysql_stmt_fetch(stmt) == 0) {
        dbPassword[length] = '\0';
        success = (password == dbPassword);
    }

    mysql_free_result(result);
    mysql_stmt_close(stmt);

    return success;
}

bool DBExecutor::doSaveChatMessage(const store::ChatMsg& msg) {
    MYSQL* conn = MySQLConnectionPool::instance().getConnection();
    if (!conn) {
        return false;
    }

    MySQLConnectionGuard guard(conn, &MySQLConnectionPool::instance());

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        return false;
    }

    const char* insertSql = "INSERT INTO chat_messages (id, room_id, sender_uid, content, created_at) VALUES (?, ?, ?, ?, ?)";

    if (mysql_stmt_prepare(stmt, insertSql, strlen(insertSql))) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[5];
    memset(params, 0, sizeof(params));

    uint64_t id = msg.id;
    params[0].buffer_type = MYSQL_TYPE_LONGLONG;
    params[0].buffer = &id;

    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (char*)msg.room_id.c_str();
    params[1].buffer_length = msg.room_id.length();

    uint64_t senderUid = msg.sender_uid;
    params[2].buffer_type = MYSQL_TYPE_LONGLONG;
    params[2].buffer = &senderUid;

    params[3].buffer_type = MYSQL_TYPE_STRING;
    params[3].buffer = (char*)msg.content.c_str();
    params[3].buffer_length = msg.content.length();

    std::time_t time = std::chrono::system_clock::to_time_t(msg.created_at);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&time));
    params[4].buffer_type = MYSQL_TYPE_STRING;
    params[4].buffer = timeStr;
    params[4].buffer_length = strlen(timeStr);

    if (mysql_stmt_bind_param(stmt, params)) {
        mysql_stmt_close(stmt);
        return false;
    }

    bool success = !mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    return success;
}

void DBExecutor::asyncRegister(const std::string& username, const std::string& password,
                               muduo::net::EventLoop* callerLoop,
                               const ResultCallback& callback) {
    if (!running_ || !callerLoop) {
        if (callback) {
            callerLoop->runInLoop(std::bind(callback, false, "DB executor not running"));
        }
        return;
    }

    threadPool_.run([this, username, password, callerLoop, callback]() {        
        bool success = doRegister(username, password);
        std::string message;
        if (success) {
            message = "Register successful";
        } else {
            message = "Register failed (username exists?)";
        }

        callerLoop->runInLoop(std::bind(callback, success, message));
    });
}

void DBExecutor::asyncLogin(const std::string& username, const std::string& password,
                            muduo::net::EventLoop* callerLoop,
                            const ResultCallback& callback) {
    if (!running_ || !callerLoop) {
        if (callback) {
            callerLoop->runInLoop(std::bind(callback, false, "DB executor not running"));
        }
        return;
    }

    threadPool_.run([this, username, password, callerLoop, callback]() {        
        bool success = doLogin(username, password);
        std::string message;
        if (success) {
            message = "Login successful";
        } else {
            message = "Login failed (wrong username/password)";
        }

        callerLoop->runInLoop(std::bind(callback, success, message));
    });
}

void DBExecutor::asyncLoginWithToken(const std::string& username, const std::string& password,
                                      muduo::net::EventLoop* callerLoop,        
                                      const ResultCallback& callback) {
    if (!running_ || !callerLoop) {
        if (callback) {
            callerLoop->runInLoop(std::bind(callback, false, "DB executor not running"));
        }
        return;
    }

    threadPool_.run([this, username, password, callerLoop, callback]() {        
        bool success = doLogin(username, password);
        std::string message;
        if (success) {
            std::string token = RedisManager::instance().generateToken(username);
            message = "Login successful, token: " + token;
        } else {
            message = "Login failed (wrong username/password)";
        }

        callerLoop->runInLoop(std::bind(callback, success, message));
    });
}

void DBExecutor::asyncSaveChatMessage(const store::ChatMsg& msg,
                                       muduo::net::EventLoop* callerLoop,
                                       const ResultCallback& callback) {
    if (!running_ || !callerLoop) {
        if (callback) {
            callerLoop->runInLoop(std::bind(callback, false, "DB executor not running"));
        }
        return;
    }

    threadPool_.run([this, msg, callerLoop, callback]() {
        bool success = doSaveChatMessage(msg);
        std::string message = success ? "Message saved" : "Failed to save message";

        callerLoop->runInLoop(std::bind(callback, success, message));
    });
}

void DBExecutor::asyncAddFriend(uint64_t uid, uint64_t friendUid,
                                 muduo::net::EventLoop* callerLoop,
                                 const ResultCallback& callback) {
    if (!running_ || !callerLoop) {
        if (callback) {
            callerLoop->runInLoop(std::bind(callback, false, "DB executor not running"));
        }
        return;
    }

    threadPool_.run([this, uid, friendUid, callerLoop, callback]() mutable {
        MYSQL* conn = MySQLConnectionPool::instance().getConnection();
        if (!conn) {
            callerLoop->runInLoop(std::bind(callback, false, "No DB connection"));
            return;
        }
        MySQLConnectionGuard guard(conn, &MySQLConnectionPool::instance());

        // 双向插入
        const char* sql = "INSERT IGNORE INTO friends (user_id, friend_id) VALUES (?, ?), (?, ?)";
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) {
            callerLoop->runInLoop(std::bind(callback, false, "stmt init failed"));
            return;
        }

        if (mysql_stmt_prepare(stmt, sql, strlen(sql))) {
            mysql_stmt_close(stmt);
            callerLoop->runInLoop(std::bind(callback, false, mysql_error(conn)));
            return;
        }

        MYSQL_BIND params[4];
        memset(params, 0, sizeof(params));
        params[0].buffer_type = MYSQL_TYPE_LONGLONG; params[0].buffer = &uid;
        params[1].buffer_type = MYSQL_TYPE_LONGLONG; params[1].buffer = &friendUid;
        params[2].buffer_type = MYSQL_TYPE_LONGLONG; params[2].buffer = &friendUid;
        params[3].buffer_type = MYSQL_TYPE_LONGLONG; params[3].buffer = &uid;

        bool success = true;
        if (mysql_stmt_bind_param(stmt, params) || mysql_stmt_execute(stmt)) {
            success = false;
        }

        mysql_stmt_close(stmt);
        std::string msg = success ? "Friend added" : "Failed";
        callerLoop->runInLoop(std::bind(callback, success, msg));
    });
}