#include "store/MysqlUserStore.h"
#include "MySQLConnectionPool.h"
#include <muduo/base/Logging.h>
#include <mysql/mysql.h>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace store {

MysqlUserStore::MysqlUserStore(MySQLConnectionPool* pool) : pool_(pool) {
}

std::optional<User> MysqlUserStore::findByToken(const std::string& token) {
    // findByToken 在RedisUserStore中实现，这个留空或者抛异常
    LOG_WARN << "MysqlUserStore::findByToken not implemented, use RedisUserStore";
    return std::nullopt;
}

std::optional<User> MysqlUserStore::findByUsername(const std::string& username) {
    if (!pool_) {
        return std::nullopt;
    }

    MYSQL* conn = pool_->getConnection();
    if (!conn) {
        return std::nullopt;
    }

    MySQLConnectionGuard guard(conn, pool_);

    std::string sql = "SELECT id, username, password, created_at FROM users WHERE username = '";
    sql += username;
    sql += "'";

    if (mysql_query(conn, sql.c_str()) != 0) {
        LOG_ERROR << "MySQL query failed: " << mysql_error(conn);
        return std::nullopt;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        LOG_ERROR << "MySQL store result failed: " << mysql_error(conn);
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return std::nullopt;
    }

    User user;
    user.id = std::stoull(row[0]);
    user.username = row[1];
    user.password = row[2];

    // 解析created_at
    std::tm tm = {};
    std::istringstream ss(row[3]);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    user.created_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));

    mysql_free_result(result);
    return user;
}

bool MysqlUserStore::insertUser(const User& user) {
    if (!pool_) {
        return false;
    }

    MYSQL* conn = pool_->getConnection();
    if (!conn) {
        return false;
    }

    MySQLConnectionGuard guard(conn, pool_);

    std::string sql = "INSERT INTO users (username, password) VALUES ('";
    sql += user.username;
    sql += "', '";
    sql += user.password;
    sql += "')";

    if (mysql_query(conn, sql.c_str()) != 0) {
        LOG_ERROR << "MySQL insert failed: " << mysql_error(conn);
        return false;
    }

    return true;
}

void MysqlUserStore::saveToken(const std::string& token, uint64_t uid) {
    // 这个在RedisUserStore中实现
    LOG_WARN << "MysqlUserStore::saveToken not implemented, use RedisUserStore";
}

} // namespace store