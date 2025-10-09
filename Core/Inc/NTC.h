#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "math.h"

// NTC 热敏电阻参数
#define NTC_BETA 3380.0f      // NTC 热敏电阻的 Beta 常数 (B值 25°C/50°C)
#define NTC_R0 10000.0f       // NTC 热敏电阻在 T0 温度下的阻值 (10k Ohm @ 25°C)
#define NTC_T0 298.15f        // 参考温度 T0 (25°C = 298.15K)
#define NTC_R_SERIES 10000.0f // 串联电阻的阻值 (10k Ohm)
#define ADC_MAX_VALUE 4095.0f // 12-bit ADC 最大值
#define V_REF 3.3f            // 参考电压 (3.3V)

// 硬件连接: ADC1_IN0 (PA0引脚)
// 电路: VCC(3.3V) -- R_SERIES(10k) -- PA0 -- NTC(10k@25°C) -- GND

// 函数声明
float compute_ntc_temperature(uint32_t adcValue);
uint32_t Read_ADC0(void);

