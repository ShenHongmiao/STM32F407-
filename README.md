# STM32F407 温度气压检测与控制

## 项目简介

基于 STM32F407 微控制器的多传感器监控系统，采用 FreeRTOS 实时操作系统，实现气压温度传感器（WF5803F）、NTC 热敏电阻温度检测以及电源电压监控功能。

## 硬件平台

- **MCU**: STM32F407VET6
- **核心板**: STM32F407 最小系统板
- **系统时钟**: 72MHz (HSI + PLL)
- **RTOS**: FreeRTOS v10.x

## 功能特性

### 1. 上位机串口通信 (UART2)

- **通信接口**: UART2 (PD5/PD6)
- **波特率**: 115200
- **接收方式**: 中断 + FreeRTOS 队列（队列大小 16 字节）
- **任务优先级**: Realtime（最高优先级，确保实时响应）
- **工作模式**: 阻塞等待接收，死等上位机命令
- **数据输出**: 传感器数据和系统状态通过 UART2 发送到上位机

**实现细节**:

- USART2 中断接收每个字节后，通过 `osMessagePut()` 放入队列
- `receiveAndTargetChange` 任务使用 `osMessageGet(osWaitForever)` 阻塞读取
- 中断优先级设置为 6，满足 FreeRTOS API 调用要求 (≥ 5)
- **接收回调函数在 `HAL_UART_RxCpltCallback()` 中实现**，请勿在其中放入过长代码

**中断优先级配置**:

```c
// usart.c - USART2 中断配置
HAL_NVIC_SetPriority(USART2_IRQn, 6, 0);  // 优先级 6 (≥ 5)
HAL_NVIC_EnableIRQ(USART2_IRQn);
```

**队列创建**:

```c
// freertos.c - 队列初始化
osMessageQDef(usart_rx_queue, 16, uint8_t);
usart_rx_queueHandle = osMessageCreate(osMessageQ(usart_rx_queue), NULL);
```

**中断处理函数**:

```c
// stm32f4xx_it.c - USART2 中断服务函数
void USART2_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart2);
}
```

**接收回调**:

```c
// usart.c - 接收完成回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2) {
    osMessagePut(usart_rx_queueHandle, (uint32_t)rx_byte, 0);
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
  }
}
```

**任务实现**:
```c
// freertos.c - USART 接收任务
void StartReceiveAndTargetChangeTask(void const * argument)
{
  for(;;) {
    event = osMessageGet(usart_rx_queueHandle, osWaitForever);
    if (event.status == osEventMessage) {
      uint8_t received_byte = (uint8_t)event.value.v;
      // 处理接收到的命令...
    }
  }
}
```

### 2. 气压温度传感器 (WF5803F)

- **通信接口**: I2C1 (PB8/PB9)
- **测量范围**: 0-2 Bar (气压), -40°C ~ 125°C (温度)
- **采样频率**: 1Hz
- **数据输出**: 通过 UART 串口输出

### 3. NTC 温度检测

- **硬件接口**: ADC1_IN0 (PA0)
- **传感器类型**: 10kΩ NTC 热敏电阻 (B=3380)
- **电路配置**: 分压电路 (10kΩ串联电阻)
- **采样频率**: 1Hz
- **温度范围**: -40°C ~ 125°C

### 4. 电源电压监控

- **检测引脚**: PC4 (V_DETECT)
- **检测方式**: ADC采样
- **监控策略**:
  - 上电时立即检测电压
  - 运行时每 10 分钟检测一次
  - 低于 70% 阈值时挂起传感器任务并发送警告
- **低压保护**: 自动挂起非关键任务以降低功耗

### 5. 温度 PID 控制系统

- **控制算法**: 增量式 PID 控制器
- **目标温度**: 可配置（默认 50°C）
- **控制输出**: TIM3 硬件 PWM（0-1000ms 占空比）
- **控制引脚**: PC6 (TIM3_CH1), PC7 (TIM3_CH2)
- **PWM 周期**: 1000ms（1Hz）
- **安全保护**
  - 紧急最高温度限制（80°C）
  - 安全关机温度（75°C）
  - 温度死区控制（±0.5°C）
- **PID 参数**: 根据目标温度自动选择（低温/中温/高温区域）

### 6. NMOS 控制输出

