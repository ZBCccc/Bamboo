cmake_minimum_required(VERSION 3.10)
project(Bamboo)

set(CMAKE_CXX_STANDARD 17)

link_libraries(-lrelic -lcrypto -lgmp -lpthread)
link_directories(/usr/local/lib)

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
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")