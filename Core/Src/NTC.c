#include "NTC.h"

/**
 * @brief  根据 ADC 数值计算 NTC 温度
 * @param  adcValue  ADC 采样值 (0 ~ 4095 for 12-bit ADC)
 * @return 温度值，单位 摄氏度 (°C)
 */
float compute_ntc_temperature(uint32_t adcValue)
{
    // 1. ADC 转电压
    float vout = (adcValue / ADC_MAX_VALUE) * V_REF;

    // 2. 由分压公式求 NTC 阻值
    // R_ntc = R_series * Vout / (Vref - Vout)
    float r_ntc = (NTC_R_SERIES * vout) / (V_REF - vout);

    // 3. Beta 公式换算温度 (K)
    float tempK = 1.0f / ( (1.0f/NTC_T0) + (1.0f/NTC_BETA) * logf(r_ntc / NTC_R0) );

    // 4. 转换为摄氏度
    float tempC = tempK - 273.15f;

    return tempC;
}

/**
 * @brief  读取 ADC1 的通道0 (PA0) 的值
 * @return ADC 采样值 (0 ~ 4095 for 12-bit ADC)
 */
uint32_t Read_ADC0(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint32_t adcValue = 0;
    
    // 配置 ADC 通道0 (PA0 - NTC)
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) == HAL_OK)
    {
        // 启动 ADC 转换
        HAL_ADC_Start(&hadc1);
        
        // 等待转换完成
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
        {
            adcValue = HAL_ADC_GetValue(&hadc1);
        }
        
        // 停止 ADC
        HAL_ADC_Stop(&hadc1);
    }
    
    return adcValue;
}