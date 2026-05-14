cmake_minimum_required(VERSION 3.10)
project(LoginServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(MUDUO_INCLUDE_DIR muduo/net/EventLoop.h)
find_library(MUDUO_BASE muduo_base)
find_library(MUDUO_NET muduo_net)

find_path(MYSQL_INCLUDE_DIR mysql/mysql.h)
find_library(MYSQL_LIBRARY mysqlclient)

find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY hiredis)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/protocol ${MUDUO_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${HIREDIS_INCLUDE_DIR})

# Protocol library (迭代一)
add_library(protocol STATIC protocol/message.cpp)

# Server (原始功能，暂时保持原样)
add_executable(server
    src/server.cpp
    src/MySQLConnectionPool.cpp
    src/DBExecutor.cpp
    src/RedisManager.cpp
)
target_link_libraries(server ${MUDUO_BASE} ${MUDUO_NET} ${MYSQL_LIBRARY} ${HIREDIS_LIBRARY} pthread)

add_executable(client src/client.cpp)
target_link_libraries(client ${MUDUO_BASE} ${MUDUO_NET} pthread)

add_executable(benchmark benchmark/benchmark.cpp)
target_link_libraries(benchmark ${MUDUO_BASE} ${MUDUO_NET} pthread)

# 协议单元测试
add_executable(protocol_test test/protocol_test.cpp)
target_link_libraries(protocol_test protocol)

# 手动测试
add_executable(manual_test test/manual_test.cpp)
target_link_libraries(manual_test protocol)