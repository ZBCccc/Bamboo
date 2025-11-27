cmake_minimum_required(VERSION 3.10)
project(BambooPoseidon)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories to build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(HOMEBREW_PREFIX "/opt/homebrew")
    set(PostgreSQL_ROOT "/opt/homebrew")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
    link_directories(/opt/homebrew/lib/postgresql@14)
else()
    set(HOMEBREW_PREFIX "/usr/local")
endif()

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

include_directories(${HOMEBREW_PREFIX}/include)
link_directories(${HOMEBREW_PREFIX}/lib)

# Common libraries
link_libraries(-lrelic -lcrypto -lgmp -lpthread)

# Options
option(ENABLE_BAMBOO "Build Bamboo targets" ON)
option(ENABLE_POSEIDON "Build Poseidon targets" OFF)

# Add subdirectories
add_subdirectory(src/core)

if(ENABLE_BAMBOO)
  add_subdirectory(src/bamboo)
endif()

if(ENABLE_POSEIDON)
  add_subdirectory(src/poseidon)
endif()
