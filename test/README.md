# Bamboo 项目测试套件

## 📋 概述

这个目录包含了 Bamboo 项目的所有测试和调试工具，采用模块化的组织结构。

## 🗂️ 文件结构

```
test/
├── CMakeLists.txt                 # 测试专用的 CMake 配置
├── README.md                      # 本文件
├── test_pi_functions_gtest.cpp    # Google Test 单元测试
├── debug_pi_issue.cpp             # Pi 函数调试工具
└── debug_bamboo_search.cpp        # Bamboo 搜索调试工具
```

## 🧪 测试程序

### 1. Google Test 单元测试 (`test_pi_functions_gtest.cpp`)
- **目的**: 全面的 Pi 函数单元测试
- **框架**: Google Test
- **测试数量**: 15 个测试用例
- **覆盖范围**: 
  - 基础功能测试
  - 边界条件测试
  - 椭圆曲线运算测试
  - 性能和并发测试
  - 错误处理测试

### 2. Pi 函数调试工具 (`debug_pi_issue.cpp`)
- **目的**: 调试 Pi 函数的映射和逆映射过程
- **功能**: 
  - 分析椭圆曲线点的二进制表示
  - 验证加密/解密过程
  - 诊断映射失败的原因

### 3. Bamboo 搜索调试工具 (`debug_bamboo_search.cpp`)
- **目的**: 调试 Bamboo 系统的搜索过程
- **功能**:
  - 模拟完整的加密/解密流程
  - 分析搜索结果的正确性
  - 验证椭圆曲线运算的一致性

## 🚀 构建和运行

### 构建所有测试
```bash
# 从项目根目录
mkdir -p build && cd build
cmake ..
make

# 或者只构建测试相关目标
cd test
mkdir -p build && cd build
cmake ..
make
```

### 运行测试

#### 1. 运行 Google Test 单元测试
```bash
# 方法1: 直接运行
./test_pi_gtest

# 方法2: 使用 CTest
ctest --verbose

# 方法3: 使用自定义目标
make run_gtest
```

#### 2. 运行调试工具
```bash
# Pi 函数调试
./debug_pi_issue

# Bamboo 搜索调试
./debug_bamboo_search

# 运行所有调试工具
make run_debug_tools
```

#### 3. 运行所有测试
```bash
make run_all_tests
```

### 生成测试报告
```bash
make test_report
cat test_report.txt
```

## 📊 测试目标

### 可用的 Make 目标

| 目标 | 描述 |
|------|------|
| `test_pi_gtest` | 构建 Google Test 单元测试 |
| `debug_pi_issue` | 构建 Pi 函数调试工具 |
| `debug_bamboo_search` | 构建 Bamboo 搜索调试工具 |
| `run_all_tests` | 运行所有测试 (CTest) |
| `run_gtest` | 运行 Google Test 套件 |
| `run_debug_tools` | 运行所有调试工具 |
| `test_report` | 生成测试报告 |
| `clean_test_data` | 清理测试数据文件 |
| `clean_all_tests` | 清理所有测试文件 |

## 🔧 开发指南

### 添加新的测试

1. **添加 Google Test 测试用例**:
   ```cpp
   TEST_F(PiFunctionTest, YourNewTest) {
       // 测试代码
   }
   ```

2. **添加新的调试工具**:
   - 创建新的 `.cpp` 文件
   - 在 `CMakeLists.txt` 中添加对应的可执行文件配置

3. **更新 CMakeLists.txt**:
   ```cmake
   add_executable(your_new_tool
       ../primitive.h
       ../primitive.cpp
       your_new_tool.cpp
   )
   target_link_libraries(your_new_tool
       -lrelic -lcrypto -lgmp -lpthread
   )
   ```

### 测试最佳实践

1. **命名规范**:
   - 测试文件: `test_*.cpp`
   - 调试工具: `debug_*.cpp`
   - 可执行文件: 与源文件同名

2. **测试组织**:
   - 按功能模块组织测试用例
   - 使用描述性的测试名称
   - 添加适当的注释和文档

3. **资源管理**:
   - 正确初始化和清理 RELIC 库
   - 使用 RAII 管理椭圆曲线点和大数
   - 避免内存泄漏

## 📈 测试覆盖率

当前测试覆盖的功能：
- ✅ Pi 函数基础映射
- ✅ Pi 函数逆映射
- ✅ 椭圆曲线加密/解密
- ✅ 线程安全性
- ✅ 错误处理
- ✅ 性能基准
- ✅ 边界条件
- ✅ 随机数据一致性

## 🐛 调试指南

### 常见问题

1. **测试失败**:
   - 检查 RELIC 库是否正确初始化
   - 验证椭圆曲线参数设置
   - 查看详细的错误信息

2. **性能问题**:
   - 使用性能测试分析瓶颈
   - 检查内存使用情况
   - 优化椭圆曲线运算

3. **内存问题**:
   - 使用 Valgrind 检测内存泄漏
   - 确保正确释放椭圆曲线点
   - 检查缓冲区边界

### 调试命令
```bash
# 使用 Valgrind 检测内存问题
valgrind --leak-check=full ./test_pi_gtest

# 使用 GDB 调试
gdb ./debug_pi_issue
```

## 📝 维护说明

- 定期运行所有测试确保代码质量
- 添加新功能时同步更新测试
- 保持测试文档的更新
- 监控测试性能和覆盖率
