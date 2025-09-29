# =============================================================================
# Bamboo Project - Test Suite CMakeLists.txt
# =============================================================================

cmake_minimum_required(VERSION 3.10)

# 设置测试项目名称
set(TEST_PROJECT_NAME "BambooTests")

# 包含父目录的头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# =============================================================================
# Google Test 配置
# =============================================================================
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTEST REQUIRED gtest_main gtest)
include_directories(${GTEST_INCLUDE_DIRS})

# 启用测试
enable_testing()

# =============================================================================
# 测试可执行文件配置
# =============================================================================

# Google Test 单元测试
add_executable(test_pi_gtest
    ../primitive.h
    ../primitive.cpp
    test_pi_functions_gtest.cpp
)

target_link_libraries(test_pi_gtest
    ${GTEST_LIBRARIES}
    -lrelic -lcrypto -lgmp -lpthread
)

# 添加测试到 CTest
include(GoogleTest)
gtest_discover_tests(test_pi_gtest)

# =============================================================================
# 自定义测试目标
# =============================================================================

# 运行所有测试的自定义目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test_pi_gtest
    COMMENT "Running all tests..."
)

# 运行 Google Test 的自定义目标
add_custom_target(run_gtest
    COMMAND $<TARGET_FILE:test_pi_gtest>
    DEPENDS test_pi_gtest
    COMMENT "Running Google Test suite..."
)

# =============================================================================
# 测试数据和清理
# =============================================================================

# 清理测试生成的文件
add_custom_target(clean_test_data
    COMMAND ${CMAKE_COMMAND} -E remove -f *.db *.log
    COMMENT "Cleaning test data files..."
)

# 完整清理（包括可执行文件）
add_custom_target(clean_all_tests
    COMMAND ${CMAKE_COMMAND} -E remove -f test_pi_gtest
    COMMAND ${CMAKE_COMMAND} -E remove -f *.db *.log
    COMMENT "Cleaning all test files..."
)

# =============================================================================
# 安装配置（可选）
# =============================================================================

# 如果需要安装测试工具到系统
# install(TARGETS test_pi_gtest
# RUNTIME DESTINATION bin/tests
# )

# =============================================================================
# 测试报告生成
# =============================================================================

# 生成测试报告的自定义目标
add_custom_target(test_report
    COMMAND echo "=== Bamboo Test Report ===" > test_report.txt
    COMMAND echo "Generated on: `date`" >> test_report.txt
    COMMAND echo "" >> test_report.txt
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose >> test_report.txt 2>&1 || true
    DEPENDS test_pi_gtest
    COMMENT "Generating test report..."
)

message(STATUS "Test suite configured successfully!")
message(STATUS "Available test targets:")
message(STATUS "  - test_pi_gtest: Google Test unit tests")
message(STATUS "  - run_all_tests: Run all tests via CTest")
message(STATUS "  - run_gtest: Run Google Test suite")
message(STATUS "  - test_report: Generate test report")
