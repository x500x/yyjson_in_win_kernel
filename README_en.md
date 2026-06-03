# yyjson in Windows Kernel

[English](README_en.md) | [中文](README.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build](https://img.shields.io/badge/Build-WDK%2010.0.22621-blue.svg)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)
[![Platform](https://img.shields.io/badge/Platform-Windows%20Kernel%20(x64)-green.svg)](https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/)

A complete port of [yyjson](https://github.com/ibireme/yyjson) (high-performance JSON library) to Windows Kernel-Mode Driver Framework (KMDF). This project provides a minimal compatibility layer that bridges yyjson's C runtime dependencies with kernel-mode APIs.

## Features

- **Full yyjson API Support**: Parse, query, modify, and generate JSON documents in kernel mode
- **Kernel-Compatible C Runtime**: Custom implementations of `stdio.h`, `math.h`, `locale.h`, and `assert.h` for kernel environment
- **File I/O Support**: Read/write JSON files from kernel space (requires `PASSIVE_LEVEL`)
- **Memory Safety**: Uses kernel pool allocations with proper cleanup
- **KMDF Integration**: Proper driver lifecycle management with WDF callbacks
- **Static Library + Example Structure**: `yyjson_kmdf_lib.lib` provides the reusable kernel library, and the example driver links against it

## Prerequisites

- **Visual Studio 2022** (or later) with C++ Desktop Development workload
- **Windows Driver Kit (WDK)** 10.0.22621 (or compatible version)
- **Test Signing Enabled**: `bcdedit /set testsigning on`
- **Kernel Debugging Setup** (optional but recommended): WinDbg or DebugView

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/your-username/yyjson_in_win_kernel.git
cd yyjson_in_win_kernel
```

### 2. Build the Library and Example Driver

```bash
# Open the solution in Visual Studio
start yyjson_kmdf_example\example.sln

# Or build from command line. The solution builds yyjson_kmdf_lib.lib before example.sys.
msbuild yyjson_kmdf_example\example.sln /p:Configuration=Release /p:Platform=x64
```

### 3. Install and Run

```bash
# Run as Administrator
yyjson_kmdf_example\install_example.cmd
```

### 4. View Output

Monitor debug output using:
- **WinDbg**: Connect to kernel debug session
- **DebugView**: Enable kernel capture (Ctrl+K)

### 5. Unload Driver

```bash
yyjson_kmdf_example\uninstall_example.cmd
```

## Project Structure

```
yyjson_in_win_kernel/
├── yyjson_kmdf_lib/                # Reusable KMDF static library
│   ├── yyjson_kmdf_lib.vcxproj     # Builds yyjson_kmdf_lib.lib
│   ├── include/                    # Library headers and kernel shim headers
│   │   ├── yyjson.h                # yyjson public API
│   │   ├── yyjsonk_runtime.h       # Kernel runtime init/log/file I/O API
│   │   ├── stdio.h                 # File I/O shim
│   │   ├── math.h                  # Math shim
│   │   ├── locale.h                # Locale shim
│   │   └── assert.h                # Debug assertion shim
│   ├── src/                        # yyjson source file
│   ├── compat/src/                 # Kernel compatibility implementation
│   └── third_party/                # double-conversion helper library
│
├── yyjson_kmdf_example/            # Example driver that links the static library
│   ├── example.sln                 # Visual Studio solution
│   ├── example/                    # Driver project
│   │   └── driver/                 # Example driver code
│   ├── install_example.cmd         # Driver installation script
│   └── uninstall_example.cmd       # Driver removal script
│
├── yyjson_kmdf_tests/              # Test driver that links yyjson_kmdf_lib
│   ├── yyjson_kmdf_tests.sln       # Test driver solution with library project reference
│   ├── compat/                     # Helper code required by upstream tests only
│   └── driver/                     # Test driver entry point and test runner
│
├── LICENSE                         # MIT License
└── README.md                       # This file
```

## Examples Covered

The example driver (`example.sys`) demonstrates:

### 1. In-Memory JSON Parsing
```c
// Parse JSON from string
yyjson_doc *doc = yyjson_read(json_str, len, 0);
yyjson_val *root = yyjson_doc_get_root(doc);

// Read values
const char *name = yyjson_get_str(yyjson_obj_get(root, "name"));
int64_t version = yyjson_get_int(yyjson_obj_get(root, "version"));
bool enabled = yyjson_get_bool(yyjson_obj_get(root, "enabled"));
```

### 2. Array Traversal
```c
// Iterate over JSON array
yyjson_val *arr = yyjson_obj_get(root, "requests");
yyjson_val *item;
yyjson_arr_foreach(arr, idx, max, item) {
    const char *op = yyjson_get_str(yyjson_obj_get(item, "op"));
    int64_t size = yyjson_get_int(yyjson_obj_get(item, "size"));
    // Process each item...
}
```

### 3. Mutable Document Generation
```c
// Create mutable document
yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
yyjson_mut_val *root = yyjson_mut_obj(doc);

// Add values
yyjson_mut_obj_add_str(doc, root, "name", "example");
yyjson_mut_obj_add_int(doc, root, "version", 1);
yyjson_mut_doc_set_root(doc, root);

// Serialize to JSON string
const char *json = yyjson_mut_write(doc, 0, NULL);
```

### 4. File I/O Operations
```c
// Write JSON to file
yyjson_mut_write_file(path, doc, YYJSON_WRITE_PRETTY, NULL, NULL);

// Read JSON from file
yyjson_doc *doc = yyjson_read_file(path, 0, NULL, NULL);
```

**Note**: File operations require:
- `PASSIVE_LEVEL` IRQL
- Absolute DOS paths (e.g., `C:\Windows\Temp\file.json`) or NT paths

## Building from Source

### Option 1: Visual Studio IDE

1. Open `yyjson_kmdf_example\example.sln`
2. Select configuration: `Debug|x64` or `Release|x64`
3. Build Solution (Ctrl+Shift+B)
4. Output: `x64\Release\example.sys`

### Option 2: Command Line

```bash
# Set up build environment
"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

# Build
msbuild yyjson_kmdf_example\example.sln /p:Configuration=Release /p:Platform=x64 /t:Build
```

### Build Configurations

| Configuration | Description |
|---------------|-------------|
| `Debug|x64`   | Debug build with symbols, no optimizations |
| `Release|x64` | Release build with optimizations |

## API Reference

### Core yyjson Functions

| Function | Description |
|----------|-------------|
| `yyjson_read()` | Parse JSON from string |
| `yyjson_doc_get_root()` | Get root value from document |
| `yyjson_get_str()` | Get string value |
| `yyjson_get_int()` | Get integer value |
| `yyjson_get_bool()` | Get boolean value |
| `yyjson_obj_get()` | Get object member by key |
| `yyjson_arr_foreach()` | Iterate array elements |

### Mutable Document Functions

| Function | Description |
|----------|-------------|
| `yyjson_mut_doc_new()` | Create new mutable document |
| `yyjson_mut_obj()` | Create mutable object |
| `yyjson_mut_obj_add_str()` | Add string to object |
| `yyjson_mut_obj_add_int()` | Add integer to object |
| `yyjson_mut_write()` | Serialize to JSON string |

### File I/O Functions

| Function | Description |
|----------|-------------|
| `yyjson_read_file()` | Read JSON from file |
| `yyjson_mut_write_file()` | Write JSON to file |

### Kernel Compatibility Layer

| Header | Provided Functions |
|--------|-------------------|
| `stdio.h` | `fopen`, `fread`, `fwrite`, `fclose`, `fseek`, `ftell` |
| `math.h` | `pow`, `log10`, `ceil`, `floor`, `fabs` |
| `locale.h` | `setlocale`, `localeconv` |
| `assert.h` | `assert` (debug builds only) |

## Debugging

### Enable Debug Output

```c
// In your driver code
DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, 
           "JSON parsing result: %s\n", json_str);
```

### View Debug Output

**WinDbg**:
```
ed nt!Kd_DEFAULT_Mask 0xFFFFFFFF
g
```

**DebugView**:
1. Run as Administrator
2. Enable "Capture Kernel" (Ctrl+K)
3. Filter by process name

### Common Debug Scenarios

| Issue | Solution |
|-------|----------|
| No output | Check IRQL level, ensure `PASSIVE_LEVEL` |
| Access violation | Verify memory allocations, check buffer sizes |
| File not found | Use absolute paths, check file permissions |

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Commit your changes: `git commit -m 'Add amazing feature'`
4. Push to the branch: `git push origin feature/amazing-feature`
5. Open a Pull Request

### Development Guidelines

- Follow kernel coding standards (SAL annotations, proper error handling)
- Test on multiple Windows versions (10, 11)
- Update documentation for API changes
- Add unit tests for new features

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [yyjson](https://github.com/ibireme/yyjson) - High-performance JSON library by ibireme
- [double-conversion](https://github.com/google/double-conversion) - IEEE 754 double-precision conversion
- [Windows Driver Samples](https://github.com/microsoft/Windows-driver-samples) - KMDF reference implementations

## Support

- **Issues**: [GitHub Issues](https://github.com/your-username/yyjson_in_win_kernel/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-username/yyjson_in_win_kernel/discussions)
- **Email**: your.email@example.com

---

**Note**: This project is for educational and development purposes. Always test kernel drivers in isolated environments before production use.
