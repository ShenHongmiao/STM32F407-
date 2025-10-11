#include "WF5803F.h"

//需要对传感器寄存器0x30写入000b开始转换单次温度转换，001b开始单次气压转换
//通过读取0x02寄存器的bit0(DRDY)值来判断是否转换完成,1表示完成
//转换完成后读取温度和气压数据，原始温度数据Temp_out为16位，分为MSB和LSB。地址为0x09~0x0A
//原始气压数据Press_out为24位，分为MSB、CSB和LSB。地址为0x06~0x08
/*气压+温度读取*/
void WF5803F_ReadDate(int16_t* Temp, int32_t* Press)
{
    uint8_t cmd = 0x0A;                           // 单次气压+温度转换
    uint8_t status;
    uint8_t buf[5];

    // 启动一次转换
    HAL_I2C_Mem_Write(&hi2c1, WF5803F_ADDR, WF5803F_REG_CTRL,
                      I2C_MEMADD_SIZE_8BIT, &cmd, 1, 100);

    // 简单等待 + 轮询 DRDY bit0
    osDelay(5);
    do {
        HAL_I2C_Mem_Read(&hi2c1, WF5803F_ADDR, WF5803F_REG_STATUS,
                          I2C_MEMADD_SIZE_8BIT, &status, 1, 100);
        osDelay(5);
    } while ((status & 0x01) == 0);

    // 一次性读出 5 字节（3B 压力 + 2B 温度）
    HAL_I2C_Mem_Read(&hi2c1, WF5803F_ADDR, WF5803F_REG_PRESS_MSB,
                      I2C_MEMADD_SIZE_8BIT, buf, 5, 100);

    *Press = (int32_t)((uint32_t)buf[0] << 16 | (uint32_t)buf[1] << 8 | buf[2]); // 24bit
    *Temp  = (int16_t)((uint16_t)buf[3] << 8 | buf[4]);                           // 16bit
}

/**
 * @brief  将 32位有符号整数(只用低24位) 转换为压力
 * @param  rawData  32位整型数，存放传感器的24位原始数据
 * @return 压力值，单位 kPa
 */
float compute_pressure_WF5803F_2BAR_fromInt(int32_t rawData) {
    // 保留 24位数据
    rawData &= 0x00FFFFFF;

    // 如果最高位 (bit23) 为1，则需要做符号扩展
    if (rawData & 0x00800000) {
        rawData |= 0xFF000000; // 扩展成32位有符号数
    }

    // 归一化到 [-1, +1)
    float factor = (float)rawData / 8388608.0f;  // 除以 2^23

    // 固定公式（2bar型号，输出 kPa）
    return 180.0f/0.81f*(factor-0.1f)+30.0f;
}

float compute_temperature_WF5803F_fromInt(int16_t rawData) {
    // 如果最高位 (bit15) 为1，则需要做符号扩展
    if (rawData & 0x00008000) {   // 如果bit15是1，表示负数
        rawData |= 0xFFFF0000;    // 高16位填充1
    }

    return (float)rawData / 256.0f; // 每个单位 = 1/256 ℃
}

/**
 * @brief  直接获取计算后的温度和气压值
 * @param  temperature  指向float变量的指针，用于存储计算后的温度值(单位: ℃)
 * @param  pressure     指向float变量的指针，用于存储计算后的气压值(单位: kPa)
 */
void WF5803F_GetData(float* temperature, float* pressure)
{
    int16_t rawTemp;
    int32_t rawPress;
    
    // 读取原始数据
    WF5803F_ReadDate(&rawTemp, &rawPress);
    
    // 转换为实际物理值
    *temperature = compute_temperature_WF5803F_fromInt(rawTemp);
    *pressure = compute_pressure_WF5803F_2BAR_fromInt(rawPress);
}


