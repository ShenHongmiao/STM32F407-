/**
  ******************************************************************************
  * @file           : temp_pid_ctrl.h
  * @brief          : Header for temp_pid_ctrl.c file.
  *                   温度PID控制器头文件
  ******************************************************************************
  * @attention
  *
  * 此文件包含温度PID控制相关的配置参数、数据结构和函数声明
  *
  ******************************************************************************
  */

#ifndef __TEMP_PID_CTRL_H
#define __TEMP_PID_CTRL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief PID控制器参数结构体
 */
typedef struct {
    float Kp;           // 比例增益
    float Ki;           // 积分增益
    float Kd;           // 微分增益
    
    float setpoint;     // 目标温度 (°C)
    float integral;     // 积分累积值
    float prev_error;   // 上次误差值
    
    float output;       // PID输出值 (0-100)
    float output_limit_max;  // 输出上限
    float output_limit_min;  // 输出下限
    
    float integral_limit_max;  // 积分限幅最大值
    float integral_limit_min;  // 积分限幅最小值
    
    uint32_t sample_time_ms;   // 采样周期(ms)
} PID_Controller_t;

/* Exported constants --------------------------------------------------------*/

/* 目标温度配置 - 可通过此宏修改控制温度 */
#define TARGET_TEMPERATURE      50.0f   // 目标温度 (°C)
#define TARGET_TEMP_INT         50      // 目标温度整数值 (用于条件编译)

/* PID参数配置 - 根据不同目标温度可能需要调整 */
/* 低温区域 (30-50°C) 推荐参数 */
#define PID_KP_LOW              8.0f    // 比例增益
#define PID_KI_LOW              0.5f    // 积分增益
#define PID_KD_LOW              2.0f    // 微分增益

/* 中温区域 (50-70°C) 推荐参数 */
#define PID_KP_MID              6.0f    // 比例增益
#define PID_KI_MID              0.3f    // 积分增益
#define PID_KD_MID              1.5f    // 微分增益

/* 高温区域 (70-100°C) 推荐参数 */
#define PID_KP_HIGH             4.0f    // 比例增益
#define PID_KI_HIGH             0.2f    // 积分增益
#define PID_KD_HIGH             1.0f    // 微分增益

/* 根据目标温度自动选择PID参数 */
#if (TARGET_TEMP_INT < 50)
    #define PID_KP              PID_KP_LOW
    #define PID_KI              PID_KI_LOW
    #define PID_KD              PID_KD_LOW
#elif (TARGET_TEMP_INT < 70)
    #define PID_KP              PID_KP_MID
    #define PID_KI              PID_KI_MID
    #define PID_KD              PID_KD_MID
#else
    #define PID_KP              PID_KP_HIGH
    #define PID_KI              PID_KI_HIGH
    #define PID_KD              PID_KD_HIGH
#endif

/* PID控制器配置 */
#define PID_SAMPLE_TIME_MS      100     // PID采样周期 (ms)
#define PID_OUTPUT_MAX          100.0f  // PID输出上限 (%)
#define PID_OUTPUT_MIN          0.0f    // PID输出下限 (%)
#define PID_INTEGRAL_MAX        500.0f  // 积分限幅最大值
#define PID_INTEGRAL_MIN        -500.0f // 积分限幅最小值

/* PWM配置 */
#define PWM_FREQUENCY           1000    // PWM频率 (Hz)
#define PWM_PERIOD              1000    // PWM周期 (对应1000Hz)
#define PWM_MIN_DUTY            0       // 最小占空比 (0%)
#define PWM_MAX_DUTY            1000    // 最大占空比 (100%)

/* 温度控制配置 */
#define TEMP_DEADBAND           0.5f    // 温度死区 (°C)，在目标温度±死区内不调整
#define TEMP_EMERGENCY_MAX      80.0f  // 紧急最高温度限制 (°C)
#define TEMP_SAFE_SHUTDOWN      75.0f  // 安全关机温度 (°C)

/* NMOS控制引脚配置 */
#define NMOS1_ON()              HAL_GPIO_WritePin(NMOS1_G_GPIO_Port, NMOS1_G_Pin, GPIO_PIN_RESET)
#define NMOS1_OFF()             HAL_GPIO_WritePin(NMOS1_G_GPIO_Port, NMOS1_G_Pin, GPIO_PIN_SET)
#define NMOS2_ON()              HAL_GPIO_WritePin(NMOS2_G_GPIO_Port, NMOS2_G_Pin, GPIO_PIN_RESET)
#define NMOS2_OFF()             HAL_GPIO_WritePin(NMOS2_G_GPIO_Port, NMOS2_G_Pin, GPIO_PIN_SET)

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief  初始化PID控制器
 * @param  pid: PID控制器结构体指针
 * @retval None
 */
void PID_Init(PID_Controller_t *pid);

/**
 * @brief  PID控制器计算
 * @param  pid: PID控制器结构体指针
 * @param  measured_value: 当前测量的温度值
 * @retval PID输出值 (0-100)
 */
float PID_Compute(PID_Controller_t *pid, float measured_value);

/**
 * @brief  设置PID目标温度
 * @param  pid: PID控制器结构体指针
 * @param  setpoint: 目标温度
 * @retval None
 */
void PID_SetSetpoint(PID_Controller_t *pid, float setpoint);

/**
 * @brief  重置PID控制器
 * @param  pid: PID控制器结构体指针
 * @retval None
 */
void PID_Reset(PID_Controller_t *pid);

/**
 * @brief  设置PID参数
 * @param  pid: PID控制器结构体指针
 * @param  kp: 比例增益
 * @param  ki: 积分增益
 * @param  kd: 微分增益
 * @retval None
 */
void PID_SetTunings(PID_Controller_t *pid, float kp, float ki, float kd);

/**
 * @brief  根据PID输出控制NMOS占空比
 * @param  duty: 占空比 (0-100)
 * @retval None
 */
void TempCtrl_SetDuty(float duty);

/**
 * @brief  温度控制任务 (在FreeRTOS任务中周期调用)
 * @param  current_temp: 当前温度值
 * @retval None
 */
void TempCtrl_Task(float current_temp);

/**
 * @brief  紧急关闭加热
 * @retval None
 */
void TempCtrl_EmergencyStop(void);

/**
 * @brief  初始化温度控制系统
 * @retval None
 */
void TempCtrl_Init(void);

/**
 * @brief  获取当前PID输出值
 * @retval PID输出值 (0-100)
 */
float TempCtrl_GetOutput(void);

/**
 * @brief  获取PID控制器指针
 * @retval PID控制器结构体指针
 */
PID_Controller_t* TempCtrl_GetPID(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEMP_PID_CTRL_H */
