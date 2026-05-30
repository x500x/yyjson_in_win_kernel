# yyjson KMDF 示例

[English](#english) | [中文](#yyjson-kmdf-示例)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE)
[![Build](https://img.shields.io/badge/Build-WDK%2010.0.22621-blue.svg)](https://learn.microsoft.com/zh-cn/windows-hardware/drivers/download-the-wdk)

一个独立的 KMDF 驱动示例，演示如何在 Windows 内核模式下集成 yyjson。本项目包含所有必要的源代码、兼容层和构建脚本。

## 功能特性

- **自包含**：所有依赖项都包含在项目目录中
- **完整示例**：JSON 解析、数组遍历、文档生成和文件 I/O
- **内核安全**：正确的内存管理和 IRQL 合规性
- **易于设置**：预配置的 Visual Studio 解决方案和安装脚本

## 前提条件

- Visual Studio 2022，包含 C++ 桌面开发工作负载
- Windows 驱动程序工具包 (WDK) 10.0.22621 或兼容版本
- 启用测试签名：`bcdedit /set testsigning on`
- 驱动安装需要管理员权限

## 快速开始

### 构建

```bash
# 打开解决方案
start example.sln

# 从命令行构建（可选）
msbuild example.sln /p:Configuration=Release /p:Platform=x64
```

### 安装并运行

```bash
# 以管理员身份运行
install_example.cmd
```

### 监控输出

- **WinDbg**：连接内核调试会话
- **DebugView**：启用内核捕获（Ctrl+K）

### 卸载

```bash
uninstall_example.cmd
```

## 项目结构

```
yyjson_kmdf_example/
├── example.sln                 # Visual Studio 解决方案
├── example/
│   ├── example.vcxproj         # 驱动项目文件
│   ├── driver/                 # 驱动实现
│   │   ├── driver.c           # 驱动入口点
│   │   └── example_sample.c   # JSON 操作示例
│   ├── compat/                 # 内核兼容层
│   │   ├── include/           # 头文件
│   │   └── src/               # 实现文件
│   ├── yyjson/                # 内置 yyjson 库
│   │   └── src/               # yyjson 源代码
│   └── third_party/           # 辅助库
│       └── double-conversion/ # IEEE 754 转换
├── install_example.cmd        # 驱动安装脚本
└── uninstall_example.cmd      # 驱动卸载脚本
```

## 示例

### 1. 内存中的 JSON 解析

```c
const char *json = "{\"name\":\"yyjson\",\"version\":1,\"enabled\":true,\"message\":\"hello kernel\"}";
yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
yyjson_val *root = yyjson_doc_get_root(doc);

const char *name = yyjson_get_str(yyjson_obj_get(root, "name"));
int64_t version = yyjson_get_int(yyjson_obj_get(root, "version"));
bool enabled = yyjson_get_bool(yyjson_obj_get(root, "enabled"));
```

### 2. 数组遍历

```c
yyjson_val *arr = yyjson_obj_get(root, "requests");
yyjson_val *item;
size_t idx, max;

yyjson_arr_foreach(arr, idx, max, item) {
    const char *op = yyjson_get_str(yyjson_obj_get(item, "op"));
    int64_t size = yyjson_get_int(yyjson_obj_get(item, "size"));
    // 处理每个元素
}
```

### 3. 可变文档创建

```c
yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
yyjson_mut_val *root = yyjson_mut_obj(doc);

yyjson_mut_obj_add_str(doc, root, "name", "example");
yyjson_mut_obj_add_int(doc, root, "version", 1);
yyjson_mut_obj_add_bool(doc, root, "enabled", true);
yyjson_mut_doc_set_root(doc, root);

char *json = yyjson_mut_write(doc, 0, NULL);
```

### 4. 文件 I/O

```c
// 写入文件
yyjson_mut_write_file("C:\\Windows\\Temp\\yyjson_kmdf_example.json", doc, 
                      YYJSON_WRITE_PRETTY, NULL, NULL);

// 从文件读取
yyjson_doc *doc = yyjson_read_file("C:\\Windows\\Temp\\yyjson_kmdf_example.json", 
                                   0, NULL, NULL);
```

**重要提示**：文件操作要求：
- `PASSIVE_LEVEL` IRQL 级别
- 绝对 DOS 路径或 NT 路径
- 正确的错误处理

## 构建配置

| 配置 | 说明 | 用途 |
|------|------|------|
| Debug\|x64 | 调试符号，无优化 | 开发和测试 |
| Release\|x64 | 优化代码 | 性能测试 |

## 调试

### 启用详细日志

```c
DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, 
           "[yyjson] 解析结果: %s\n", json_string);
```

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 驱动加载失败 | 测试签名已禁用 | 启用测试签名 |
| 无调试输出 | IRQL 级别错误 | 确保在 PASSIVE_LEVEL |
| 文件操作失败 | 路径无效 | 使用绝对路径 |
| 内存泄漏 | 缺少清理 | 调用 yyjson_doc_free() |

## API 参考

### 使用的核心函数

```c
// 文档管理
yyjson_doc *yyjson_read(const char *str, size_t len, yyjson_read_flag flg);
void yyjson_doc_free(yyjson_doc *doc);
yyjson_val *yyjson_doc_get_root(yyjson_doc *doc);

// 值访问
const char *yyjson_get_str(yyjson_val *val);
int64_t yyjson_get_int(yyjson_val *val);
bool yyjson_get_bool(yyjson_val *val);

// 对象操作
yyjson_val *yyjson_obj_get(yyjson_val *obj, const char *key);

// 数组操作
yyjson_val *yyjson_arr_get(yyjson_val *arr, size_t idx);
size_t yyjson_arr_size(yyjson_val *arr);

// 可变文档
yyjson_mut_doc *yyjson_mut_doc_new(const yyjson_alc *alc);
yyjson_mut_val *yyjson_mut_obj(yyjson_mut_doc *doc);
void yyjson_mut_obj_add_str(yyjson_mut_doc *doc, yyjson_mut_val *obj, 
                            const char *key, const char *val);
char *yyjson_mut_write(yyjson_mut_doc *doc, yyjson_write_flag flg, 
                       size_t *len);
```

## 许可证

本示例是 [yyjson_in_win_kernel](../README.md) 项目的一部分，采用 MIT 许可证。

---

# English

[English](#english) | [中文](#yyjson-kmdf-示例)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE)
[![Build](https://img.shields.io/badge/Build-WDK%2010.0.22621-blue.svg)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)

A standalone KMDF driver example demonstrating yyjson integration in Windows kernel mode. This project includes all necessary source code, compatibility layer, and build scripts.

## Features

- **Self-Contained**: All dependencies included in the project tree
- **Complete Examples**: JSON parsing, array traversal, document generation, and file I/O
- **Kernel-Safe**: Proper memory management and IRQL compliance
- **Easy Setup**: Pre-configured Visual Studio solution with installation scripts

## Prerequisites

- Visual Studio 2022 with C++ Desktop Development workload
- Windows Driver Kit (WDK) 10.0.22621 or compatible
- Test signing enabled: `bcdedit /set testsigning on`
- Administrator privileges for driver installation

## Quick Start

### Build

```bash
# Open solution
start example.sln

# Build from command line (optional)
msbuild example.sln /p:Configuration=Release /p:Platform=x64
```

### Install and Run

```bash
# Run as Administrator
install_example.cmd
```

### Monitor Output

- **WinDbg**: Connect to kernel debug session
- **DebugView**: Enable kernel capture (Ctrl+K)

### Unload

```bash
uninstall_example.cmd
```

## Project Structure

```
yyjson_kmdf_example/
├── example.sln                 # Visual Studio solution
├── example/
│   ├── example.vcxproj         # Driver project file
│   ├── driver/                 # Driver implementation
│   │   ├── driver.c           # Driver entry point
│   │   └── example_sample.c   # JSON operation examples
│   ├── compat/                 # Kernel compatibility layer
│   │   ├── include/           # Header files
│   │   └── src/               # Implementation files
│   ├── yyjson/                # Vendored yyjson library
│   │   └── src/               # yyjson source code
│   └── third_party/           # Helper libraries
│       └── double-conversion/ # IEEE 754 conversion
├── install_example.cmd        # Driver installation script
└── uninstall_example.cmd      # Driver removal script
```

## Examples

### 1. In-Memory JSON Parsing

```c
const char *json = "{\"name\":\"yyjson\",\"version\":1,\"enabled\":true,\"message\":\"hello kernel\"}";
yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
yyjson_val *root = yyjson_doc_get_root(doc);

const char *name = yyjson_get_str(yyjson_obj_get(root, "name"));
int64_t version = yyjson_get_int(yyjson_obj_get(root, "version"));
bool enabled = yyjson_get_bool(yyjson_obj_get(root, "enabled"));
```

### 2. Array Traversal

```c
yyjson_val *arr = yyjson_obj_get(root, "requests");
yyjson_val *item;
size_t idx, max;

yyjson_arr_foreach(arr, idx, max, item) {
    const char *op = yyjson_get_str(yyjson_obj_get(item, "op"));
    int64_t size = yyjson_get_int(yyjson_obj_get(item, "size"));
    // Process each element
}
```

### 3. Mutable Document Creation

```c
yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
yyjson_mut_val *root = yyjson_mut_obj(doc);

yyjson_mut_obj_add_str(doc, root, "name", "example");
yyjson_mut_obj_add_int(doc, root, "version", 1);
yyjson_mut_obj_add_bool(doc, root, "enabled", true);
yyjson_mut_doc_set_root(doc, root);

char *json = yyjson_mut_write(doc, 0, NULL);
```

### 4. File I/O

```c
// Write to file
yyjson_mut_write_file("C:\\Windows\\Temp\\yyjson_kmdf_example.json", doc, 
                      YYJSON_WRITE_PRETTY, NULL, NULL);

// Read from file
yyjson_doc *doc = yyjson_read_file("C:\\Windows\\Temp\\yyjson_kmdf_example.json", 
                                   0, NULL, NULL);
```

**Important**: File operations require:
- `PASSIVE_LEVEL` IRQL
- Absolute DOS paths or NT paths
- Proper error handling for file operations

## Build Configurations

| Configuration | Description | Use Case |
|---------------|-------------|----------|
| Debug\|x64 | Debug symbols, no optimization | Development and testing |
| Release\|x64 | Optimized code | Performance testing |

## Debugging

### Enable Verbose Logging

```c
DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, 
           "[yyjson] Parsing result: %s\n", json_string);
```

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Driver fails to load | Test signing disabled | Enable test signing |
| No debug output | Wrong IRQL level | Ensure PASSIVE_LEVEL |
| File operation fails | Invalid path | Use absolute paths |
| Memory leak | Missing cleanup | Call yyjson_doc_free() |

## API Reference

### Core Functions Used

```c
// Document management
yyjson_doc *yyjson_read(const char *str, size_t len, yyjson_read_flag flg);
void yyjson_doc_free(yyjson_doc *doc);
yyjson_val *yyjson_doc_get_root(yyjson_doc *doc);

// Value access
const char *yyjson_get_str(yyjson_val *val);
int64_t yyjson_get_int(yyjson_val *val);
bool yyjson_get_bool(yyjson_val *val);

// Object operations
yyjson_val *yyjson_obj_get(yyjson_val *obj, const char *key);

// Array operations
yyjson_val *yyjson_arr_get(yyjson_val *arr, size_t idx);
size_t yyjson_arr_size(yyjson_val *arr);

// Mutable document
yyjson_mut_doc *yyjson_mut_doc_new(const yyjson_alc *alc);
yyjson_mut_val *yyjson_mut_obj(yyjson_mut_doc *doc);
void yyjson_mut_obj_add_str(yyjson_mut_doc *doc, yyjson_mut_val *obj, 
                            const char *key, const char *val);
char *yyjson_mut_write(yyjson_mut_doc *doc, yyjson_write_flag flg, 
                       size_t *len);
```

## License

This example is part of the [yyjson_in_win_kernel](../README.md) project and is licensed under the MIT License.