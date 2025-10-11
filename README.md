# STM32F407 多传感器监控系统

## 项目简介

基于 STM32F407 微控制器的多传感器监控系统，采用 FreeRTOS 实时操作系统，实现气压温度传感器（WF5803F）、NTC 热敏电阻温度检测以及电源电压监控功能。

## 硬件平台

- **MCU**: STM32F407VET6
- **核心板**: STM32F407 最小系统板
- **系统时钟**: 72MHz (HSI + PLL)
- **RTOS**: FreeRTOS v10.x

## 功能特性

### 1. 气压温度传感器 (WF5803F)
- **通信接口**: I2C1 (PB8/PB9)
- **测量范围**: 0-7 Bar (气压), -40°C ~ 125°C (温度)
- **采样频率**: 1Hz
- **数据输出**: 通过 UART 串口输出

### 2. NTC 温度检测
- **硬件接口**: ADC1_IN0 (PA0)
- **传感器类型**: 10kΩ NTC 热敏电阻 (B=3380)
- **电路配置**: 分压电路 (10kΩ串联电阻)
- **采样频率**: 1Hz
- **温度范围**: -40°C ~ 125°C

### 3. 电源电压监控
- **检测引脚**: PC4 (V_DETECT)
- **检测方式**: ADC采样
- **监控策略**: 
  - 上电时立即检测电压
  - 运行时每 10 分钟检测一次
  - 低于 70% 阈值时挂起传感器任务并发送警告
- **低压保护**: 自动挂起非关键任务以降低功耗

### 4. NMOS 控制输出
- **控制引脚**: PC6, PC7, PC8, PC9
- **功能**: 4路 NMOS 驱动控制

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
| 48 | PB6 | UART1_TX | 调试串口输出 |
| 46 | PB7 | UART1_RX | 调试串口输入 |
| 39 | PD5 | UART2_TX | 预留串口 |
| 36 | PD6 | UART2_RX | 预留串口 |

### GPIO 控制
| 核心板引脚 | STM32引脚 | 功能 | 备注 |
|-----------|----------|------|------|
| 59 | PC6 | NMOS1_G | NMOS 1 栅极控制 |
| 58 | PC7 | NMOS2_G | NMOS 2 栅极控制 |
| 56 | PC9 | NMOS3_G | NMOS 3 栅极控制 |
| 54 | PC8 | NMOS4_G | NMOS 4 栅极控制 |

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
| defaultTask | Realtime | 256 | 上电初始化，执行首次电压检测后自删除 |
| sensorTask | Normal | 512 | WF5803F 传感器数据读取 (1Hz) |
| ntcTask | Normal | 256 | NTC 温度采集 (1Hz) |
| voltageMonitorTask | Low | 256 | 电源电压监控 (每10分钟检测) |

### 任务执行流程

```
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

### UART1 配置
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验**: None
- **流控**: None

## 编译与烧录

### 环境要求
- CMake 3.20+
- ARM GCC Toolchain
- STM32CubeMX
- STM32 Programmer CLI (用于烧录)


### 低压警告输出

```
[PERIODIC] Voltage check...
Voltage: 3.20V
!!! LOW VOLTAGE ALERT !!!
Suspending All tasks...
Sensor task suspended due to low voltage!
NTC task suspended due to low voltage!
```

## 注意事项

1. **电源要求**: 
   - 输入电压: 24V (±5%)
   - 建议使用稳定的电源适配器
   - 低压阈值: 16.8V (70% of 24)

2. **I2C 上拉电阻**: 
   - 确保 I2C 总线有 4.7kΩ 上拉电阻
   - WF5803F 需要稳定的电源供应

3. **NTC 接线**: 
   - NTC 与 10kΩ 串联电阻构成分压电路
   - 确保 PA0 与 AGND 之间连接正确

4. **调试接口**: 
   - 使用 ST-Link V2 或兼容调试器
   - SWD 接口: SWDIO (PA14), SWCLK (PC11)

5. **任务挂起**: 
   - 低压保护触发后，传感器任务会被挂起
   - 需要重启系统才能恢复正常运行
   - 电压监控任务会持续发送低压警告

## 开发工具

- **IDE**: Visual Studio Code + STM32 Extension
- **编译器**: ARM GCC 10.3.1 (或更高版本)
- **调试器**: ST-Link V2 / J-Link
- **生成工具**: STM32CubeMX 6.x
- **构建系统**: CMake + Ninja

## 项目结构

```
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
│   │   └── FreeRTOSConfig.h
│   └── Src/               # 源文件
│       ├── main.c
│       ├── freertos.c     # FreeRTOS 任务实现
│       ├── i2c.c
│       ├── adc.c
│       ├── usart.c
│       ├── gpio.c
│       ├── WF5803F.c
│       └── NTC.c
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
```
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
| 更换 NTC 型号 (B=3950) | `NTC_BETA`, `NTC_R0` | `#define NTC_BETA 3950.0f`<br>`#define NTC_R0 10000.0f` |
| 使用 100kΩ NTC | `NTC_R0`, `NTC_R_SERIES` | `#define NTC_R0 100000.0f`<br>`#define NTC_R_SERIES 100000.0f` |
| 更换串联电阻 | `NTC_R_SERIES` | `#define NTC_R_SERIES 4700.0f` |

**常见 NTC 型号参数：**

| NTC 型号 | R0 (25°C) | Beta 值 | 温度范围 |
|---------|----------|---------|---------|
| MF52-103 | 10kΩ | 3380 | -40°C ~ 125°C |
| MF58-103 | 10kΩ | 3950 | -50°C ~ 150°C |
| NTCLE100E3 | 10kΩ | 3977 | -40°C ~ 125°C |

**温度计算公式 (Steinhart-Hart 简化)：**
```
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

当前代码支持 **7 Bar** 量程传感器，计算公式在 `compute_pressure_WF5803F_7BAR_fromInt()` 中：

```c
float compute_pressure_WF5803F_7BAR_fromInt(int32_t rawData) {
    // 固定公式（7bar型号，输出 kPa）
    return 180.0f/0.81f*(factor-0.1f)+30.0f;
}
```

**如需更换其他量程型号，需修改公式：**

| 传感器型号 | 量程 | 计算公式 | 修改建议 |
|-----------|------|---------|---------|
| WF5803F-7BAR | 0-2 Bar | 当前公式 | 无需修改 |
| WF5803F-2BAR | 0-7 Bar | 查阅数据手册 | 修改计算公式中的系数 |
| WF5803F-10BAR | 0-10 Bar | 查阅数据手册 | 修改计算公式中的系数 |

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



### 7.重要注意事项

⚠️ **修改宏定义后必须做的事：**
1. 清理编译缓存：`CMake: clean rebuild`
2. 重新烧录程序
3. 验证硬件连接是否与软件配置匹配
4. 检查分压电阻是否会导致 ADC 过压

⚠️ **常见错误排查：**
- **I2C 无响应**: 检查上拉电阻、设备地址、供电
- **ADC 读数异常**: 检查参考电压、分压比、接线
- **任务卡死**: 检查栈大小、死锁、优先级配置
- **串口无输出**: 检查波特率、TX/RX 引脚、重定向配置

## 版本历史

### v1.0.0 (2025-10-11)
- ✅ 实现 WF5803F 气压温度传感器驱动
- ✅ 实现 NTC 热敏电阻温度采集
- ✅ 实现电源电压监控与低压保护
- ✅ FreeRTOS 多任务调度
- ✅ UART 串口数据输出
- ✅ 4 路 NMOS 驱动控制接口



*最后更新: 2025年10月11日*
