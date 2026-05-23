cmake_minimum_required(VERSION 3.10)
project(ChatServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(MUDUO_INCLUDE_DIR muduo/net/EventLoop.h)
find_library(MUDUO_BASE muduo_base)
find_library(MUDUO_NET muduo_net)

find_path(MYSQL_INCLUDE_DIR mysql/mysql.h)
find_library(MYSQL_LIBRARY mysqlclient)

find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY hiredis)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/protocol
    ${PROJECT_SOURCE_DIR}/store
    ${PROJECT_SOURCE_DIR}/service
    ${PROJECT_SOURCE_DIR}/net
    ${MUDUO_INCLUDE_DIR}
    ${MYSQL_INCLUDE_DIR}
    ${HIREDIS_INCLUDE_DIR}
)

add_library(protocol STATIC protocol/message.cpp)

add_library(common STATIC
    src/MySQLConnectionPool.cpp
    src/DBExecutor.cpp
    src/RedisManager.cpp
)

add_library(store STATIC
    store/MysqlUserStore.cpp
    store/RedisUserStore.cpp
    store/RedisChatStore.cpp
)

add_library(service STATIC
    service/AuthService.cpp
    service/ChatService.cpp
)

add_library(net STATIC
    net/ChatServer.cpp
)

add_executable(chat_server main.cpp)
target_link_libraries(chat_server
    net
    service
    store
    protocol
    common
    ${MUDUO_BASE}
    ${MUDUO_NET}
    ${MYSQL_LIBRARY}
    ${HIREDIS_LIBRARY}
    pthread
)

add_executable(client src/client.cpp)
target_link_libraries(client
    protocol
    ${MUDUO_BASE}
    ${MUDUO_NET}
    pthread
)

add_executable(benchmark benchmark/benchmark.cpp)
target_link_libraries(benchmark
    protocol
    ${MUDUO_BASE}
    ${MUDUO_NET}
    pthread
)