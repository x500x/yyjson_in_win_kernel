# yyjson in Windows Kernel

## 中文

`yyjson_in_win_kernel` 演示如何在 Windows KMDF 内核驱动中集成 yyjson，并提供一组最小兼容层来补齐 yyjson 依赖的 C 运行时能力。

### 目录结构

- `yyjson_win_kernel/`: 面向 yyjson 内核适配的 KMDF 工程和测试入口。
- `yyjson_kmdf_example/`: 独立 KMDF 示例驱动，直接随工程携带 yyjson 源码和兼容层。
- `LICENSE`: 仓库许可证。

### 示例覆盖

`yyjson_kmdf_example` 当前包含这些运行示例：

- 从内存字符串解析 JSON，并读取字符串、整数、布尔值。
- 遍历 JSON 数组对象并统计状态与字节数。
- 构造可变 JSON 文档并格式化输出。
- 使用 yyjson 文件 API 写入并读取 JSON 文件。

文件示例默认写入：

```text
C:\Windows\Temp\yyjson_kmdf_example.json
```

内核文件 API 只应在 `PASSIVE_LEVEL` 调用，并建议使用绝对 DOS 路径或 NT 路径。

### 构建

1. 安装 Visual Studio 和 Windows Driver Kit。
2. 打开 `yyjson_kmdf_example\example.sln`。
3. 选择 `Debug|x64` 或 `Release|x64`。
4. 构建后生成 `example.sys`。

### 运行

请在测试签名和内核调试环境中运行驱动，并使用管理员权限执行：

```cmd
yyjson_kmdf_example\install_example.cmd
```

驱动会通过 `DbgPrintEx` 输出解析、生成和文件读写日志，可在 WinDbg 或 DebugView 中查看。

卸载示例驱动：

```cmd
yyjson_kmdf_example\uninstall_example.cmd
```

## English

`yyjson_in_win_kernel` demonstrates how to integrate yyjson into a Windows KMDF kernel driver. It also includes a small compatibility layer for the C runtime pieces yyjson expects.

### Layout

- `yyjson_win_kernel/`: KMDF project and test entry points for the yyjson kernel port.
- `yyjson_kmdf_example/`: Standalone KMDF example driver with vendored yyjson sources and compatibility code.
- `LICENSE`: Repository license.

### Example Coverage

`yyjson_kmdf_example` currently demonstrates:

- Parsing JSON from an in-memory string and reading string, integer, and boolean fields.
- Traversing an array of JSON objects and summarizing status and byte counts.
- Building a mutable JSON document and writing formatted JSON.
- Writing and reading JSON through yyjson file APIs.

The file example writes to:

```text
C:\Windows\Temp\yyjson_kmdf_example.json
```

Kernel file APIs should only be called at `PASSIVE_LEVEL`, and callers should prefer absolute DOS paths or NT paths.

### Build

1. Install Visual Studio and the Windows Driver Kit.
2. Open `yyjson_kmdf_example\example.sln`.
3. Select `Debug|x64` or `Release|x64`.
4. Build the solution to produce `example.sys`.

### Run

Run the driver only in a test-signing and kernel-debugging environment. From an elevated prompt:

```cmd
yyjson_kmdf_example\install_example.cmd
```

The driver logs parsing, generation, and file I/O output through `DbgPrintEx`; view it in WinDbg or DebugView.

Unload the example driver with:

```cmd
yyjson_kmdf_example\uninstall_example.cmd
```
