add_library(core STATIC
    primitive.h
    primitive.cpp
)

target_link_libraries(core
    pqxx
    pq
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
