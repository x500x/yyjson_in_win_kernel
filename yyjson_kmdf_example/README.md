# yyjson KMDF Example / yyjson KMDF 示例

## 中文

这个目录包含一个独立的 KMDF 驱动解决方案。工程把 yyjson、内核兼容层和示例代码放在同一棵目录中，构建产物为 `example.sys`。

### 目录结构

- `example.sln`: Visual Studio 解决方案。
- `example\example.vcxproj`: 单个 KMDF 驱动工程。
- `example\driver`: 驱动入口和 yyjson 示例代码。
- `example\compat`: yyjson 在内核模式下需要的兼容层。
- `example\yyjson\src`: 随示例工程携带的 yyjson 核心源码。
- `example\third_party`: 兼容层使用的 double-conversion 辅助代码。
- `install_example.cmd`: 创建并启动 `example` 内核服务。
- `uninstall_example.cmd`: 停止并删除 `example` 内核服务。

### 示例内容

驱动加载后会依次运行：

- 内存 JSON 解析示例：读取 `name`、`version`、`enabled` 和 `message` 字段。
- 数组遍历示例：遍历 `requests` 数组，输出每个操作并统计成功数量和字节数。
- 可变文档生成示例：构造对象、数组和基础类型，并输出格式化 JSON。
- 文件读写示例：用 `yyjson_mut_write_file` 写入 JSON，再用 `yyjson_read_file` 读回并验证字段。

文件示例默认写入：

```text
C:\Windows\Temp\yyjson_kmdf_example.json
```

文件 API 依赖兼容层中的 `fopen`、`fread`、`fwrite` 和 `fclose`，只能在 `PASSIVE_LEVEL` 使用。路径应使用绝对 DOS 路径或 NT 路径。

### 构建

1. 安装 Visual Studio 和 Windows Driver Kit。
2. 打开 `example.sln`。
3. 选择 `Debug|x64` 或 `Release|x64`。
4. 构建后会生成 `example.sys`。

### 运行与卸载

请在测试签名和内核调试环境中运行，并用管理员权限执行：

```cmd
install_example.cmd
```

日志通过 `DbgPrintEx` 输出，可在 WinDbg 或 DebugView 中查看。

卸载驱动：

```cmd
uninstall_example.cmd
```

## English

This directory contains a standalone KMDF driver solution. The project keeps yyjson, the kernel compatibility layer, and the sample code in one tree and builds `example.sys`.

### Layout

- `example.sln`: Visual Studio solution.
- `example\example.vcxproj`: Single KMDF driver project.
- `example\driver`: Driver entry point and yyjson sample code.
- `example\compat`: Compatibility layer required by yyjson in kernel mode.
- `example\yyjson\src`: Vendored yyjson core sources.
- `example\third_party`: double-conversion helper code used by the compatibility layer.
- `install_example.cmd`: Creates and starts the `example` kernel service.
- `uninstall_example.cmd`: Stops and deletes the `example` kernel service.

### Examples

When the driver loads, it runs:

- In-memory JSON parsing: reads `name`, `version`, `enabled`, and `message`.
- Array traversal: walks a `requests` array, logs each operation, and summarizes success and byte counts.
- Mutable document generation: builds objects, arrays, primitive values, and formatted JSON output.
- File I/O: writes JSON through `yyjson_mut_write_file`, then reads it back through `yyjson_read_file`.

The file example writes to:

```text
C:\Windows\Temp\yyjson_kmdf_example.json
```

The file APIs use the compatibility layer's `fopen`, `fread`, `fwrite`, and `fclose` implementations. They must run at `PASSIVE_LEVEL`; paths should be absolute DOS paths or NT paths.

### Build

1. Install Visual Studio and the Windows Driver Kit.
2. Open `example.sln`.
3. Select `Debug|x64` or `Release|x64`.
4. Build the solution to produce `example.sys`.

### Run and Unload

Run in a test-signing and kernel-debugging environment from an elevated prompt:

```cmd
install_example.cmd
```

Logs are emitted through `DbgPrintEx` and can be viewed in WinDbg or DebugView.

Unload the driver:

```cmd
uninstall_example.cmd
```
