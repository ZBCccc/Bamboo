add_library(bamboo_client STATIC
    client/BambooClient.h
    client/BambooClient.cpp
    client/ClientState.h
    client/ClientStateSQL.cpp
    client/ClientStateSQL.h
    client/ClientStateMemory.cpp
    client/ClientStateMemory.h
    client/SSEClient.cpp
    client/SSEClient.h
    client/Benchmark.h
    client/Benchmark.cpp
)

target_link_libraries(bamboo_client
    core
    sqlite3
)

target_include_directories(bamboo_client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

add_library(bamboo_server STATIC
    server/BambooServer.h
    server/BambooServer.cpp
    server/SSEServer.cpp
    server/SSEServer.h
    storage/ServerStorage.h
    storage/ServerStorage.cpp
    storage/impl/ServerStorageMemory.h
    storage/impl/ServerStorageMemory.cpp
    storage/impl/ServerStorageSQL.h
    storage/impl/ServerStorageSQL.cpp
)

target_link_libraries(bamboo_server
    core
    pqxx
    pq
)

target_include_directories(bamboo_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

add_executable(Bamboo-CLIENT
    client/RunSSEClient.cpp
)

target_link_libraries(Bamboo-CLIENT
    bamboo_client
)

add_executable(Bamboo-SERVER
    server/RunSSEServer.cpp
)

target_link_libraries(Bamboo-SERVER
    bamboo_server
)
