#include "V_detect.h"

/**
 * @brief  读取 ADC1 通道14 (PC4) 的电压
 * @return ADC 采样值 (0-4095)
 */
uint32_t Read_VoltageADC(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint32_t adcValue = 0;
    
    // 配置 ADC 通道14 (PC4 - V_DETECT)
    sConfig.Channel = ADC_CHANNEL_14;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) == HAL_OK)
    {
        // 启动 ADC 转换
        HAL_ADC_Start(&hadc1);
        
        // 等待转换完成 (超时 100ms)
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
        {
            adcValue = HAL_ADC_GetValue(&hadc1);
        }
        
        // 停止 ADC
        HAL_ADC_Stop(&hadc1);
    }
    
    return adcValue;
}

/**
 * @brief  根据 ADC 值计算实际电源电压
 * @param  adcValue  ADC 采样值 (0-4095)
 * @return 电源电压值 (V)
 * 
 * 电路说明:
 * VCC(24V) -- R1(51kΩ) -- PC4(ADC) -- R2(5.1kΩ) -- GND
 * 
 * 分压公式:
 * VADC = VCC × (R2 / (R1 + R2)) = VCC × 0.0909
 * 反推: VCC = VADC / 0.0909
 */
float Calculate_SourceVoltage(uint32_t adcValue)
{
    // 1. ADC 值转换为电压
    float vAdc = ((float)adcValue / ADC_MAX_VALUE) * ADC_VREF;
    
    // 2. 根据分压公式反推电源电压
    // VADC = VCC × (R2 / (R1 + R2))
    // VCC = VADC / VOLTAGE_RATIO
    float vSource = vAdc / VOLTAGE_RATIO;
    
    return vSource;
}

/**
 * @brief  通过串口发送电压信息给上位机
 * @param  voltage   当前电压值
 * @param  isNormal  电压是否正常: 1=正常, 0=不足
 */
void Send_VoltageWarning(float voltage, const char* message)
{
    char uartMsg[80];
    
    // 根据 message 判断电压状态
    // message == "OK" 表示正常, 其他表示低压
    if (strcmp(message, "OK") == 0) {
        // 电压正常
        snprintf(uartMsg, sizeof(uartMsg), 
                 "Power OK,voltage: %.2fV.\r\n", voltage);
    } else {
        // 电压不足
        snprintf(uartMsg, sizeof(uartMsg), 
                 "Power Low,voltage: %.2fV,please charge.\r\n", voltage);
    }

    // 通过 UART2 发送给上位机
    HAL_UART_Transmit(&huart2, (uint8_t*)uartMsg, strlen(uartMsg), 1000);
}

/**
 * @brief  检测电压是否正常
 * @param  pVoltage  指向 float 的指针，用于返回检测到的电压值
 * @return 1 = 电压正常, 0 = 电压过低
 */
uint8_t Check_Voltage(float* pVoltage)
{
    uint32_t adcValue;
    float voltage;
    
    // 读取 ADC
    adcValue = Read_VoltageADC();
    
    // 计算电压
    voltage = Calculate_SourceVoltage(adcValue);
    
    // 返回电压值
    if (pVoltage != NULL) {
        *pVoltage = voltage;
    }
    
    // 判断电压是否正常
    if (voltage < VOLTAGE_THRESHOLD) {
        return 0;  // 电压过低
    } else {
        return 1;  // 电压正常
    }
}

/**
 * @brief  阻塞延迟函数（不依赖 FreeRTOS）
 * @param  ms  延迟时间（毫秒）
 * 
 * 注意：此函数使用空循环实现延迟，不会释放 CPU
 * 仅适用于 FreeRTOS 调度器启动前的初始化阶段
 * 假设系统时钟为 72MHz（根据实际系统时钟调整）
 */
void Delay_Blocking_ms(uint32_t ms)
{
    // 获取系统时钟频率
    uint32_t sysClock = HAL_RCC_GetSysClockFreq();
    
    // 计算每毫秒需要的循环次数
    // 每个循环约 4 条指令，每条指令 1 个时钟周期
    uint32_t loopsPerMs = sysClock / 4000;
    
    // 延迟循环
    for (uint32_t i = 0; i < ms; i++) {
        for (volatile uint32_t j = 0; j < loopsPerMs; j++) {
            __NOP();  // 空操作，防止编译器优化
        }
    }
}
