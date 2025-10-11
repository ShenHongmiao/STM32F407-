#include "i2c.h"
#include "main.h"
#include "freertos.h"
#include "math.h"

// WF5803F 寄存器地址定义
#define WF5803F_ADDR          (0x6C<<1)   // I2C 设备地址 (7位地址左移)
#define WF5803F_REG_CTRL      0x30        // 控制寄存器
#define WF5803F_REG_STATUS    0x02        // 状态寄存器
#define WF5803F_REG_PRESS_MSB 0x06        // 气压数据 MSB
#define WF5803F_REG_TEMP_MSB  0x09        // 温度数据 MSB

// 函数声明
// void WF5803F_ReadDate_Temp(int16_t* Temp);
// void WF5803F_ReadDate_Press(int32_t* Press);
void WF5803F_ReadDate(int16_t* Temp, int32_t* Press);
float compute_pressure_WF5803F_2BAR_fromInt(int32_t rawData);
float compute_temperature_WF5803F_fromInt(int16_t rawData);
void WF5803F_GetData(float* temperature, float* pressure);