- **控制引脚**: PC6, PC7, PC8, PC9
- **功能**: 4路 NMOS 驱动控制
- **PC6/PC7**: TIM3 硬件 PWM 控制（用于加热温控，周期 1000ms）
- **PC8/PC9**: GPIO 普通输出控制
- **PWM 极性**: 低电平有效（LOW polarity）- 占空比 0 = 高电平 = 加热关闭

## 引脚定义

### 电源引脚

| 核心板引脚 | STM32引脚 | 功能描述 |
|-----------|----------|---------|
| 2, 4 | +5V | 5V 电源输入 |
| 35 | +3V3 | 3.3V 电源输出 |
| 1, 30-33, 60 | GND | 地线 |
| 3 | AGND | 模拟地 |

### I2C 接口

| 核心板引脚 | STM32引脚 | 功能 | 备注 |
|-----------|----------|------|------|
| 44 | PB8 | I2C1_SCL | WF5803F 气压传感器 |
| 42 | PB9 | I2C1_SDA | WF5803F 气压传感器 |
| 21 | PE11 | I2C2_SCL | 预留 I2C2 接口 |
| 19 | PE9 | I2C2_SDA | 预留 I2C2 接口 |

### ADC 接口

| 核心板引脚 | STM32引脚 | 通道 | 功能 |
|-----------|----------|------|------|
| 5 | PA0 | ADC1_IN0 | NTC 温度传感器 |
| 7 | PA1 | ADC1_IN1 | 预留 ADC 通道 |
| 13 | PC4 | - | V_DETECT 电压检测 |

### UART 接口

| 核心板引脚 | STM32引脚 | 功能 | 用途 |
|-----------|----------|------|------|
| 48 | PB6 | UART1_TX | 调试串口输出（保留） |
| 46 | PB7 | UART1_RX | 调试串口输入（保留） |
| 39 | PD5 | UART2_TX | 数据输出串口 |
| 36 | PD6 | UART2_RX | 上位机命令接收串口 |

### PWM/GPIO 控制

| 核心板引脚 | STM32引脚 | 功能 | 模式 | 备注 |
|-----------|----------|------|------|------|
| 59 | PC6 | NMOS1_G / TIM3_CH1 | 硬件PWM | 加热控制（1000ms周期，低电平有效） |
| 58 | PC7 | NMOS2_G / TIM3_CH2 | 硬件PWM | 加热控制（1000ms周期，低电平有效） |
| 56 | PC9 | NMOS3_G | GPIO输出 | 普通GPIO控制 |
| 54 | PC8 | NMOS4_G | GPIO输出 | 普通GPIO控制 |

### 调试接口

| 核心板引脚 | STM32引脚 | 功能 |
|-----------|----------|------|
| 55 | PA14 | SWDIO |
| 53 | PC11 | SWCLK |
| 50 | NRST | NRST (复位) |

## 软件架构

### FreeRTOS 任务

| 任务名称 | 优先级 | 栈大小 | 功能描述 |
|---------|-------|--------|---------|

| defaultTask | High | 256 | 上电初始化，执行首次电压检测后自删除 |
| receiveAndTargetChange | Realtime | 256 | USART2 接收任务，阻塞等待上位机命令 |
| Sensors_and_compute | Normal | 512 | WF5803F 传感器数据读取和 NTC 温度采集 (1Hz) |
| voltageMonitorTask | Low | 256 | 电源电压监控 (每10分钟检测) |

### 任务执行流程

```text
系统上电
   ↓
defaultTask (最高优先级)
   ├─ 延时 500ms (系统稳定)
   ├─ 执行首次电压检测
   ├─ 电压正常？
   │    ├─ 是: 发送正常消息，所有任务正常运行
   │    └─ 否: 发送低压警告，挂起传感器任务
   └─ 任务自删除
   ↓
sensorTask & ntcTask (正常运行/挂起)
   ├─ 每秒采集传感器数据
   └─ 通过 UART1 输出数据
   ↓
voltageMonitorTask (低优先级后台运行)
   ├─ 每 10 分钟检测电压
   └─ 低压时挂起传感器任务并持续发送警告
```

### 低压保护机制

1. **上电检测**: 系统启动后立即检测电压
2. **周期检测**: 运行期间每 10 分钟检测一次
3. **动作策略**:
   - 电压 < 70% 阈值: 挂起传感器任务，持续发送低压警告
   - 电压正常: 发送正常消息
