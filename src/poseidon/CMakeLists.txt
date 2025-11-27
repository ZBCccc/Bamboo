add_library(poseidon_client STATIC
    client/PoseidonClient.h
    client/PoseidonClient.cpp
    client/PoseidonClientState.h
    client/PoseidonClientStateMemory.cpp
    client/PoseidonClientStateMemory.h
    client/SSEClient.cpp
    client/SSEClient.h
    core/metadata.h
)

target_link_libraries(poseidon_client
    core
)

target_include_directories(poseidon_client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

add_library(poseidon_server STATIC
    server/PoseidonServer.h
    server/PoseidonServer.cpp
    storage/ServerStorage.h
    storage/impl/ServerStorageMemory.h
    storage/impl/ServerStorageMemory.cpp
)

target_link_libraries(poseidon_server
    core
)

target_include_directories(poseidon_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

# Optional app components (benchmarks)
add_library(poseidon_app STATIC
    app/Benchmark.h
    app/Benchmark.cpp
)

target_link_libraries(poseidon_app
    poseidon_client
)

target_include_directories(poseidon_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)
