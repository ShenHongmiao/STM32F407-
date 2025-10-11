#ifndef __V_DETECT_H
#define __V_DETECT_H

#include "main.h"
#include "adc.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

// 电压检测参数
#define VOLTAGE_R1           51000.0f    // R21 = 51kΩ
#define VOLTAGE_R2           5100.0f     // R22 = 5.1kΩ
#define VOLTAGE_RATIO        (VOLTAGE_R2 / (VOLTAGE_R1 + VOLTAGE_R2))  // 0.0909
#define VOLTAGE_NORMAL       24.0f       // 标准电压 24V
#define VOLTAGE_THRESHOLD    (VOLTAGE_NORMAL * 0.7f)  // 低压阈值 16.8V (70%)
#define ADC_MAX_VALUE        4095.0f     // 12-bit ADC
#define ADC_VREF             3.3f        // ADC 参考电压

// 函数声明
uint32_t Read_VoltageADC(void);
float Calculate_SourceVoltage(uint32_t adcValue);
void Send_VoltageWarning(float voltage, const char* message);
uint8_t Check_Voltage(float* pVoltage);
void Delay_Blocking_ms(uint32_t ms);

#endif /* __V_DETECT_H */