4. **恢复策略**: 需要重启系统才能恢复任务运行

## 外设配置

### I2C1 配置

- **速度**: 100 kHz (标准模式)
- **地址模式**: 7-bit
- **设备地址**: 0x6C (WF5803F)

### ADC1 配置

- **分辨率**: 12-bit (0-4095)
- **参考电压**: 3.3V
- **采样时间**: 15 cycles
- **触发方式**: 软件触发

### UART 配置

#### UART1 (预留调试口)

- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验**: None
- **流控**: None

#### UART2 (上位机通信)

- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验**: None
- **流控**: None
- **中断优先级**: 6 (必须 ≥ configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)
- **接收模式**: 中断接收 + FreeRTOS 队列
- **队列大小**: 16 字节

## 编译与烧录

### 环境要求

- CMake 3.20+
- ARM GCC Toolchain
- STM32CubeMX
- STM32 Programmer CLI (用于烧录)

### 首次克隆项目后的构建步骤

⚠️ **重要提示**：本项目的 `.gitignore` 已配置排除所有编译缓存和构建产物，因此从 Git 克隆后需要完整构建。

**第一次构建的完整步骤：**

1. **克隆仓库**

   ```bash
   git clone https://github.com/ShenHongmiao/STM32F407-.git
   cd STM32F407-
   ```

2. **配置 CMake**（使用预设配置）

   ```bash
   cmake --preset Debug
   ```

3. **编译项目**

   ```bash
   cmake --build build/Debug
   ```

4. **烧录到开发板**（方式一：使用 CMake Task）
   - 在 VS Code 中按 `Ctrl+Shift+P`
   - 选择 `Tasks: Run Task`
   - 选择 `Build + Flash`

5. **烧录到开发板**（方式二：命令行）

   ```bash
   STM32_Programmer_CLI --connect port=swd --download build/Debug/I2C.elf -hardRst -rst --start
   ```

**常见问题排查：**

| 问题 | 原因 | 解决方法 |
|------|------|---------|
| CMake 找不到工具链 | 未设置 ARM GCC 路径 | 检查 `cmake/gcc-arm-none-eabi.cmake` 中的工具链路径 |
| 找不到可执行文件 | 未构建或路径错误 | 运行 `cmake --build build/Debug` 重新编译 |
| ST-Link 连接失败 | 硬件未连接或驱动问题 | 检查 ST-Link 连接，安装 ST-Link 驱动 |
| 路径包含中文报错 | CMake 不支持中文路径 | 将项目移动到纯英文路径 |

**`.gitignore` 已排除的文件：**

- ✅ `build/` - CMake 构建输出目录
- ✅ `*.elf`, `*.bin`, `*.hex`, `*.map` - 编译生成的可执行文件
- ✅ `CMakeCache.txt`, `CMakeFiles/` - CMake 缓存
- ✅ `*.o`, `*.obj` - 目标文件
- ✅ `.ninja_deps`, `.ninja_log`, `build.ninja` - Ninja 构建文件

这些文件不会提交到 Git，因此每次克隆后都需要重新构建。

### 串口输出示例

**正常运行输出（UART2）：**

```text
=== USART Receive Task Started (Priority: Realtime) ===
Waiting for USART2 data...
=== Sensors_and_compute Task Started! ===
WF5803-Temp: 25.30 C, Press: 101.25 kPa
NTC-Temp: 24.80C
```

**接收上位机命令示例：**

```text
Received byte from USART2: '5' (0x35)
Command: Set heating to 50%

Received byte from USART2: 'h' (0x68)
Help: Send 0-9 to control heating power

Received byte from USART2: 'X' (0x58)
Unknown command: 'X'
```

### 低压警告输出

```text
[PERIODIC] Voltage check...
Voltage: 3.20V
!!! LOW VOLTAGE ALERT !!!
Suspending All tasks...
Sensor task suspended due to low voltage!
NTC task suspended due to low voltage!
```

## 注意事项

1. **电源要求**
   - 输入电压: 24V (±5%)
   - 建议使用稳定的电源适配器
   - 低压阈值: 16.8V (70% of 24)

2. **I2C 上拉电阻**
   - 确保 I2C 总线有 4.7kΩ 上拉电阻
   - WF5803F 需要稳定的电源供应

