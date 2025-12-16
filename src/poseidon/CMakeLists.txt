add_library(poseidon_client STATIC
    client/PoseidonClient.h
    client/PoseidonClient.cpp
    client/PoseidonClientState.h
    client/PoseidonClientStateMemory.cpp
    client/PoseidonClientStateMemory.h
    client/SSEClient.cpp
    client/SSEClient.h
    types/metadata.h
    tools/Benchmark.h
    tools/Benchmark.cpp
)

target_link_libraries(poseidon_client
    core
)

target_include_directories(poseidon_client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/types
)

add_library(poseidon_server STATIC
    server/PoseidonServer.h
    server/PoseidonServer.cpp
    server/SSEServer.cpp
    server/SSEServer.h
    types/metadata.h
    storage/ServerStorage.h
    storage/ServerStorage.cpp
    storage/impl/ServerStorageMemory.h
    storage/impl/ServerStorageMemory.cpp
    tools/Benchmark.h
    tools/Benchmark.cpp
)

target_link_libraries(poseidon_server
    core
)

target_include_directories(poseidon_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/types
)

add_executable(Poseidon-CLIENT
    tools/RunSSEClient.cpp
)

target_link_libraries(Poseidon-CLIENT
    poseidon_client
)

add_executable(Poseidon-SERVER
    tools/RunSSEServer.cpp
)

target_link_libraries(Poseidon-SERVER
    poseidon_server
)

add_executable(Poseidon-TEST
    tools/TestPoseidon_detailed.cpp
)

target_link_libraries(Poseidon-TEST
    poseidon_client
    poseidon_server
)
