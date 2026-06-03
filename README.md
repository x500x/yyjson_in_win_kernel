# yyjson Windows 内核移植

[English](README_en.md) | [中文](README.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build](https://img.shields.io/badge/Build-WDK%2010.0.22621-blue.svg)](https://learn.microsoft.com/zh-cn/windows-hardware/drivers/download-the-wdk)
[![Platform](https://img.shields.io/badge/Platform-Windows%20Kernel%20(x64%2FARM64)-green.svg)](https://docs.microsoft.com/zh-cn/windows-hardware/drivers/kernel/)

将 [yyjson](https://github.com/ibireme/yyjson)（高性能 JSON 库）完整移植到 Windows 内核模式驱动框架 (KMDF)。本项目提供了一个最小化的兼容层，用于桥接 yyjson 的 C 运行时依赖与内核模式 API。

## 功能特性

- **完整的 yyjson API 支持**：在内核模式下解析、查询、修改和生成 JSON 文档
- **内核兼容的 C 运行时**：为内核环境自定义实现 `stdio.h`、`math.h`、`locale.h` 和 `assert.h`
- **文件 I/O 支持**：从内核空间读写 JSON 文件（需要 `PASSIVE_LEVEL`）
- **内存安全**：使用内核池分配并正确清理
- **KMDF 集成**：通过 WDF 回调实现正确的驱动生命周期管理
- **静态库 + 示例结构**：`yyjson_kmdf_lib.lib` 提供可复用内核库，库工程支持 `x64`/`ARM64`，示例驱动单独链接使用

## 前提条件

- **Visual Studio 2022**（或更高版本），包含 C++ 桌面开发工作负载
- **Windows 驱动程序工具包 (WDK)** 10.0.22621（或兼容版本）
- **ARM64 构建工具**：仅在需要生成 `ARM64` 静态库时安装对应 VS/WDK 组件
- **启用测试签名**：`bcdedit /set testsigning on`
- **内核调试设置**（可选但推荐）：WinDbg 或 DebugView

## 快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/your-username/yyjson_in_win_kernel.git
cd yyjson_in_win_kernel
```

### 2. 构建库和示例驱动

```bash
# 在 Visual Studio 中打开解决方案
start yyjson_kmdf_example\example.sln

# 或者从命令行构建，solution 会先构建 yyjson_kmdf_lib.lib 再构建 example.sys
msbuild yyjson_kmdf_example\example.sln /p:Configuration=Release /p:Platform=x64
```

### 3. 安装并运行

```bash
# 以管理员身份运行
yyjson_kmdf_example\install_example.cmd
```

### 4. 查看输出

使用以下方式监控调试输出：
- **WinDbg**：连接内核调试会话
- **DebugView**：启用内核捕获（Ctrl+K）

### 5. 卸载驱动

```bash
yyjson_kmdf_example\uninstall_example.cmd
```

## 项目结构

```
yyjson_in_win_kernel/
├── yyjson_kmdf_lib/                # 可复用 KMDF 静态库
│   ├── yyjson_kmdf_lib.vcxproj     # 生成 yyjson_kmdf_lib.lib
│   ├── build-all-abi.ps1           # 批量生成 x64/ARM64 静态库
│   ├── include/                    # 库头文件与内核 shim 头
│   │   ├── yyjson.h                # yyjson 公共 API
│   │   ├── yyjsonk_runtime.h       # 内核运行时初始化/日志/文件 I/O API
│   │   ├── stdio.h                 # 文件 I/O shim
│   │   ├── math.h                  # 数学 shim
│   │   ├── locale.h                # 区域设置 shim
│   │   └── assert.h                # 调试断言 shim
│   ├── src/                        # yyjson 源文件
│   ├── compat/src/                 # 内核兼容层实现
│   └── third_party/                # double-conversion 辅助库
│
├── yyjson_kmdf_example/            # 链接静态库的示例驱动
│   ├── example.sln                 # Visual Studio 解决方案
│   ├── example/                    # 驱动项目
│   │   └── driver/                 # 示例驱动代码
│   ├── install_example.cmd         # 驱动安装脚本
│   └── uninstall_example.cmd       # 驱动卸载脚本
│
├── yyjson_kmdf_tests/              # 链接 yyjson_kmdf_lib 的测试驱动
│   ├── yyjson_kmdf_tests.sln       # 测试驱动解决方案，包含库工程引用
│   ├── compat/                     # 仅保留上游测试所需的辅助代码
│   └── driver/                     # 测试驱动入口和测试执行器
│
├── LICENSE                         # MIT 许可证
└── README.md                       # 英文文档
└── README_zh.md                    # 本文档
```

## 示例说明

示例驱动 (`example.sys`) 演示以下功能：

### 1. 内存中的 JSON 解析
```c
// 从字符串解析 JSON
yyjson_doc *doc = yyjson_read(json_str, len, 0);
yyjson_val *root = yyjson_doc_get_root(doc);

// 读取值
const char *name = yyjson_get_str(yyjson_obj_get(root, "name"));
int64_t version = yyjson_get_int(yyjson_obj_get(root, "version"));
bool enabled = yyjson_get_bool(yyjson_obj_get(root, "enabled"));
```

### 2. 数组遍历
```c
// 遍历 JSON 数组
yyjson_val *arr = yyjson_obj_get(root, "requests");
yyjson_val *item;
yyjson_arr_foreach(arr, idx, max, item) {
    const char *op = yyjson_get_str(yyjson_obj_get(item, "op"));
    int64_t size = yyjson_get_int(yyjson_obj_get(item, "size"));
    // 处理每个元素...
}
```

### 3. 可变文档生成
```c
// 创建可变文档
yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
yyjson_mut_val *root = yyjson_mut_obj(doc);

// 添加值
yyjson_mut_obj_add_str(doc, root, "name", "example");
yyjson_mut_obj_add_int(doc, root, "version", 1);
yyjson_mut_doc_set_root(doc, root);

// 序列化为 JSON 字符串
const char *json = yyjson_mut_write(doc, 0, NULL);
```

### 4. 文件 I/O 操作
```c
// 将 JSON 写入文件
yyjson_mut_write_file(path, doc, YYJSON_WRITE_PRETTY, NULL, NULL);

// 从文件读取 JSON
yyjson_doc *doc = yyjson_read_file(path, 0, NULL, NULL);
```

**注意**：文件操作要求：
- `PASSIVE_LEVEL` IRQL 级别
- 使用绝对 DOS 路径（例如 `C:\Windows\Temp\file.json`）或 NT 路径

## 从源码构建

### 构建静态库

`yyjson_kmdf_lib` 支持 `Debug|x64`、`Release|x64`、`Debug|ARM64`、`Release|ARM64`。WDK 10 的内核驱动项目不接受 `Win32` 作为有效架构。

```powershell
# 生成 Debug/Release 的 x64 和 ARM64 静态库
pwsh -File .\yyjson_kmdf_lib\build-all-abi.ps1

# 只生成 Release
pwsh -File .\yyjson_kmdf_lib\build-all-abi.ps1 -Configuration Release

# 只生成 ARM64
pwsh -File .\yyjson_kmdf_lib\build-all-abi.ps1 -Platform ARM64
```

输出路径：

```text
yyjson_kmdf_lib\bin\<Platform>\<Configuration>\yyjson_kmdf_lib.lib
```

### 方式一：Visual Studio IDE（示例驱动）

1. 打开 `yyjson_kmdf_example\example.sln`
2. 选择配置：`Debug|x64` 或 `Release|x64`
3. 生成解决方案（Ctrl+Shift+B）
4. 输出：`x64\Release\example.sys`

### 方式二：命令行（示例驱动）

```bash
# 设置构建环境
"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

# 构建
msbuild yyjson_kmdf_example\example.sln /p:Configuration=Release /p:Platform=x64 /t:Build
```

### 构建配置

| 目标 | 配置 | 说明 |
|------|------|------|
| 静态库 | `Debug|x64` / `Debug|ARM64` | 调试版本，包含符号，无优化 |
| 静态库 | `Release|x64` / `Release|ARM64` | 发布版本，启用优化 |
| 示例/测试驱动 | `Debug|x64` / `Release|x64` | 当前驱动工程配置 |

## API 参考

### 核心 yyjson 函数

| 函数 | 说明 |
|------|------|
| `yyjson_read()` | 从字符串解析 JSON |
| `yyjson_doc_get_root()` | 从文档获取根值 |
| `yyjson_get_str()` | 获取字符串值 |
| `yyjson_get_int()` | 获取整数值 |
| `yyjson_get_bool()` | 获取布尔值 |
| `yyjson_obj_get()` | 通过键获取对象成员 |
| `yyjson_arr_foreach()` | 遍历数组元素 |

### 可变文档函数

| 函数 | 说明 |
|------|------|
| `yyjson_mut_doc_new()` | 创建新的可变文档 |
| `yyjson_mut_obj()` | 创建可变对象 |
| `yyjson_mut_obj_add_str()` | 向对象添加字符串 |
| `yyjson_mut_obj_add_int()` | 向对象添加整数 |
| `yyjson_mut_write()` | 序列化为 JSON 字符串 |

### 文件 I/O 函数

| 函数 | 说明 |
|------|------|
| `yyjson_read_file()` | 从文件读取 JSON |
| `yyjson_mut_write_file()` | 将 JSON 写入文件 |

### 内核兼容层

| 头文件 | 提供的函数 |
|--------|-----------|
| `stdio.h` | `fopen`, `fread`, `fwrite`, `fclose`, `fseek`, `ftell` |
| `math.h` | `pow`, `log10`, `ceil`, `floor`, `fabs` |
| `locale.h` | `setlocale`, `localeconv` |
| `assert.h` | `assert`（仅调试版本） |

## 调试指南

### 启用调试输出

```c
// 在驱动代码中
DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, 
           "JSON 解析结果: %s\n", json_str);
```

### 查看调试输出

**WinDbg**:
```
ed nt!Kd_DEFAULT_Mask 0xFFFFFFFF
g
```

**DebugView**:
1. 以管理员身份运行
2. 启用"捕获内核"（Ctrl+K）
3. 按进程名过滤

### 常见调试场景

| 问题 | 解决方案 |
|------|----------|
| 无输出 | 检查 IRQL 级别，确保在 `PASSIVE_LEVEL` |
| 访问冲突 | 验证内存分配，检查缓冲区大小 |
| 文件未找到 | 使用绝对路径，检查文件权限 |

## 贡献指南

1. Fork 本仓库
2. 创建功能分支：`git checkout -b feature/amazing-feature`
3. 提交更改：`git commit -m '添加某项功能'`
4. 推送到分支：`git push origin feature/amazing-feature`
5. 创建 Pull Request

### 开发规范

- 遵循内核编码标准（SAL 注解、正确的错误处理）
- 在多个 Windows 版本上测试（10、11）
- 更新 API 更改的文档
- 为新功能添加单元测试

## 许可证

本项目采用 MIT 许可证 - 详情请参阅 [LICENSE](LICENSE) 文件。

## 致谢

- [yyjson](https://github.com/ibireme/yyjson) - ibireme 开发的高性能 JSON 库
- [double-conversion](https://github.com/google/double-conversion) - IEEE 754 双精度浮点数转换
- [Windows Driver Samples](https://github.com/microsoft/Windows-driver-samples) - KMDF 参考实现

## 支持

- **问题反馈**：[GitHub Issues](https://github.com/x500x/yyjson_in_win_kernel/issues)
- **讨论交流**：[GitHub Discussions](https://github.com/x500x/yyjson_in_win_kernel/discussions)