3. **NTC 接线**
   - NTC 与 10kΩ 串联电阻构成分压电路
   - 确保 PA0 与 AGND 之间连接正确

4. **调试接口**
   - 使用 ST-Link V2 或兼容调试器
   - SWD 接口: SWDIO (PA14), SWCLK (PC11)

5. **任务挂起**
   - 低压保护触发后，传感器任务会被挂起
   - 需要重启系统才能恢复正常运行
   - 电压监控任务会持续发送低压警告

6. **Git 克隆与构建**
   - ⚠️ 本项目 `.gitignore` 已排除所有编译产物和构建缓存
   - 从 Git 克隆后必须重新运行 CMake 配置和编译
   - 如果遇到 "找不到可执行文件" 错误，请先执行:
     1. `cmake --preset Debug`
     2. `cmake --build build/Debug`
   - 不同电脑环境可能需要调整 `cmake/gcc-arm-none-eabi.cmake` 中的工具链路径
   - 建议使用纯英文路径，避免中文路径导致的 CMake 错误

## 开发工具

- **IDE**: Visual Studio Code + STM32 Extension
- **编译器**: ARM GCC 10.3.1 (或更高版本)
- **调试器**: ST-Link V2 / J-Link
- **生成工具**: STM32CubeMX 6.x
- **构建系统**: CMake + Ninja

## 项目结构

```text
I2C/
├── Core/
│   ├── Inc/               # 头文件
│   │   ├── main.h
│   │   ├── i2c.h
│   │   ├── adc.h
│   │   ├── usart.h
│   │   ├── gpio.h
│   │   ├── WF5803F.h      # 气压传感器驱动
│   │   ├── NTC.h          # NTC 温度传感器驱动
│   │   ├── temp_pid_ctrl.h # PID 温度控制器
│   │   ├── V_detect.h     # 电压检测
│   │   └── FreeRTOSConfig.h
│   └── Src/               # 源文件
│       ├── main.c
│       ├── freertos.c     # FreeRTOS 任务实现
│       ├── i2c.c
│       ├── adc.c
│       ├── usart.c
│       ├── gpio.c
│       ├── WF5803F.c
│       ├── NTC.c
│       ├── temp_pid_ctrl.c # PID 温度控制实现
│       └── V_detect.c     # 电压检测实现
├── Drivers/
│   ├── STM32F4xx_HAL_Driver/  # STM32 HAL 库
│   └── CMSIS/                  # CMSIS 核心文件
├── Middlewares/
│   └── Third_Party/
│       └── FreeRTOS/          # FreeRTOS 源码
├── build/                     # 编译输出目录
├── CMakeLists.txt            # CMake 配置
├── CMakePresets.json         # CMake 预设
├── I2C.ioc                   # STM32CubeMX 项目文件
└── README.md                 # 本文档
```

## 详细调试相关内容

本节介绍如何根据不同应用场景修改程序中的宏定义参数。

### 1. 电压检测配置 (`Core/Inc/V_detect.h`)

根据实际硬件分压电路和电源电压修改：

```c
// ========== 分压电阻配置 ==========
#define VOLTAGE_R1           51000.0f    // 上分压电阻 R21 (Ω)
#define VOLTAGE_R2           5100.0f     // 下分压电阻 R22 (Ω)
#define VOLTAGE_RATIO        (VOLTAGE_R2 / (VOLTAGE_R1 + VOLTAGE_R2))

// ========== 电压阈值配置 ==========
#define VOLTAGE_NORMAL       24.0f       // 标准工作电压 (V)
#define VOLTAGE_THRESHOLD    (VOLTAGE_NORMAL * 0.7f)  // 低压阈值 (70%)

// ========== ADC 参数配置 ==========
#define ADC_MAX_VALUE        4095.0f     // 12-bit ADC 最大值
#define ADC_VREF             3.3f        // ADC 参考电压 (V)
```

**修改场景：**

| 场景 | 需要修改的宏 | 修改示例 |
|------|------------|---------|
| 更换电源电压 (12V) | `VOLTAGE_NORMAL` | `#define VOLTAGE_NORMAL 12.0f` |
| 修改低压阈值 (60%) | `VOLTAGE_THRESHOLD` | `#define VOLTAGE_THRESHOLD (VOLTAGE_NORMAL * 0.6f)` |
| 更换分压电阻 | `VOLTAGE_R1`, `VOLTAGE_R2` | 根据实际电阻值修改 |
| 使用外部参考电压 | `ADC_VREF` | `#define ADC_VREF 3.0f` |

