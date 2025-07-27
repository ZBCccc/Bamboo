cmake_minimum_required(VERSION 3.10)
project(Bamboo)

set(CMAKE_CXX_STANDARD 17)

# 添加Homebrew路径支持
if(APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(HOMEBREW_PREFIX "/opt/homebrew")
    # 添加PostgreSQL特定路径
    set(PostgreSQL_ROOT "/opt/homebrew")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
    # 添加PostgreSQL库路径
    link_directories(/opt/homebrew/lib/postgresql@14)
else()
    set(HOMEBREW_PREFIX "/usr/local")
endif()

# 添加 relic 库的路径（优先使用 /usr/local）
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

# 添加其他依赖库的路径
include_directories(${HOMEBREW_PREFIX}/include)
link_directories(${HOMEBREW_PREFIX}/lib)

link_libraries(-lrelic -lcrypto -lgmp -lpthread)

add_executable(Bamboo-CLIENT
        primitive.h
        primitive.cpp
        client/BambooClient.cpp
        client/BambooClient.h
        client/ClientState.cpp
        client/ClientState.h
        client/SSEClient.cpp
        client/SSEClient.h
        client/Benchmark.h
        client/Benchmark.cpp
        client/RunSSEClient.cpp)
target_link_libraries(Bamboo-CLIENT
        -lsqlite3)

add_executable(Bamboo-SERVER
        primitive.h
        primitive.cpp
        server/BambooServer.cpp
        server/BambooServer.h
        server/ServerStorage.cpp
        server/SSEServer.cpp
        server/SSEServer.h
        server/RunSSEServer.cpp)
target_link_libraries(Bamboo-SERVER
        -lpthread
        pqxx
        pq)

# =============================================================================
# 测试子项目
# =============================================================================
# 添加测试子目录
add_subdirectory(test)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")