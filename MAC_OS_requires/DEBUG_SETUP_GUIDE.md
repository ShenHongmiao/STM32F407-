# STM32 调试环境配置指南

> 📌 **快速参考**：本文档说明如何在新的 STM32 工程中快速配置 Cortex-Debug 调试环境

---

## 📋 目录

- [前置条件](#前置条件)
- [快速开始](#快速开始)
- [方法一：自动化脚本（推荐）](#方法一自动化脚本推荐)
- [方法二：手动配置](#方法二手动配置)
- [方法三：复制模板](#方法三复制模板)
- [常见问题](#常见问题)
- [芯片型号参考](#芯片型号参考)

---

## 前置条件

### ✅ 已安装的工具

1. **VS Code 扩展**
   - `Cortex-Debug` (marus25.cortex-debug) ✅
   - `C/C++` (ms-vscode.cpptools)
   - `CMake Tools` (可选)

2. **STM32 工具链**
   - 安装路径：`/opt/ST/STM32CubeCLT_1.19.0/`
   - 包含：GDB、ST-LINK Server、ARM GCC 工具链

3. **硬件**
   - ST-LINK 调试器
   - STM32 开发板

---

## 快速开始

### 🚀 最快方法（1 分钟配置）

在新工程根目录执行：

```bash
# 使用自动化脚本
bash .vscode/init-debug.sh <项目名> [芯片型号] [构建目录]

# 示例：
bash .vscode/init-debug.sh MyProject STM32F407VG build/Debug
```

然后按 `F5` 开始调试！

---

## 方法一：自动化脚本（推荐）

### 📝 使用步骤

1. **复制脚本到新工程**

   ```bash
   # 从模板工程复制脚本
   cp /path/to/STM32F407-/.vscode/init-debug.sh ./
   chmod +x init-debug.sh
   ```

2. **运行脚本**

   ```bash
   ./init-debug.sh MyProject STM32F407VG
   ```

   脚本会自动：
   - ✅ 创建 `.vscode` 目录
   - ✅ 查找 `.elf` 文件
   - ✅ 生成 `launch.json`
   - ✅ 生成 `tasks.json`
   - ✅ 匹配对应的 SVD 文件

3. **验证配置**
   - 连接 ST-LINK 调试器
   - 按 `F5` 启动调试
   - 程序应该在 `main` 函数停止

### 🔧 脚本参数说明

```bash
./init-debug.sh [项目名] [芯片型号] [构建目录]
```

| 参数 | 说明 | 默认值 | 示例 |
|------|------|--------|------|
| 项目名 | `.elf` 文件名称 | MyProject | I2C, LED_Blink |
| 芯片型号 | MCU 完整型号 | STM32F407VG | STM32F103C8 |
| 构建目录 | 相对于项目根的构建路径 | build/Debug | Debug, build |

### 💡 示例

```bash
# STM32F407 项目
./init-debug.sh I2C_Demo STM32F407VG build/Debug

# STM32F103 项目
./init-debug.sh LED_Blink STM32F103C8 Debug

# 自定义构建目录
./init-debug.sh MyApp STM32H743ZI cmake-build-debug
```

---

## 方法二：手动配置

### 📂 创建 `.vscode` 目录

```bash
mkdir -p .vscode
```

### 📄 创建 `launch.json`

在 `.vscode/launch.json` 中添加：

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Cortex Debug",
      "type": "cortex-debug",
      "request": "launch",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/build/Debug/YOUR_PROJECT.elf",
      "servertype": "stlink",
      "device": "STM32F407VG",
      "interface": "swd",
      "runToEntryPoint": "main",
      "svdFile": "/opt/ST/STM32CubeCLT_1.19.0/STMicroelectronics_CMSIS_SVD/STM32F407.svd",
      "serverpath": "/opt/ST/STM32CubeCLT_1.19.0/STLink-gdb-server/bin/ST-LINK_gdbserver",
      "armToolchainPath": "/opt/ST/STM32CubeCLT_1.19.0/GNU-tools-for-STM32/bin",
      "stlinkPath": "/opt/ST/STM32CubeCLT_1.19.0/STLink-gdb-server/bin",
      "preLaunchTask": "Build"
    }
  ]
}
```

### 🔑 关键参数修改

修改以下 3 个参数：

1. **executable** - 指向您的 `.elf` 文件

   ```json
   "executable": "${workspaceFolder}/build/Debug/YOUR_PROJECT.elf"
   ```

2. **device** - 您的芯片型号

   ```json
   "device": "STM32F407VG"
   ```

3. **svdFile** - 对应的 SVD 文件（根据芯片系列）

   ```json
   "svdFile": "/opt/ST/STM32CubeCLT_1.19.0/STMicroelectronics_CMSIS_SVD/STM32F407.svd"
   ```

### 📄 创建 `tasks.json`

在 `.vscode/tasks.json` 中添加：

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build",
      "type": "shell",
      "command": "cmake",
      "args": [
        "--build",
        "${workspaceFolder}/build/Debug",
        "--target",
        "all",
        "-j",
        "10"
      ],
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "problemMatcher": "$gcc"
    }
  ]
}
```

---

## 方法三：复制模板

### 📋 从现有项目复制配置

如果您有一个已配置好的项目（如本项目），可以直接复制：

```bash
# 复制整个 .vscode 目录到新工程
cp -r /path/to/STM32F407-/.vscode /path/to/new-project/

# 只复制必要文件
cp /path/to/STM32F407-/.vscode/launch.json /path/to/new-project/.vscode/
cp /path/to/STM32F407-/.vscode/tasks.json /path/to/new-project/.vscode/
```

然后只需修改：

- `launch.json` 中的 `executable` 和 `device`
- `tasks.json` 中的构建路径（如果不同）

### 📦 使用模板文件

项目中包含 `launch.json.template`，可以作为参考：

```bash
cp .vscode/launch.json.template .vscode/launch.json
# 然后编辑 launch.json，替换以下内容：
# - YOUR_PROJECT_NAME → 实际项目名
# - STM32F407VG → 实际芯片型号（如果不同）
```

---

## 常见问题

### ❓ 如何查找 `.elf` 文件位置？

```bash
find . -name "*.elf" -type f
```

常见位置：

- CMake: `build/Debug/PROJECT.elf` 或 `build/PROJECT.elf`
- Makefile: `Debug/PROJECT.elf` 或 `build/PROJECT.elf`
- STM32CubeIDE: `Debug/PROJECT.elf`

### ❓ 如何确定芯片型号？

查看以下文件：

- `.ioc` 文件（STM32CubeMX 项目）
- `CMakeLists.txt` 中的 MCU 定义
- 芯片丝印（开发板上）

常见格式：

- `STM32F407VGTx` → 使用 `STM32F407VG`
- `STM32F103C8T6` → 使用 `STM32F103C8`

### ❓ 如何查找对应的 SVD 文件？

```bash
# 列出所有可用的 SVD 文件
ls /opt/ST/STM32CubeCLT_1.19.0/STMicroelectronics_CMSIS_SVD/

# 搜索特定型号
ls /opt/ST/STM32CubeCLT_1.19.0/STMicroelectronics_CMSIS_SVD/ | grep -i f407
```

规则：使用芯片系列名称，如：

- `STM32F407VG` → `STM32F407.svd`
- `STM32F103C8` → `STM32F103.svd`
- `STM32H743ZI` → `STM32H743.svd`

### ❓ 调试时提示 "Unable to start debugging"？

**解决方案：**

1. **检查 ST-LINK 连接**

   ```bash
   /opt/ST/STM32CubeCLT_1.19.0/STLink-gdb-server/bin/ST-LINK_gdbserver --version
   ```

2. **检查 .elf 文件是否存在**

   ```bash
   ls -lh build/Debug/*.elf
   ```

3. **重新构建项目**
   - 按 `Cmd+Shift+B` (macOS) 或 `Ctrl+Shift+B` (Windows)
   - 或在终端运行：`cmake --build build/Debug`

4. **检查 launch.json 中的路径**
   - 确保 `executable` 路径正确
   - 确保所有工具链路径存在

### ❓ SVD 文件有什么用？

SVD (System View Description) 文件的作用：

- ✅ 在调试时查看外设寄存器（如 GPIO、UART、I2C 等）
- ✅ 提供友好的寄存器名称和位字段描述
- ✅ 实时监控外设状态

**非必需，但强烈推荐使用！**

---

## 芯片型号参考

### 📊 常用 STM32 芯片系列

| 芯片系列 | device 示例 | SVD 文件 | 典型应用 |
|---------|------------|----------|----------|
| STM32F0 | STM32F030C8 | STM32F030.svd | 入门级 |
| STM32F1 | STM32F103C8 | STM32F103.svd | 经典系列 |
| STM32F4 | STM32F407VG | STM32F407.svd | 高性能 |
| STM32F4 | STM32F429ZI | STM32F429.svd | 带显示 |
| STM32H7 | STM32H743ZI | STM32H743.svd | 旗舰级 |
| STM32L4 | STM32L476RG | STM32L476.svd | 低功耗 |
| STM32G4 | STM32G474RE | STM32G474.svd | 电机控制 |

### 🔍 查看所有支持的芯片

```bash
# 查看所有 SVD 文件（即所有支持的芯片系列）
ls /opt/ST/STM32CubeCLT_1.19.0/STMicroelectronics_CMSIS_SVD/ | sort

# 按系列分类查看
ls /opt/ST/STM32CubeCLT_1.19.0/STMicroelectronics_CMSIS_SVD/ | grep -E "STM32(F|H|L|G)" | sort -u
```

---

## 🎯 调试工作流程

### 标准流程

1. **编写代码**
2. **构建项目**（`Cmd+Shift+B` 或 Build 任务）
3. **连接调试器**（ST-LINK 连接电脑和开发板）
4. **启动调试**（按 `F5`）
5. **设置断点**（点击代码行号左侧）
6. **单步调试**
   - `F10` - 单步跳过（Step Over）
   - `F11` - 单步进入（Step Into）
   - `Shift+F11` - 单步跳出（Step Out）
   - `F5` - 继续运行（Continue）

### 调试面板功能

- **变量（Variables）** - 查看局部变量和全局变量
- **监视（Watch）** - 添加自定义表达式监视
- **调用堆栈（Call Stack）** - 查看函数调用链
- **断点（Breakpoints）** - 管理所有断点
- **外设（Peripherals）** - 查看 MCU 外设寄存器（需要 SVD 文件）
- **内存（Memory）** - 查看内存内容

---

## 💡 最佳实践

### 建议

1. **保存模板配置**
   - 将配置好的 `.vscode` 文件夹保存到云盘或版本控制
   - 新项目直接复制，只需改项目名

2. **使用 Git 忽略临时文件**

   ```gitignore
   # .gitignore
   build/
   .vscode/*.log
   ```

3. **为不同芯片创建配置模板**

   ```text
   templates/
   ├── STM32F1/
   │   └── .vscode/
   ├── STM32F4/
   │   └── .vscode/
   └── STM32H7/
       └── .vscode/
   ```

4. **验证工具链安装**

   ```bash
   # 验证脚本
   /opt/ST/STM32CubeCLT_1.19.0/GNU-tools-for-STM32/bin/arm-none-eabi-gcc --version
   /opt/ST/STM32CubeCLT_1.19.0/STLink-gdb-server/bin/ST-LINK_gdbserver --version
   ```

---

## 🆚 Cortex-Debug vs cppdbg

### 功能对比

| 特性 | Cortex-Debug | cppdbg |
|------|--------------|--------|
| 配置难度 | ⭐⭐ 简单 | ⭐⭐⭐⭐ 复杂 |
| 外设寄存器查看 | ✅ 支持（SVD） | ❌ 不支持 |
| 内存查看 | ✅ 图形化 | ⭐ 命令行 |
| RTOS 线程查看 | ✅ 原生支持 | ⭐ 有限支持 |
| 反汇编视图 | ✅ 优秀 | ⭐ 基础 |
| 性能分析 | ✅ 支持 | ❌ 不支持 |
| 社区支持 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **推荐度** | ⭐⭐⭐⭐⭐ | ⭐⭐ |

### 结论

**强烈推荐使用 Cortex-Debug！**

---

## 📚 参考资源

### 官方文档

- [Cortex-Debug 扩展](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug)
- [STM32CubeCLT 文档](https://www.st.com/en/development-tools/stm32cubeclt.html)
- [VS Code 调试文档](https://code.visualstudio.com/docs/editor/debugging)

### 相关文件

- 📄 `.vscode/launch.json.template` - 配置模板
- 📄 `.vscode/CORTEX_DEBUG_GUIDE.md` - 详细配置说明
- 🔧 `.vscode/init-debug.sh` - 自动化配置脚本

---

## 📞 需要帮助？

如果遇到问题：

1. 检查本文档的 [常见问题](#常见问题) 部分
2. 查看 `.vscode/CORTEX_DEBUG_GUIDE.md` 获取更多细节
3. 验证工具链安装和路径是否正确
4. 检查 ST-LINK 驱动是否正常工作

---

> 最后更新时间：2025-10-16  
> 适用工具链版本：STM32CubeCLT 1.19.0