**分压电阻计算公式：**

```text
ADC_IN = V_SOURCE × (R2 / (R1 + R2))
V_SOURCE = ADC_IN × (R1 + R2) / R2
```

**注意事项：**

- ADC 输入电压不得超过 3.3V
- 分压比应确保：`V_SOURCE_MAX × VOLTAGE_RATIO ≤ 3.3V`
- 例如 24V 系统：`24V × 0.0909 = 2.18V < 3.3V` ✅

---

### 2. NTC 温度传感器配置 (`Core/Inc/NTC.h`)

根据 NTC 热敏电阻规格修改：

```c
// ========== NTC 参数配置 ==========
#define NTC_BETA       3380.0f      // Beta 常数 (B值 25°C/50°C)
#define NTC_R0         10000.0f     // 25°C 时的阻值 (Ω)
#define NTC_T0         298.15f      // 参考温度 (25°C = 298.15K)
#define NTC_R_SERIES   10000.0f     // 串联电阻值 (Ω)

// ========== ADC 参数配置 ==========
#define ADC_MAX_VALUE  4095.0f      // 12-bit ADC 最大值
#define V_REF          3.3f         // 参考电压 (V)
```

**修改场景：**

| 场景 | 需要修改的宏 | 修改示例 |
|------|------------|---------|
| 更换 NTC 型号 (B=3950) | `NTC_BETA`, `NTC_R0` | `#define NTC_BETA 3950.0f` 和 `#define NTC_R0 10000.0f` |
| 使用 100kΩ NTC | `NTC_R0`, `NTC_R_SERIES` | `#define NTC_R0 100000.0f` 和 `#define NTC_R_SERIES 100000.0f` |
| 更换串联电阻 | `NTC_R_SERIES` | `#define NTC_R_SERIES 4700.0f` |

**常见 NTC 型号参数：**

| NTC 型号 | R0 (25°C) | Beta 值 | 温度范围 |
|---------|----------|---------|---------|
| MF52-103 | 10kΩ | 3380 | -40°C ~ 125°C |
| MF58-103 | 10kΩ | 3950 | -50°C ~ 150°C |
| NTCLE100E3 | 10kΩ | 3977 | -40°C ~ 125°C |

**温度计算公式 (Steinhart-Hart 简化)：**

```text
R_NTC = R_SERIES × (ADC_MAX / ADC_VALUE - 1)
T(K) = 1 / (1/T0 + (1/BETA) × ln(R_NTC/R0))
T(°C) = T(K) - 273.15
```

---

### 3. WF5803F 传感器配置 (`Core/Inc/WF5803F.h`)

根据传感器地址和寄存器定义修改：

```c
// ========== I2C 设备地址 ==========
#define WF5803F_ADDR          (0x6C<<1)   // I2C 7位地址左移

// ========== 寄存器地址 ==========
#define WF5803F_REG_CTRL      0x30        // 控制寄存器
#define WF5803F_REG_STATUS    0x02        // 状态寄存器
#define WF5803F_REG_PRESS_MSB 0x06        // 气压数据 MSB
#define WF5803F_REG_TEMP_MSB  0x09        // 温度数据 MSB
```

**修改场景：**

| 场景 | 需要修改的宏 | 修改示例 |
|------|------------|---------|
| 更换 I2C 地址 | `WF5803F_ADDR` | `#define WF5803F_ADDR (0x6D<<1)` |
| 不同传感器型号 | 查阅数据手册 | 根据寄存器映射修改 |

**气压量程配置 (`Core/Src/WF5803F.c`)：**

当前代码支持 **2 Bar** 量程传感器，计算公式在 `compute_pressure_WF5803F_2BAR_fromInt()` 中：

