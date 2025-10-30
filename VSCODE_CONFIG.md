# VS Code 工作区配置详解

本文档详细说明 `.vscode` 文件夹中各配置文件的作用和移植方法。

## 📁 配置文件概览

| 文件 | 用途 | 是否需要手动配置 | 优先级 |
|------|------|----------------|--------|
| `launch.json` | 调试器配置 | ✅ 是 | 🔴 必需 |
| `tasks.json` | 编译/烧录任务 | ✅ 是 | 🔴 必需 |
| `settings.json` | 工作区设置 | ⚠️ 半自动 | 🟡 推荐 |
| `c_cpp_properties.json` | IntelliSense 配置 | 🔄 自动生成 | 🟢 自动 |
| `extensions.json` | 推荐扩展 | ⭐ 可选 | 🟢 可选 |

---

## 🐛 1. launch.json (调试配置)

### 作用

配置 VS Code 的调试器启动方式，定义如何连接到目标硬件、加载程序并开始调试。

### 本项目配置

提供了 **3 个调试配置**：

1. **Build & Debug Microcontroller - ST-Link** (推荐)
   - 使用 ST-Link 调试器
   - 支持 ST-Link V2/V2-1/V3
   - 使用 ST 官方 GDB Server

2. **Attach to Microcontroller - ST-Link**
   - 附加到运行中的程序
   - 不重新烧录固件
   - 适合调试已运行的代码

3. **OpenOCD Debugging Session**
   - 使用 OpenOCD + CMSIS-DAP
   - 开源方案，跨平台支持
   - 支持多种调试器

### 关键配置项

#### 基本配置

```jsonc
{
    "name": "调试配置名称",           // 显示在调试面板中
    "type": "cortex-debug",          // 调试器类型（ARM Cortex 系列）
    "request": "launch",             // launch=启动调试, attach=附加调试
    "cwd": "${workspaceFolder}",     // 工作目录
}
```

#### 目标设备配置

```jsonc
{
    "device": "STM32F407VETx",       // MCU 型号（必须修改）
    "interface": "swd",              // 调试接口: swd 或 jtag
    "servertype": "stlink",          // 调试服务器: stlink/openocd/jlink
    "executable": "${command:cmake.launchTargetPath}",  // .elf 文件路径
}
```

#### 调试器路径配置

**ST-Link 方式:**

```jsonc
{
    "serverpath": "${config:STM32VSCodeExtension.cubeCLT.path}/STLink-gdb-server/bin/ST-LINK_gdbserver",
    "gdbPath": "${config:STM32VSCodeExtension.cubeCLT.path}/GNU-tools-for-STM32/bin/arm-none-eabi-gdb",
    "svdFile": "${config:STM32VSCodeExtension.cubeCLT.path}/STMicroelectronics_CMSIS_SVD/STM32F407.svd"
}
```

**OpenOCD 方式:**

```jsonc
{
    "serverpath": "E:/OpenOCD-20250710-0.12.0/bin/openocd.exe",  // 需修改为实际路径
    "configFiles": [
        "interface/cmsis-dap.cfg",   // 调试器接口配置
        "target/stm32f4x.cfg"        // 目标芯片配置
    ],
    "searchDir": [
        "E:/OpenOCD-20250710-0.12.0/share/openocd/scripts"  // 脚本目录
    ],
    "gdbPath": "${config:STM32VSCodeExtension.cubeCLT.path}/GNU-tools-for-STM32/bin/arm-none-eabi-gdb"
}
```

### 移植步骤

#### 步骤 1: 修改目标设备

```jsonc
"device": "STM32H743VI",  // 改为你的 MCU 型号
```

#### 步骤 2: 修改 SVD 文件路径

```jsonc
"svdFile": "${config:STM32VSCodeExtension.cubeCLT.path}/STMicroelectronics_CMSIS_SVD/STM32H743.svd"
```

SVD 文件提供外设寄存器可视化，可从以下途径获取：

