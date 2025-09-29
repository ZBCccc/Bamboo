# Bamboo

Bamboo 是一个基于可搜索加密（SSE, Searchable Symmetric Encryption）的原型系统，包含客户端与服务端两部分，并提供完整的测试与调试工具集。项目使用 CMake 构建，依赖 RELIC、OpenSSL、GMP、SQLite3（客户端）以及 libpqxx/libpq（服务端）。

---

## 目录结构

```text
.
├── client/                     # 客户端逻辑与入口（RunSSEClient.cpp）
├── server/                     # 服务端逻辑与入口（RunSSEServer.cpp）
├── test/                       # 单元测试与调试工具（详见 test/README.md）
├── primitive.*                 # 核心加密/椭圆曲线相关原语
├── CMakeLists.txt              # 顶层构建配置，生成 Bamboo-CLIENT / Bamboo-SERVER
├── build/                      # 构建输出目录（建议外部构建）
├── sse_data/                   # 示例/基准数据目录
├── SEKU_clnt_data_sse_data_priv_key # 客户端本地密钥/状态文件（示例）
├── debug_point.cpp             # 调试文件（可选）
└── README.md                   # 本文档
```

- 主要可执行文件：
  - `Bamboo-CLIENT`：客户端，可进行 `Setup`、`DataUpdate`、`Search`、`KeyUpdate` 等操作（示例流程见 `client/RunSSEClient.cpp`）。
  - `Bamboo-SERVER`：服务端，监听 `127.0.0.1:54324`，处理客户端请求（见 `server/RunSSEServer.cpp` 与 `server/SSEServer.cpp`）。

- 测试与调试：位于 `test/`，包括 Google Test 用例与两个调试工具，详见 `test/README.md`。

---

## 依赖与环境

- C++17 编译器，CMake ≥ 3.10
- 必需库：
  - RELIC（`-lrelic`）
  - OpenSSL（`-lcrypto`）
  - GMP（`-lgmp`）
  - pthread（`-lpthread`）
  - SQLite3（客户端，`-lsqlite3`）
  - PostgreSQL C/C++（服务端，`pqxx`, `pq`）
- macOS（Apple Silicon/Intel）
  - 顶层 `CMakeLists.txt` 已为 Homebrew 安装路径做了适配：
    - ARM: `/opt/homebrew`
    - Intel: `/usr/local`
  - 若依赖通过 Homebrew 安装，CMake 会自动在上述前缀中查找头文件与库。

---

## 构建

建议使用 out-of-source 构建：

```bash
# 从项目根目录
mkdir -p build && cd build
cmake ..
make -j
```

生成的目标：

- `Bamboo-CLIENT`
- `Bamboo-SERVER`
- 以及 `test/` 子项目中的测试与调试可执行文件（见下文“测试与调试”）。

如需仅构建测试，请参考 `test/README.md`。

---

## 运行

### 1) 启动服务端

```bash
# 在 build/ 目录下
./Bamboo-SERVER
```

- 服务端默认绑定 `127.0.0.1:54324`（见 `server/RunSSEServer.cpp`）。
- 服务端内部支持操作：`Setup`、`SaveCipher`、`SrchQry`、`KeyUpdt`、`BackupEDB`、`LoadEDB`、`SaveBatch`、`ECDH`（详见 `server/SSEServer.cpp`）。

### 2) 运行客户端示例

在另一个终端中运行：

```bash
# 在 build/ 目录下
./Bamboo-CLIENT
```

客户端默认连接 `127.0.0.1:54324` 并执行示例流程（见 `client/RunSSEClient.cpp`）：

- `Setup()` 初始化
- 批量 `DataUpdate()` 插入若干标签-文档对
- `Search()` 检索
- `KeyUpdate()` 执行密钥轮换后再检索

若需要运行基准测试，可在 `client/RunSSEClient.cpp` 中将 `run_Benchmark()` 打开并注释/关闭 `TestClient()`（当前示例默认运行 `TestClient()`）。

---

## 测试与调试

详见 `test/README.md`，常用操作如下：

```bash
# 在 build/ 目录下（顶层构建会包含 test 子项目）
ctest --verbose                    # 运行所有测试
make run_gtest                     # 运行 Google Test 套件
make run_debug_tools               # 运行全部调试工具
./test/debug_pi_issue              # 单独运行 Pi 调试工具（路径以实际生成为准）
./test/debug_bamboo_search         # 单独运行 Bamboo 搜索调试工具
```

或在 `test/` 目录进行独立构建（参考 `test/README.md`）。

---

## 常见问题（FAQ）

- 编译期找不到库/头文件：
  - 确认依赖通过 Homebrew 安装，并位于 `/opt/homebrew`（ARM）或 `/usr/local`（Intel）。
  - 如自定义安装路径，请在顶层 `CMakeLists.txt` 中追加 `include_directories()` 与 `link_directories()`。
- 运行期连接失败：
  - 确保服务端已启动并监听 `127.0.0.1:54324`。
  - 检查本机防火墙或端口占用情况。
- 服务端数据库依赖：
  - 服务端链接 `pqxx`/`pq`，如未使用到持久化功能，确保库可用即可。
- 客户端本地状态：
  - 客户端使用 SQLite3 维护状态，生成的数据库位于 `build/bamboo_clnt_state.db`（见构建产物）。

---

## 许可

本项目用于学术与实验用途，具体许可以仓库 License 为准（如未提供，请联系作者或维护者）。