```c
float compute_pressure_WF5803F_2BAR_fromInt(int32_t rawData) {
    // 保留 24位数据
    rawData &= 0x00FFFFFF;
    
    // 如果最高位 (bit23) 为1，则需要做符号扩展
    if (rawData & 0x00800000) {
        rawData |= 0xFF000000;
    }
    
    // 归一化到 [-1, +1)
    float factor = (float)rawData / 8388608.0f;  // 除以 2^23 (n = factor)
    
    // 固定公式（2bar型号，输出 kPa）
    return 180.0f/0.81f*(factor-0.1f)+30.0f;
}
```

**支持的传感器型号及计算公式：**

所有公式中 `n = factor`（归一化值，范围 [-1, +1)），输出单位均为 **kPa**。

**⚠️注意：除了本项目使用的型号外，均未进行测试，不保证其他型号公式正确可以正常使用。**

#### WF5803F 系列（气压传感器）

| 型号 | 量程 | 计算公式 | C 代码实现 |
|------|------|---------|-----------|
| **WF5803F-1BAR** | 0-1 Bar | P = 125×n + 17.5 | `return 125.0f*factor + 17.5f;` |
| **WF5803F-2BAR** ⭐ | 0-2 Bar | P = (180/0.81)×(n-0.1) + 30 | `return 180.0f/0.81f*(factor-0.1f)+30.0f;` |
| **WF5803F-7BAR** | 0-7 Bar | P = 1000×n - 50 | `return 1000.0f*factor - 50.0f;` |
| **WF5803F-12BAR** | 0-12 Bar | P = 700×n + 530 | `return 700.0f*factor + 530.0f;` |

⭐ 表示当前代码使用的型号

#### WF100D 系列（差压传感器）

| 型号 | 量程 | 计算公式 | C 代码实现 |
|------|------|---------|-----------|
| **WF100D-5KPA** | 0-5 kPa | P = 5×n + 0.25 | `return 5.0f*factor + 0.25f;` |
| **WF100D-10KPA** | 0-10 kPa | P = 15×n + 2 | `return 15.0f*factor + 2.0f;` |
| **WF100D-40KPA** | 0-40 kPa | P = 50×n + 5 | `return 50.0f*factor + 5.0f;` |
| **WF100D-100KPA** | 0-100 kPa | P = 125×n - 12.5 | `return 125.0f*factor - 12.5f;` |
| **WF100D-200KPA** | 0-200 kPa | P = 250×n + 25 | `return 250.0f*factor + 25.0f;` |
| **WF100D-300KPA** | 0-300 kPa | P = 400×n + 10 | `return 400.0f*factor + 10.0f;` |

#### WF200D 系列（差压传感器）

| 型号 | 量程 | 计算公式 | C 代码实现 |
|------|------|---------|-----------|
| **WF200D-5KPA** | 0-5 kPa | P = 5×n + 0.25 | `return 5.0f*factor + 0.25f;` |

**更换传感器型号的步骤：**

1. **修改头文件** (`Core/Inc/WF5803F.h`)：
   - 将函数名改为对应型号，例如：`compute_pressure_WF5803F_7BAR_fromInt()`

2. **修改源文件** (`Core/Src/WF5803F.c`)：
   - 修改函数定义和函数调用
   - 更换计算公式（从上表中复制对应的 C 代码）

3. **示例：更换为 7BAR 型号**

   ```c
   // 原公式（2BAR）
   return 180.0f/0.81f*(factor-0.1f)+30.0f;
   
   // 替换为（7BAR）
   return 1000.0f*factor - 50.0f;
   ```

4. **重新编译并烧录程序**

---

### 4. FreeRTOS 任务配置 (`Core/Src/freertos.c`)

根据系统需求调整任务参数：

```c
// ========== 任务优先级定义 ==========
osThreadDef(defaultTask, StartDefaultTask, osPriorityRealtime, 0, 256);
osThreadDef(sensorTask, StartSensorTask, osPriorityNormal, 0, 512);
osThreadDef(ntcTask, StartNTCTask, osPriorityNormal, 0, 256);
osThreadDef(voltageMonitor, StartVoltageMonitorTask, osPriorityLow, 0, 256);

// ========== 电压检测间隔 ==========
#define VOLTAGE_CHECK_INTERVAL  600000  // 10分钟 (ms)
```

**修改场景：**