- STM32CubeCLT 安装目录
- [CMSIS-SVD 仓库](https://github.com/cmsis-svd/cmsis-svd)
- 芯片厂商官网

#### 步骤 3: 修改 OpenOCD 配置（如使用 OpenOCD）

```jsonc
"serverpath": "/usr/local/bin/openocd",  // Linux/macOS 路径
"configFiles": [
    "interface/stlink.cfg",     // 修改为你的调试器
    "target/stm32h7x.cfg"       // 修改为你的芯片系列
],
"searchDir": [
    "/usr/share/openocd/scripts"  // 修改为 OpenOCD 脚本目录
]
```

**常用调试器接口配置：**

| 调试器 | 配置文件 |
|--------|---------|
| CMSIS-DAP | `interface/cmsis-dap.cfg` |
| ST-Link V2 | `interface/stlink-v2.cfg` |
| ST-Link V2-1/V3 | `interface/stlink.cfg` |
| J-Link | `interface/jlink.cfg` |
| DAPLink | `interface/cmsis-dap.cfg` |

**常用芯片目标配置：**

| 芯片系列 | 配置文件 |
|---------|---------|
| STM32F1 | `target/stm32f1x.cfg` |
| STM32F4 | `target/stm32f4x.cfg` |
| STM32F7 | `target/stm32f7x.cfg` |
| STM32H7 | `target/stm32h7x.cfg` |
| STM32L4 | `target/stm32l4x.cfg` |
| STM32G0 | `target/stm32g0x.cfg` |

#### 步骤 4: 修改可执行文件路径（可选）

如果不使用 CMake，可以指定固定路径：

```jsonc
"executable": "${workspaceFolder}/build/my_project.elf"
```

### 调试前/后命令配置

#### preLaunchCommands (启动前命令)

```jsonc
"preLaunchCommands": [
    "set mem inaccessible-by-default off",  // 允许访问所有内存
    "enable breakpoint",                     // 启用断点
    "monitor reset halt"                     // 复位并暂停
]
```

#### postRestartCommands (重启后命令)

```jsonc
"postRestartCommands": [
    "monitor reset halt"  // 重启时复位并暂停
]
```

### 常见问题

#### 问题 1: 找不到调试器

**错误信息**: `Error: No ST-Link detected`

**解决方法**:

1. 检查调试器硬件连接
2. 安装调试器驱动程序
3. 运行诊断任务: `CubeProg: List all available communication interfaces`

#### 问题 2: SVD 文件加载失败

**错误信息**: `Failed to load SVD file`

**解决方法**:

1. 检查 SVD 文件路径是否正确
2. 确认 STM32 扩展已正确安装
3. 手动指定 SVD 文件绝对路径

#### 问题 3: OpenOCD 找不到配置文件

**错误信息**: `Can't find interface/cmsis-dap.cfg`

**解决方法**:

1. 确认 `searchDir` 路径正确
2. 检查 OpenOCD 是否正确安装
3. 使用绝对路径指定配置文件

---

## ⚙️ 2. tasks.json (任务配置)

### 2.1 作用

定义可重复执行的自动化任务，如编译、烧录、清理等操作。

### 2.2 本项目配置

提供了 **5 个任务**：

1. **CubeProg: Flash project (SWD)** - 使用 STM32CubeProgrammer 烧录
2. **Build + Flash** - 一键编译并烧录（组合任务）
3. **CMake: clean rebuild** - CMake 清理并重新编译
4. **CubeProg: List all available communication interfaces** - 列出所有调试器
5. **OpenOCD: Flash project (CMSIS-DAP)** - 使用 OpenOCD 烧录

### 2.3 任务类型

#### Shell 任务

执行 shell 命令：

```jsonc
{
    "type": "shell",
    "label": "任务名称",
    "command": "命令名称",
    "args": ["参数1", "参数2"],
    "options": {
        "cwd": "${workspaceFolder}"  // 工作目录
    }
}
```

#### CMake 任务

CMake 专用任务：

```jsonc
{
    "type": "cmake",
    "label": "CMake: clean rebuild",
    "command": "cleanRebuild",  // build/clean/rebuild/cleanRebuild
    "targets": ["all"],
    "preset": "${command:cmake.activeBuildPresetName}"
}
```

#### 组合任务

按顺序执行多个任务：

```jsonc
{
    "label": "Build + Flash",
    "dependsOrder": "sequence",  // sequence=顺序, parallel=并行
    "dependsOn": [
        "CMake: clean rebuild",
        "CubeProg: Flash project (SWD)"
    ]
}
```

### 2.4 关键配置项说明

#### STM32CubeProgrammer 烧录任务

```jsonc
{
    "type": "shell",
    "label": "CubeProg: Flash project (SWD)",
    "command": "STM32_Programmer_CLI",  // 需在 PATH 中
    "args": [
        "--connect",
        "port=swd",                     // swd 或 jtag
        "--download",
        "${command:cmake.launchTargetPath}",  // 自动获取 .elf 文件
        "-hardRst",                     // 硬件复位
        "-rst",                         // 软件复位
        "--start"                       // 烧录后启动
    ]
}
```

#### OpenOCD 烧录任务

```jsonc
{
    "type": "shell",
    "label": "OpenOCD: Flash project (CMSIS-DAP)",
    "command": "openocd",
    "args": [
        "-f", "interface/cmsis-dap.cfg",
        "-f", "target/stm32f4x.cfg",
        "-c", "program ${workspaceFolder}/build/Debug/project.elf verify reset exit"
    ]
}
```

### 2.5 移植步骤

#### 步骤 1: 修改烧录接口

```jsonc
"args": [
    "--connect",
    "port=jtag",  // 改为 jtag 接口
    // ...
]
```

#### 步骤 2: 修改可执行文件路径

```jsonc
"--download",
"${workspaceFolder}/build/Release/my_project.elf",  // 固定路径
```

或使用 .bin 文件（需指定起始地址）：

```jsonc
"--download",
"${workspaceFolder}/build/Release/my_project.bin",
"0x08000000",  // Flash 起始地址
```

#### 步骤 3: 修改 OpenOCD 配置文件

```jsonc
"args": [
    "-f", "interface/jlink.cfg",      // 修改调试器
    "-f", "target/stm32h7x.cfg",      // 修改芯片系列
    "-c", "program ${workspaceFolder}/build/Debug/project.bin 0x08000000 verify reset exit"
]
```

#### 步骤 4: 调整组合任务

```jsonc
{
    "label": "Build + Flash",
    "dependsOrder": "sequence",
    "dependsOn": [
        "CMake: build",              // 改为增量编译
        "OpenOCD: Flash project"     // 改用 OpenOCD 烧录
    ]
}
```

### 2.6 执行任务的方法

1. **命令面板**: `Ctrl+Shift+P` → `Tasks: Run Task` → 选择任务
2. **快捷键**: `Ctrl+Shift+B` (执行默认构建任务)
3. **终端菜单**: `Terminal` → `Run Task...`
4. **自定义快捷键**: 在 `keybindings.json` 中绑定

### 2.7 常见任务问题

#### 问题 1: 找不到 STM32_Programmer_CLI

**错误信息**: `'STM32_Programmer_CLI' is not recognized`

**解决方法**:

1. 安装 STM32CubeProgrammer
2. 将安装目录的 `bin` 文件夹添加到系统 PATH
   - Windows: `C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin`
   - Linux: `/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin`

#### 问题 2: 找不到 openocd

**错误信息**: `'openocd' is not recognized`

**解决方法**:

1. 安装 OpenOCD
2. 将 OpenOCD 的 `bin` 目录添加到 PATH
3. 或在任务中使用绝对路径：

```jsonc
"command": "E:/OpenOCD/bin/openocd.exe"
```

#### 问题 3: CMake 找不到可执行文件

**错误信息**: `Could not determine executable path`

**解决方法**:

1. 确保项目已经编译过
2. 运行 `cmake --build build/Debug`
3. 或使用固定路径代替 `${command:cmake.launchTargetPath}`

---

## 🎨 3. settings.json (工作区设置)

### 3.1 作用说明

定义项目级别的 VS Code 设置，覆盖全局设置。

### 3.2 本项目配置示例

```jsonc
{
    // CMake 工具配置
    "cmake.cmakePath": "cube-cmake",
    "cmake.configureArgs": [
        "-DCMAKE_COMMAND=cube-cmake"
    ],
    "cmake.preferredGenerators": [
        "Ninja"  // 使用 Ninja 构建系统
    ],

    // Clangd 配置（用于 IntelliSense）
    "stm32cube-ide-clangd.path": "cube",
    "stm32cube-ide-clangd.arguments": [
        "starm-clangd",
        "--query-driver=${env:CUBE_BUNDLE_PATH}/gnu-tools-for-stm32/13.3.1+st.9/bin/arm-none-eabi-gcc.exe",
        "--query-driver=${env:CUBE_BUNDLE_PATH}/gnu-tools-for-stm32/13.3.1+st.9/bin/arm-none-eabi-g++.exe"
    ]
}
```

### 3.3 常用配置项

#### 文件关联

```jsonc
{
    "files.associations": {
        "*.h": "c",
        "*.c": "c",
        "*.ld": "linkerscript"
    }
}
```

#### 排除文件和文件夹

```jsonc
{
    "files.exclude": {
        "**/build": true,
        "**/.git": true,
        "**/*.o": true
    },
    "search.exclude": {
        "**/build": true,
        "**/Drivers": true
    }
}
```

#### C/C++ 配置提供者

```jsonc
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

### 3.4 移植建议

通常不需要手动配置，扩展会自动生成。如需自定义：

1. **修改文件关联**: 根据项目文件类型调整
2. **排除不需要的文件夹**: 加快搜索和索引速度
3. **调整编辑器行为**: 缩进、换行等

---

## 📝 4. c_cpp_properties.json (IntelliSense 配置)

### 4.1 作用介绍

配置 C/C++ 扩展的 IntelliSense（代码补全、跳转、错误检测）。

### 4.2 本项目配置示例

```jsonc
{
    "configurations": [
        {
            "name": "STM32",
            "configurationProvider": "ms-vscode.cmake-tools",
            "intelliSenseMode": "${default}",
            "compileCommands": "${workspaceFolder}/build/Debug/compile_commands.json"
        }
    ],
    "version": 4
}
```

### 4.3 关键配置项说明

#### 使用 CMake 自动配置（推荐）

```jsonc
{
    "configurationProvider": "ms-vscode.cmake-tools",
    "compileCommands": "${workspaceFolder}/build/Debug/compile_commands.json"
}
```

#### 手动配置（不推荐）

```jsonc
{
    "name": "ARM",
    "includePath": [
        "${workspaceFolder}/**",
        "C:/gcc-arm/include"
    ],
    "defines": [
        "STM32F407xx",
        "USE_HAL_DRIVER"
    ],
    "compilerPath": "C:/gcc-arm/bin/arm-none-eabi-gcc.exe",
    "cStandard": "c11",
    "cppStandard": "c++17",
    "intelliSenseMode": "gcc-arm"
}
```

### 4.4 移植指南

1. **使用 CMake**: 无需手动配置，自动生成
2. **手动配置**: 修改头文件路径、宏定义、编译器路径

### 4.5 IntelliSense 常见问题

#### 问题 1: 头文件找不到

**错误信息**: `#include errors detected`

**解决方法**:

1. 确保 CMake 已配置（`cmake --preset Debug`）
2. 检查 `compile_commands.json` 是否存在
3. 重新加载 IntelliSense: `Ctrl+Shift+P` → `C/C++: Reset IntelliSense Database`

#### 问题 2: 宏定义错误

**解决方法**:
手动添加宏定义到 `c_cpp_properties.json`:

```jsonc
"defines": [
    "STM32F407xx",
    "USE_HAL_DRIVER",
    "USE_FULL_ASSERT"
]
```

---

## 🧩 5. extensions.json (推荐扩展)

### 5.1 扩展作用

推荐团队成员安装的 VS Code 扩展，确保开发环境一致。

### 5.2 本项目推荐扩展列表

#### 核心扩展（必需）

| 扩展 ID | 名称 | 用途 |
|---------|------|------|
| `ms-vscode.cpptools` | C/C++ | C/C++ 语言支持 |
| `ms-vscode.cmake-tools` | CMake Tools | CMake 项目管理 |
| `marus25.cortex-debug` | Cortex-Debug | ARM 调试支持 |

#### 语法高亮扩展

| 扩展 ID | 名称 | 用途 |
|---------|------|------|
| `dan-c-underwood.arm` | ARM Assembly | ARM 汇编语法高亮 |
| `zixuanwang.linkerscript` | Linker Script | 链接脚本语法高亮 |
| `twxs.cmake` | CMake | CMake 语法高亮 |
| `jeff-hykin.better-cpp-syntax` | Better C++ Syntax | 增强 C++ 语法 |

#### 调试辅助扩展

| 扩展 ID | 名称 | 用途 |
|---------|------|------|
| `mcu-debug.memory-view` | Memory View | 内存查看器 |
| `mcu-debug.peripheral-viewer` | Peripheral Viewer | 外设寄存器查看 |
| `mcu-debug.rtos-views` | RTOS Views | FreeRTOS 任务查看 |

#### 其他实用扩展

| 扩展 ID | 名称 | 用途 |
|---------|------|------|
| `ms-vscode.hexeditor` | Hex Editor | 十六进制编辑器 |
| `trond-snekvik.gnu-mapfiles` | GNU Map Files | .map 文件查看 |

### 配置示例

```jsonc
{
    "recommendations": [
        // 核心扩展
        "ms-vscode.cpptools",
        "ms-vscode.cmake-tools",
        "marus25.cortex-debug",

        // 语法高亮
        "dan-c-underwood.arm",
        "zixuanwang.linkerscript",

        // 调试辅助
        "mcu-debug.memory-view",
        "mcu-debug.peripheral-viewer"
    ]
}
```

### 安装推荐扩展

当打开项目时，VS Code 会自动提示安装推荐扩展：

1. 点击右下角弹出的通知
2. 或手动打开: `Ctrl+Shift+P` → `Extensions: Show Recommended Extensions`
3. 点击 "Install All"

---

## 🚀 移植检查清单

### 必需修改（🔴 高优先级）

- [ ] `launch.json` - 修改 `device` 为目标 MCU 型号
- [ ] `launch.json` - 修改 `svdFile` 路径
- [ ] `launch.json` - 修改调试器配置文件（OpenOCD）
- [ ] `tasks.json` - 修改可执行文件路径（如不用 CMake）
- [ ] `tasks.json` - 修改 OpenOCD 配置文件

### 推荐检查（🟡 中优先级）

- [ ] `tasks.json` - 确认烧录接口（SWD/JTAG）
- [ ] `launch.json` - 确认调试器路径
- [ ] `settings.json` - 排除不需要的文件夹
- [ ] `extensions.json` - 添加项目特定扩展

### 可选优化（🟢 低优先级）

- [ ] 创建自定义快捷键
- [ ] 配置 `preLaunchTask` 自动编译
- [ ] 添加更多自动化任务
- [ ] 配置代码格式化规则

---

## 🛠️ 常用调试技巧

### 1. 一键编译并调试

在 `launch.json` 中添加 `preLaunchTask`:

```jsonc
{
    "name": "Build & Debug",
    "preLaunchTask": "Build + Flash",  // 调试前自动编译烧录
    // ... 其他配置
}
```

### 2. 查看外设寄存器

调试时打开 "Cortex Peripherals" 面板：

1. 启动调试（F5）
2. 侧边栏选择 "Cortex Peripherals"
3. 展开外设查看寄存器值

### 3. 查看 FreeRTOS 任务

安装 `mcu-debug.rtos-views` 扩展后：

1. 启动调试
2. 侧边栏选择 "RTOS" 面板
3. 查看任务状态、栈使用情况

### 4. 自定义快捷键

在 `keybindings.json` 中添加：

```jsonc
[
    {
        "key": "ctrl+alt+f",
        "command": "workbench.action.tasks.runTask",
        "args": "Build + Flash"
    },
    {
        "key": "F6",
        "command": "workbench.action.tasks.runTask",
        "args": "CubeProg: Flash project (SWD)"
    }
]
```

---

## 📚 参考资源

### 官方文档

- [VS Code 任务配置](https://code.visualstudio.com/docs/editor/tasks)
- [VS Code 调试配置](https://code.visualstudio.com/docs/editor/debugging)
- [Cortex-Debug 扩展](https://github.com/Marus/cortex-debug)
- [CMake Tools 扩展](https://github.com/microsoft/vscode-cmake-tools)

### STM32 相关

- [STM32 VS Code Extension](https://marketplace.visualstudio.com/items?itemName=stmicroelectronics.stm32-vscode-extension)
- [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)
- [OpenOCD](https://openocd.org/)

### 工具链

- [ARM GCC Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
- [CMSIS-SVD 文件](https://github.com/cmsis-svd/cmsis-svd)

---

## ❓ 常见问题 FAQ

### Q1: 为什么 IntelliSense 不工作？

**A**: 确保以下步骤：

1. 运行 `cmake --preset Debug` 配置项目
2. 检查 `compile_commands.json` 是否生成
3. 重启 VS Code
4. 重新加载 IntelliSense: `Ctrl+Shift+P` → `C/C++: Reset IntelliSense Database`

### Q2: 如何切换调试配置？

**A**: 在调试面板（左侧边栏）选择不同的调试配置：

1. 点击 "Run and Debug" 图标
2. 顶部下拉菜单选择调试配置
3. 按 F5 启动调试

### Q3: 如何添加新的编译任务？

**A**: 在 `tasks.json` 中添加新任务：

```jsonc
{
    "type": "shell",
    "label": "My Custom Task",
    "command": "your-command",
    "args": ["arg1", "arg2"]
}
```

### Q4: 为什么状态栏没有 "生成" 按钮？

**A**: 确保安装了 **CMake Tools** 扩展：

1. `Ctrl+Shift+X` 打开扩展面板
2. 搜索 "CMake Tools"
3. 点击 "Install"

### Q5: 如何在 Linux/macOS 上配置？

**A**: 主要修改路径分隔符和可执行文件扩展名：

- Windows: `E:\\OpenOCD\\bin\\openocd.exe`
- Linux/macOS: `/usr/local/bin/openocd`

---

---

## 📦 完整配置文件（可直接复制）

由于本项目的 `.gitignore` 忽略了 `.vscode` 文件夹，这里提供完整的配置文件供复制使用。

### launch.json (完整配置)

创建 `.vscode/launch.json` 文件，复制以下内容：

```jsonc
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            // ========== 基本配置 ==========
            "name": "OpenOCD Debugging Session",  // 调试配置名称,会显示在VS Code调试面板中
            "cwd": "${workspaceFolder}",          // 工作目录,通常保持为工作区根目录
            "type": "cortex-debug",               // 调试器类型,用于ARM Cortex系列MCU
            
            // ========== 可执行文件配置 ==========
            "executable": "${command:cmake.launchTargetPath}",  // 要调试的.elf文件路径
            // 说明: 使用CMake自动获取编译输出的可执行文件
            // 移植时可改为: "${workspaceFolder}/build/Debug/your_project.elf"
            
            "request": "launch",  // 调试模式: "launch"(启动并调试) 或 "attach"(附加到运行中的程序)
            
            // ========== 调试服务器配置 ==========
            "servertype": "openocd",  // GDB服务器类型,可选: openocd, stlink, jlink, pyocd等
            
            // ========== 目标设备配置 ==========
            "device": "STM32F407VETx",  // 目标MCU型号
            // 移植时需修改为你的芯片型号,如: STM32F103C8, STM32H743VI等
            
            "interface": "swd",  // 调试接口,可选: swd(推荐) 或 jtag
            
            "runToEntryPoint": "main",  // 启动后自动运行到此函数并暂停,通常设为"main"
            
            // ========== SVD文件配置 ==========
            "svdFile": "${config:STM32VSCodeExtension.cubeCLT.path}/STMicroelectronics_CMSIS_SVD/STM32F407.svd",
            // SVD文件提供外设寄存器的可视化查看功能
            // 移植时需修改为对应芯片的SVD文件路径
            // 下载地址: https://github.com/cmsis-svd/cmsis-svd
            
            // ========== OpenOCD路径配置 ==========
            "serverpath": "E:/OpenOCD-20250710-0.12.0/bin/openocd.exe",
            // OpenOCD可执行文件的完整路径
            // 移植时需修改为你系统中OpenOCD的实际安装路径
            // Linux/Mac示例: "/usr/local/bin/openocd"
            
            // ========== OpenOCD配置文件 ==========
            "configFiles": [
                "interface/cmsis-dap.cfg",  // 调试器接口配置文件
                // 常用接口配置:
                //   - interface/cmsis-dap.cfg    (DAPLink调试器)
                //   - interface/stlink-v2.cfg    (ST-Link V2)
                //   - interface/stlink.cfg       (ST-Link V2-1/V3)
                //   - interface/jlink.cfg        (J-Link)
                
                "target/stm32f4x.cfg"        // 目标芯片配置文件
                // 移植时需修改为对应的芯片系列配置:
                //   - target/stm32f1x.cfg  (STM32F1系列)
                //   - target/stm32f4x.cfg  (STM32F4系列)
                //   - target/stm32h7x.cfg  (STM32H7系列)
                //   完整列表查看OpenOCD的scripts/target目录
            ],
            
            // ========== OpenOCD脚本搜索目录 ==========
            "searchDir": [
                "E:/OpenOCD-20250710-0.12.0/share/openocd/scripts"
            ],
            // OpenOCD配置脚本的搜索路径
            // 移植时需修改为你的OpenOCD安装目录下的scripts文件夹
            // Linux示例: "/usr/share/openocd/scripts"
            
            // ========== GDB路径配置 ==========
            "gdbPath": "${config:STM32VSCodeExtension.cubeCLT.path}/GNU-tools-for-STM32/bin/arm-none-eabi-gdb",
            // ARM GDB调试器的路径
            // 移植时可能需要修改,如使用自己安装的工具链:
            //   Windows: "C:/gcc-arm-none-eabi/bin/arm-none-eabi-gdb.exe"
            //   Linux: "/usr/bin/arm-none-eabi-gdb"
            
            // ========== 调试启动前命令 ==========
            "preLaunchCommands": [
                "set mem inaccessible-by-default off",  // 允许访问所有内存区域,避免某些区域无法读取
                "enable breakpoint",                     // 启用断点功能
                "monitor reset halt"                     // 复位MCU并立即停止,确保从头开始调试
            ],
            // 这些GDB命令在程序下载到芯片后、开始调试前执行
            
            // ========== 调试重启后命令 ==========
            "postRestartCommands": [
                "monitor reset halt"  // 重启调试会话时,复位MCU并停止
            ]
            // 当在调试过程中点击"重启"按钮时执行这些命令
            
            // ========== 移植检查清单 ==========
            // 1. 修改 device 为目标MCU型号
            // 2. 修改 svdFile 为对应芯片的SVD文件
            // 3. 修改 serverpath 为OpenOCD安装路径
            // 4. 修改 configFiles 中的接口和目标配置文件
            // 5. 修改 searchDir 为OpenOCD脚本目录
            // 6. 修改 gdbPath 为GDB工具链路径
            // 7. 根据需要修改 executable 路径
            // 8. 确认调试器硬件连接(SWD/JTAG接口)
        }
    ]
}
```

### tasks.json (完整配置)

创建 `.vscode/tasks.json` 文件，复制以下内容：

```jsonc
{
    // ========== Tasks配置文件版本 ==========
    "version": "2.0.0",
    
    // ========== Windows平台特定配置 ==========
    "windows": {
        "options": {
            "shell": {
                "executable": "cmd.exe",  // 使用CMD作为shell执行器
                "args": ["/d", "/c"]      // /d: 禁用AutoRun命令, /c: 执行命令后关闭
                // 移植说明: Linux/Mac系统会自动使用bash/sh,无需此配置
            }
        }
    },
    
    "tasks": [
        // ========== 任务1: CMake清理并重新编译 ==========
        {
            "type": "cmake",  // 任务类型: CMake专用任务
            "label": "CMake: clean rebuild",  // 任务名称
            
            "command": "cleanRebuild",  // CMake命令: 清理旧文件并重新编译
            // 其他可选命令:
            //   - "build": 仅编译(增量编译)
            //   - "clean": 仅清理
            //   - "rebuild": 重新编译(不清理)
            
            "targets": [
                "all"  // 编译目标: all表示编译所有目标
                // 可改为具体目标名称,如: ["my_project.elf"]
            ],
            
            "preset": "${command:cmake.activeBuildPresetName}",
            // 使用当前激活的CMake预设配置
            // 预设配置定义在CMakePresets.json中
            // 移植时: 确保CMakePresets.json中有对应的预设配置
            
            "group": "build",  // 任务分组: 属于构建类任务
            // 可以通过 Ctrl+Shift+B 快捷键调用构建组任务
            
            "problemMatcher": [],  // 问题匹配器
            
            "detail": "CMake template clean rebuild task"  // 任务详细描述
        },
        
        // ========== 任务2: 列出所有可用的通信接口 (诊断工具) ==========
        {
            "type": "shell",
            "label": "CubeProg: List all available communication interfaces",
            
            "command": "STM32_Programmer_CLI",  // STM32烧录工具命令行
            
            "args": ["--list"],  // 列出所有连接的ST-Link/调试器
            // 使用场景: 
            //   - 检查调试器是否正确连接
            //   - 查看调试器的序列号(多调试器环境)
            //   - 诊断连接问题
            
            "options": {
                "cwd": "${workspaceFolder}"  // 工作目录
            },
            
            "problemMatcher": []
        },
        
        // ========== 任务3: 使用OpenOCD通过CMSIS-DAP烧录程序 ==========
        {
            "type": "shell",
            "label": "OpenOCD: Flash project (CMSIS-DAP)",
            
            // ========== OpenOCD命令 ==========
            "command": "openocd",
            // 说明: 开源的片上调试工具,支持多种调试器和芯片
            // 移植前提: 需要安装OpenOCD并添加到系统PATH
            // 下载地址: https://openocd.org/ 或 https://github.com/xpack-dev-tools/openocd-xpack
            
            // ========== OpenOCD参数详解 ==========
            "args": [
                "-f",  // -f: 加载配置文件
                "interface/cmsis-dap.cfg",  // 调试器接口配置
                // 常用调试器接口配置文件:
                //   - interface/cmsis-dap.cfg    (DAPLink/CMSIS-DAP调试器)
                //   - interface/stlink-v2.cfg    (ST-Link V2)
                //   - interface/stlink.cfg       (ST-Link V2-1/V3)
                //   - interface/jlink.cfg        (SEGGER J-Link)
                //   - interface/ftdi/...         (FTDI系列调试器)
                // 移植时: 根据实际使用的调试器修改
                
                "-f",  // 加载第二个配置文件
                "target/stm32f4x.cfg",  // 目标芯片配置
                // 常用芯片配置文件:
                //   - target/stm32f1x.cfg  (STM32F1系列)
                //   - target/stm32f4x.cfg  (STM32F4系列)
                //   - target/stm32f7x.cfg  (STM32F7系列)
                //   - target/stm32h7x.cfg  (STM32H7系列)
                //   - target/stm32l4x.cfg  (STM32L4系列)
                // 完整列表查看: OpenOCD安装目录/scripts/target/
                // 移植时: 根据目标芯片系列修改
                
                "-c",  // -c: 执行OpenOCD命令
                "program ${command:st-stm32-ide-debug-launch.get-projects-binary-from-context1} verify reset exit"
                // 命令说明:
                //   program <file>: 烧录指定文件到芯片
                //   verify:         烧录后验证数据完整性
                //   reset:          烧录完成后复位芯片
                //   exit:           烧录完成后退出OpenOCD
                // 移植时: 可将变量改为固定路径
                //   如: "program ${workspaceFolder}/build/Debug/my_project.elf verify reset exit"
                //   或: "program ${workspaceFolder}/build/Debug/my_project.bin 0x08000000 verify reset exit"
                //       (对于.bin文件需要指定烧录地址,通常STM32为0x08000000)
            ],
            
            "options": {
                "cwd": "${workspaceFolder}"  // 命令执行目录
            },
            
            "problemMatcher": []
            
            // ========== OpenOCD vs STM32CubeProgrammer 对比 ==========
            // OpenOCD:
            //   优点: 开源免费,跨平台,支持多种调试器和芯片
            //   缺点: 配置相对复杂,部分新芯片支持较慢
            // STM32CubeProgrammer:
            //   优点: ST官方工具,支持最新芯片,功能全面
            //   缺点: 仅限STM32芯片,Windows下体验更好
            
            // ========== 移植检查清单 ==========
            // 1. 确认OpenOCD已安装并在PATH中
            // 2. 修改interface配置文件匹配你的调试器
            // 3. 修改target配置文件匹配你的芯片系列
            // 4. 修改program命令中的文件路径
            // 5. 对于.bin文件,需要添加烧录起始地址
            // 6. 可添加OpenOCD配置文件搜索路径: -s <path>
        }
    ]
}
```

### settings.json (可选配置)

创建 `.vscode/settings.json` 文件，复制以下内容：

```jsonc
{
    // CMake 工具配置
    "cmake.cmakePath": "cube-cmake",
    "cmake.configureArgs": [
        "-DCMAKE_COMMAND=cube-cmake"
    ],
    "cmake.preferredGenerators": [
        "Ninja"  // 使用 Ninja 构建系统
    ],

    // Clangd 配置（用于 IntelliSense）
    "stm32cube-ide-clangd.path": "cube",
    "stm32cube-ide-clangd.arguments": [
        "starm-clangd",
        "--query-driver=${env:CUBE_BUNDLE_PATH}/gnu-tools-for-stm32/13.3.1+st.9/bin/arm-none-eabi-gcc.exe",
        "--query-driver=${env:CUBE_BUNDLE_PATH}/gnu-tools-for-stm32/13.3.1+st.9/bin/arm-none-eabi-g++.exe"
    ],

    // 文件关联（可选）
    "files.associations": {
        "*.h": "c",
        "*.c": "c"
    },

    // 排除不需要的文件（可选）
    "files.exclude": {
        "**/build": true,
        "**/.git": true
    }
}
```

### c_cpp_properties.json (自动生成，可选)

创建 `.vscode/c_cpp_properties.json` 文件，复制以下内容：

```json
{
    "configurations": [
        {
            "name": "STM32",
            "configurationProvider": "ms-vscode.cmake-tools",
            "intelliSenseMode": "${default}",
            "compileCommands": "${workspaceFolder}/build/Debug/compile_commands.json"
        }
    ],
    "version": 4
}
```

### extensions.json (推荐扩展列表)

创建 `.vscode/extensions.json` 文件，复制以下内容：

```jsonc
{
    "recommendations": [
        // 核心扩展
        "ms-vscode.cpptools",                   // C/C++ 语言支持
        "ms-vscode.cpptools-themes",            // C/C++ 主题
        "ms-vscode.cmake-tools",                // CMake 工具
        "twxs.cmake",                           // CMake 语法高亮
        "ms-vscode.cpptools-extension-pack",    // C/C++ 扩展包
        
        // ARM/嵌入式开发
        "dan-c-underwood.arm",                  // ARM 汇编语法高亮
        "zixuanwang.linkerscript",              // 链接脚本语法高亮
        
        // 调试工具
        "marus25.cortex-debug",                 // Cortex 调试支持
        "mcu-debug.debug-tracker-vscode",       // 调试跟踪器
        "mcu-debug.memory-view",                // 内存查看器
        "mcu-debug.peripheral-viewer",          // 外设寄存器查看器
        "mcu-debug.rtos-views",                 // RTOS 任务查看器
        
        // 实用工具
        "ms-vscode.hexeditor",                  // 十六进制编辑器
        "trond-snekvik.gnu-mapfiles",           // MAP 文件查看器
        "jeff-hykin.better-cpp-syntax"          // 增强 C++ 语法
    ]
}
```

---

## 🚀 快速开始指南

### 1. 创建 .vscode 文件夹

在项目根目录创建 `.vscode` 文件夹：

```bash
mkdir .vscode
```

### 2. 复制配置文件

按照上面的内容，依次创建以下文件：

- **必需**: `launch.json` (调试配置)
- **必需**: `tasks.json` (任务配置)
- **推荐**: `extensions.json` (扩展推荐)
- **可选**: `settings.json` (工作区设置)
- **可选**: `c_cpp_properties.json` (IntelliSense 配置)

### 3. 修改路径

根据你的系统环境，修改以下路径：

**launch.json**:

- `device`: 改为你的 MCU 型号
- `serverpath`: OpenOCD 安装路径
- `searchDir`: OpenOCD 脚本目录
- `configFiles`: 调试器和芯片配置文件
- `svdFile`: SVD 文件路径

**tasks.json**:

- `configFiles`: 调试器和芯片配置文件
- OpenOCD 路径（如果不在 PATH 中）

### 4. 安装推荐扩展

打开项目后，VS Code 会提示安装推荐扩展，点击 "Install All" 即可。

### 5. 开始使用

- **编译**: `Ctrl+Shift+B`
- **调试**: `F5`
- **运行任务**: `Ctrl+Shift+P` → `Tasks: Run Task`

---

**文档版本**: v1.1
**最后更新**: 2025-10-30
