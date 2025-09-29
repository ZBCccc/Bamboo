# Bamboo 项目测试与调试套件

本目录包含 Bamboo 项目的单元测试与调试程序，使用 CMake 构建，并基于 Google Test 与 RELIC/OpenSSL/GMP 等依赖。

---

## 目录结构

```text
test/
├── CMakeLists.txt                 # 测试与调试的 CMake 配置
├── README.md                      # 本文件
├── test_pi_functions_gtest.cpp    # Google Test 单元测试（pi / pi_inv 等）
├── debug_pi_issue.cpp             # pi / pi_inv 逐步调试工具
└── debug_bamboo_search.cpp        # Bamboo 搜索流程调试工具
```

---

## 依赖

- 编译环境：C++17、CMake ≥ 3.10
- 第三方库（通过顶层 CMakeLists 自动链接/查找）
  - RELIC（`-lrelic`）
  - OpenSSL（`-lcrypto`）
  - GMP（`-lgmp`）
  - pthread（`-lpthread`）
  - Google Test（通过 pkg-config：`gtest`, `gtest_main`）
- macOS（ARM/Intel）
  - Homebrew 前缀自动适配：ARM `/opt/homebrew`，Intel `/usr/local`

如依赖安装在自定义路径，请在顶层 `CMakeLists.txt` 增加包含/库目录。

---

## 构建

推荐从项目根目录进行统一构建（会包含 test 子项目）：

```bash
# 从项目根目录
mkdir -p build && cd build
cmake ..
make -j
```

生成内容包含：

- 顶层可执行文件：`Bamboo-CLIENT`, `Bamboo-SERVER`
- 测试与调试可执行文件：`test_pi_gtest`, `debug_pi_issue`, `debug_bamboo_search`

也可以在 `test/` 目录单独构建（独立 out-of-source）：

```bash
# 方式一：在顶层 build 已存在时，直接使用顶层
cd build && make -j

# 方式二：在 test 下单独构建
cd test
mkdir -p build && cd build
cmake ..
make -j
```

> 提示：若在 `test/build` 下独立构建，生成的测试/调试可执行文件位于 `test/build/` 目录。

---

## 运行

以下命令假设在顶层 `build/` 目录构建：

### 1. 运行 Google Test 单元测试

```bash
# 方法1：直接运行可执行文件
./test_pi_gtest

# 方法2：使用 CTest（会自动发现 gtest 测试）
ctest --verbose

# 方法3：使用自定义目标（在构建系统内）
make run_gtest
```

### 2. 运行调试工具

```bash
# 调试 pi / pi_inv
./debug_pi_issue

# 调试 Bamboo 搜索流程
./debug_bamboo_search

# 一次性运行所有调试工具
make run_debug_tools
```

> 如果是在 `test/build` 下独立构建，请相应地将可执行文件路径替换为 `./test_pi_gtest`、`./debug_pi_issue`、`./debug_bamboo_search`（位于 `test/build` 目录）。

---

## 自定义目标与清理

- 可用自定义目标（在构建目录内通过 `make <target>` 调用）：

| 目标 | 描述 |
|------|------|
| `run_all_tests` | 通过 CTest 运行所有测试（依赖 `test_pi_gtest`） |
| `run_gtest` | 直接运行 Google Test 可执行文件 |
| `run_debug_tools` | 运行 `debug_pi_issue` 与 `debug_bamboo_search` |
| `test_report` | 生成测试报告到 `test_report.txt` |
| `clean_test_data` | 删除测试生成的数据文件（如 `*.db`, `*.log`） |
| `clean_all_tests` | 删除测试可执行文件与数据文件 |

---

## 常见问题（FAQ）

- Google Test 找不到/链接错误：
  - 确认安装了 `gtest` 与 `gtest_main`，并且 `pkg-config` 可找到；必要时设置 `PKG_CONFIG_PATH`。
- 运行时报 `core_init`/RELIC 相关错误：
  - 测试与调试程序内部已调用 `core_init()` 与 `ep_param_set(NIST_P256)`，如仍报错，检查 RELIC 安装或 OpenSSL/GMP 版本。
- 可执行文件路径不一致：
  - 若在顶层构建，测试/调试可执行文件位于顶层 `build/`。
  - 若在 `test` 目录独立构建，产物位于 `test/build/`。
- 生成测试报告为空或失败：
  - 确保先成功构建 `test_pi_gtest`，再执行 `make test_report`。

---

## 开发建议

- 为新调试工具添加可重复的、逐步打印输出，便于问题定位。
- 添加新的单元测试：
  - 在 `test_pi_functions_gtest.cpp` 中新增 `TEST_F` 用例，或创建新 `test_*.cpp` 并在 `CMakeLists.txt` 注册。
- 椭圆曲线资源管理：
  - 使用 `ep_new/ep_free`、`bn_new/bn_free` 成对管理，避免泄漏。
- RELIC 运行时：
  - 初始化（`core_init`）后设置曲线参数（`ep_param_set(NIST_P256)`），在程序结束前统一 `core_clean`（已在测试中通过 `GlobalCleanup` 处理）。