| 场景 | 需要修改的参数 | 修改示例 |
|------|--------------|---------|
| 缩短电压检测间隔 (5分钟) | `VOLTAGE_CHECK_INTERVAL` | `#define VOLTAGE_CHECK_INTERVAL 300000` |
| 增加传感器任务优先级 | 任务优先级 | 改为 `osPriorityAboveNormal` |
| 减少栈大小节省内存 | 栈大小参数 | 将 512 改为 256 (需确保不会栈溢出) |

**可用的优先级等级：**

- `osPriorityIdle` - 空闲
- `osPriorityLow` - 低
- `osPriorityBelowNormal` - 低于正常
- `osPriorityNormal` - 正常 ⭐
- `osPriorityAboveNormal` - 高于正常
- `osPriorityHigh` - 高
- `osPriorityRealtime` - 实时 ⚠️

**任务采样频率调整：**

在任务函数中修改 `osDelay()` 参数：

```c
void StartSensorTask(void const * argument) {
    for(;;) {
        // ... 读取传感器 ...
        
        osDelay(1000);  // 1Hz 采样 (1000ms)
        // osDelay(500);   // 2Hz 采样 (500ms)
        // osDelay(2000);  // 0.5Hz 采样 (2000ms)
    }
}
```

---

### 5. UART 串口调试输出配置

**启用/禁用调试输出：**

在各任务函数中，使用条件编译控制 `printf()` 输出：

```c
// 在文件顶部添加宏定义
#define DEBUG_ENABLE  1  // 1=启用调试, 0=禁用调试

// 在代码中使用
#if DEBUG_ENABLE
    printf("Debug message\n");
#endif
```

**波特率修改 (CubeMX 配置)：**

1. 打开 `I2C.ioc` 文件
2. 找到 USART1 配置
3. 修改 Baud Rate 参数
4. 重新生成代码

**常用波特率：**

- 9600 (低速，兼容性好)
- 115200 (当前配置) ⭐
- 460800 (高速数据传输)
- 921600 (最高速，需要硬件支持)

---

### 6. 常见调试场景快速配置

#### 场景 1: 更换为 12V 电源系统

```c
// V_detect.h
#define VOLTAGE_NORMAL       12.0f
#define VOLTAGE_THRESHOLD    (VOLTAGE_NORMAL * 0.7f)  // 8.4V
#define VOLTAGE_R1           27000.0f    // 调整分压电阻
#define VOLTAGE_R2           5100.0f
```

#### 场景 2: 提高传感器采样率到 10Hz

```c
// freertos.c - StartSensorTask()
osDelay(100);  // 改为 100ms (10Hz)

// freertos.c - StartNTCTask()
osDelay(100);  // 改为 100ms (10Hz)
```

#### 场景 3: 缩短电压监控间隔到 1 分钟

```c
// freertos.c
#define VOLTAGE_CHECK_INTERVAL  60000  // 1分钟
```

#### 场景 4: 更换为 100kΩ NTC

```c
// NTC.h
#define NTC_R0         100000.0f
#define NTC_R_SERIES   100000.0f  // 建议使用相同阻值
#define NTC_BETA       3950.0f    // 根据实际型号修改
```

### 7. TIM3 硬件 PWM 配置（温度控制）

本项目已将加热 MOS 管（PC6/PC7）改为由 TIM3 定时器硬件 PWM 输出控制，周期为 1000ms，占空比可调。

#### 主要特性

- **PWM 周期**: 1000ms (1Hz)
- **计数频率**: 10kHz
- **PWM 极性**: `TIM_OCPOLARITY_LOW`（低电平有效）
  - 占空比 0ms → 输出高电平 → 加热关闭 ✅
  - 占空比 1000ms → 输出低电平 → 加热全开 🔥
- **控制范围**: 0-1000ms（对应 0-10000 计数值）

#### 硬件极性说明

⚠️ **重要**: 本项目硬件设计为**低电平驱动加热**，因此 PWM 配置为 `TIM_OCPOLARITY_LOW`：

- 当 `Set_Heating_PWM(0)` 时：PWM 输出保持高电平 → MOS 管关闭 → 加热关闭（安全状态）
- 当 `Set_Heating_PWM(1000)` 时：PWM 输出保持低电平 → MOS 管导通 → 加热全功率

这样确保了系统断电或故障时，默认状态为高电平，加热器处于关闭状态，符合安全设计原则。

#### 主要修改内容

