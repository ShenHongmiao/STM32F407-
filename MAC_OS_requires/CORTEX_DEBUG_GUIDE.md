# STM32 Cortex-Debug 快速配置指南

## 📦 前置条件

1. 安装 Cortex-Debug 扩展
2. 安装 STM32CubeCLT 工具链

## 🚀 在新工程中快速配置步骤

### 方法 1：使用模板（最快）

1. **复制配置文件到新工程**

   ```bash
   cp /path/to/template/.vscode/launch.json /path/to/new-project/.vscode/
   cp /path/to/template/.vscode/tasks.json /path/to/new-project/.vscode/
   ```

2. **修改 launch.json 中的关键参数**
   - 找到 `"executable"` 行，修改为您的 .elf 文件路径

     ```json
     "executable": "${workspaceFolder}/build/Debug/YOUR_PROJECT_NAME.elf"
     ```

   - 如果芯片型号不同，修改 `"device"`

     ```json
     "device": "STM32F407VG"  // 改为您的芯片型号
     ```

   - 如果芯片系列不同，修改 `"svdFile"`

     ```json
     "svdFile": "/opt/ST/STM32CubeCLT_1.19.0/STMicroelectronics_CMSIS_SVD/STM32F407.svd"
     ```

3. **修改 tasks.json（如果构建目录不同）**
   - 检查构建路径是否为 `build/Debug`
   - 如果不同，修改所有任务中的路径

4. **按 F5 开始调试！**

### 方法 2：VS Code 自动生成

1. 在新工程中按 `F5`
2. 选择 **"Cortex Debug"**
3. VS Code 会生成基础配置
4. 手动补充缺失的参数（参考模板）

## 📝 常用芯片型号对照

| 芯片系列 | device 参数 | SVD 文件名 |
|---------|------------|-----------|
| STM32F407 | STM32F407VG | STM32F407.svd |
| STM32F103 | STM32F103C8 | STM32F103.svd |
| STM32F429 | STM32F429ZI | STM32F429.svd |
| STM32H743 | STM32H743ZI | STM32H743.svd |
| STM32L476 | STM32L476RG | STM32L476.svd |

查看所有可用的 SVD 文件：

```bash
ls /opt/ST/STM32CubeCLT_1.19.0/STMicroelectronics_CMSIS_SVD/
```

## 🔍 查找 .elf 文件位置

不同构建系统的输出路径：

- **CMake**: `build/Debug/YOUR_PROJECT.elf` 或 `build/YOUR_PROJECT.elf`
- **Make**: `Debug/YOUR_PROJECT.elf`
- **STM32CubeIDE**: `Debug/YOUR_PROJECT.elf`

快速查找命令：

```bash
find . -name "*.elf" -type f
```

## 🎯 核心配置参数说明

### Cortex-Debug 配置

```json
{
  "name": "Cortex Debug",
  "type": "cortex-debug",           // 必须是 cortex-debug
  "executable": "path/to/file.elf", // 【必改】可执行文件路径
  "servertype": "stlink",           // 调试器类型（stlink/jlink/openocd）
  "device": "STM32F407VG",          // 【必改】芯片型号
  "interface": "swd",               // 接口类型（swd/jtag）
  "svdFile": "path/to/svd",         // 【推荐改】SVD 文件，用于查看外设
  "serverpath": "path/to/server",   // GDB 服务器路径
  "armToolchainPath": "path/to/gcc" // 工具链路径
}
```

## 💡 提示

1. **SVD 文件的作用**：
   - 可以在调试时查看外设寄存器
   - 提供更友好的寄存器名称显示
   - 非必需，但强烈推荐

2. **保存模板**：
   - 将配置好的 `.vscode` 文件夹保存到您的模板目录
   - 每次新建工程时复制过去，只需修改文件名即可

3. **自动化脚本**：
   可以创建一个脚本自动替换项目名：

   ```bash
   #!/bin/bash
   PROJECT_NAME=$1
   cp -r template/.vscode ./.vscode
   # 替换项目名
   sed -i '' "s/YOUR_PROJECT_NAME/${PROJECT_NAME}/g" .vscode/launch.json
   ```

## 🆚 Cortex-Debug vs cppdbg

| 特性 | Cortex-Debug | cppdbg |
|------|-------------|--------|
| 易用性 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| 外设查看 | ✅ 支持（需SVD） | ❌ 不支持 |
| 内存查看 | ✅ 图形化 | ⭐ 基础 |
| RTOS 支持 | ✅ 优秀 | ⭐ 有限 |
| 配置复杂度 | 简单 | 较复杂 |
| **推荐度** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |

**结论**：强烈推荐使用 Cortex-Debug！