- 在 `main.c` 中添加了 `MX_TIM3_Init()`，配置 TIM3 为 PWM 输出，周期 1000ms，极性为 LOW。
- 在 `gpio.c` 中将 PC6/PC7 配置为定时器复用功能（`GPIO_MODE_AF_PP` + `GPIO_AF2_TIM3`）。
- 在 `main.c` 初始化流程中调用 TIM3 初始化和启动 PWM。
- 在 `temp_pid_ctrl.c` 中添加了 `Set_Heating_PWM(uint16_t duty_ms)`，用于设置占空比（0-1000ms）。

#### 使用方法

1. **初始化**
   - 在 `main.c` 的初始化流程中已自动调用 `MX_TIM3_Init()` 和 `HAL_TIM_PWM_Start()`，无需手动启动。

2. **设置占空比**
   - 在温度控制任务或PID计算后，调用：

     ```c
     Set_Heating_PWM(duty_ms); // duty_ms范围0-1000
     ```

   - 例如：

     ```c
     uint16_t duty_ms = (uint16_t)PID_Compute(...);
     Set_Heating_PWM(duty_ms);
     ```

3. **引脚说明**
   - PC6: TIM3_CH1 (硬件 PWM 输出，低电平有效)
   - PC7: TIM3_CH2 (硬件 PWM 输出，低电平有效)
   - PC8: GPIO 普通输出
   - PC9: GPIO 普通输出

4. **定时器参数说明**
   - 计数频率: 10kHz
   - 周期: 9999 计数（对应 1000ms）
   - PWM 极性: `TIM_OCPOLARITY_LOW`
   - 占空比设置范围: 0-1000ms（实际传入 `Set_Heating_PWM()` 后会自动转换为 0-10000 计数）
   - 初始状态: 占空比为 0（输出高电平，加热关闭）

#### CubeMX/手动配置说明

如需在 CubeMX 中重新配置：

1. **TIM3 配置**
   - Mode: PWM Generation CH1/CH2
   - Prescaler: `(SystemCoreClock / 10000) - 1` = 7199（假设 72MHz 系统时钟）
   - Counter Period (ARR): 9999（对应 1000ms）
   - Pulse (CCR): 0（初始占空比）
   - PWM Mode: PWM Mode 1
   - **⚠️ 重要**: CH1/CH2 Polarity 必须设置为 `Low`

2. **GPIO 配置**
   - PC6: 选择为 TIM3_CH1（复用功能 AF2）
   - PC7: 选择为 TIM3_CH2（复用功能 AF2）
   - PC8/PC9: 保持为 GPIO_Output

#### 兼容性说明

- 该方案不再依赖软件 PWM，不受 FreeRTOS 任务调度影响，PWM 波形精确稳定。
- 只需调用 `Set_Heating_PWM()` 即可实现加热功率调节。
- **硬件要求**: 系统硬件必须是低电平驱动加热（PC6/PC7 低电平时 MOS 管导通）。
- **安全性**: 系统断电或复位时，GPIO 默认为高电平，加热器自动关闭。

#### 相关代码片段

```c
// main.c - TIM3 初始化（注意 OCPolarity 配置）
void MX_TIM3_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};
  
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = (SystemCoreClock / 10000) - 1;  // 72MHz → 10kHz
  htim3.Init.Period = 9999;                              // 10kHz / 10000 = 1Hz (1000ms)
  HAL_TIM_PWM_Init(&htim3);
  
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;  // 初始占空比为 0（输出高电平，加热关闭）
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;  // ⚠️ 关键：低极性配置
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);
}

// main.c - 启动 PWM
MX_TIM3_Init();
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

// temp_pid_ctrl.c - 设置占空比
void Set_Heating_PWM(uint16_t duty_ms)
{
    if (duty_ms > 1000) duty_ms = 1000;
    uint32_t pulse = duty_ms * 10;  // 1000ms 对应 10000 计数
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);  // PC6
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);  // PC7
}

// gpio.c - GPIO 配置
// PC6/PC7 配置为 TIM3 复用功能
GPIO_InitStruct.Pin = NMOS1_G_Pin | NMOS2_G_Pin;  // PC6/PC7
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

// PC8/PC9 配置为普通 GPIO 输出
GPIO_InitStruct.Pin = NMOS3_G_Pin | NMOS4_G_Pin;  // PC8/PC9
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
```

---
如有疑问可随时联系开发者。
