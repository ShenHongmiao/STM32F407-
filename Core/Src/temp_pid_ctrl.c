/**
  ******************************************************************************
  * @file           : temp_pid_ctrl.c
  * @brief          : Temperature PID Control Implementation
  *                   温度PID控制器实现
  ******************************************************************************
  * @attention
  *
  * 此文件实现了基于PID算法的温度控制功能
  * 
  * 硬件配置:
  * - PC6: TIM3_CH1 (加热控制1) - 硬件PWM输出
  * - PC7: TIM3_CH2 (加热控制2) - 硬件PWM输出
  * 
  * 控制策略:
  * - 使用TIM3硬件PWM控制NMOS占空比 (周期1000ms)
  * - PID算法计算占空比 (0-1000ms)
  * - 温度过高时紧急关断加热
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "temp_pid_ctrl.h"
#include "cmsis_os.h"
#include "usart.h"
#include <math.h>
#include <stdio.h>
#include "stm32f4xx_hal_tim.h"

extern TIM_HandleTypeDef htim3; // TIM3句柄

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static PID_Controller_t g_pid;                    // 全局PID控制器

/* Private function prototypes -----------------------------------------------*/
static float Clamp(float value, float min, float max);

/* Function implementations --------------------------------------------------*/

/**
 * @brief  设置加热MOS管硬件PWM占空比（0-10000）
 * @param  duty_ms: 0-1000ms（实际PWM周期为1000ms）
 */
void Set_Heating_PWM(uint16_t duty_ms)
{
    if (duty_ms > 1000) duty_ms = 1000;
    if(duty_ms < 0) duty_ms = 0;
    uint32_t pulse = duty_ms * 10; // 1000ms对应10000计数
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse); // PC6
    // __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse); // PC7
}

/**
 * @brief  初始化PID控制器
 * @param  pid: PID控制器结构体指针
 * @retval None
 */
void PID_Init(PID_Controller_t *pid)
{
    if (pid == NULL) return;
    
    // 设置PID参数
    pid->Kp = PID_KP;
    pid->Ki = PID_KI;
    pid->Kd = PID_KD;
    
    // 设置目标温度
    pid->setpoint = TARGET_TEMPERATURE;
    
    // 初始化内部状态
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output = 0.0f;
    
    // 设置输出限幅
    pid->output_limit_max = PID_OUTPUT_MAX;
    pid->output_limit_min = PID_OUTPUT_MIN;
    
    // 设置积分限幅
    pid->integral_limit_max = PID_INTEGRAL_MAX;
    pid->integral_limit_min = PID_INTEGRAL_MIN;
    
    // 设置采样时间
    pid->sample_time_ms = PID_SAMPLE_TIME_MS;
}

/**
 * @brief  PID控制器计算
 * @param  pid: PID控制器结构体指针
 * @param  measured_value: 当前测量的温度值
 * @retval PID输出值 (0-1000ms)
 */
float PID_Compute(PID_Controller_t *pid, float measured_value)
{
    if (pid == NULL) return 0.0f;
    
    // 计算误差
    float error = pid->setpoint - measured_value;
    
    // 死区控制 - 在目标温度附近小幅波动时不调整
    if (fabsf(error) < TEMP_DEADBAND) {
        // 保持当前输出，不累积积分
        return pid->output;
    }
    
    // 计算积分项 (转换采样时间到秒)
    float dt = pid->sample_time_ms / 1000.0f;
    pid->integral += error * dt;
    
    // 积分限幅，防止积分饱和
    pid->integral = Clamp(pid->integral, pid->integral_limit_min, pid->integral_limit_max);
    
    // 计算微分项
    float derivative = (error - pid->prev_error) / dt;
    
    // PID输出计算
    pid->output = pid->Kp * error + 
                  pid->Ki * pid->integral + 
                  pid->Kd * derivative;
    
    // 输出限幅 (0-1000ms)
    pid->output = Clamp(pid->output, pid->output_limit_min, pid->output_limit_max);
    
    // 保存当前误差供下次使用
    pid->prev_error = error;
    
    return pid->output;
}

/**
 * @brief  设置PID目标温度
 * @param  pid: PID控制器结构体指针
 * @param  setpoint: 目标温度
 * @retval None
 */
void PID_SetSetpoint(PID_Controller_t *pid, float setpoint)
{
    if (pid == NULL) return;
    pid->setpoint = setpoint;
}

/**
 * @brief  重置PID控制器
 * @param  pid: PID控制器结构体指针
 * @retval None
 */
void PID_Reset(PID_Controller_t *pid)
{
    if (pid == NULL) return;
    
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output = 0.0f;
}

/**
 * @brief  限幅函数
 * @param  value: 输入值
 * @param  min: 最小值
 * @param  max: 最大值
 * @retval 限幅后的值
 */
static float Clamp(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * @brief  紧急关闭加热
 * @retval None
 */
void TempCtrl_EmergencyStop(void)
{
    // 关闭硬件PWM输出
    Set_Heating_PWM(0);
    
    // 重置PID控制器
    PID_Reset(&g_pid);
    
    send_message("[TEMP_CTRL] Emergency stop activated!\n");
}

/**
 * @brief  初始化温度控制系统
 * @retval None
 */
void TempCtrl_Init(void)
{
    // 初始化PID控制器
    PID_Init(&g_pid);
    
    // 初始化硬件PWM为关断状态
    Set_Heating_PWM(0);
    
    send_message("[TEMP_CTRL] Temperature Control Initialized\n");
    send_message("[TEMP_CTRL] Target Temperature: %.2f°C\n", TARGET_TEMPERATURE);
    send_message("[TEMP_CTRL] PID Parameters: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", 
           g_pid.Kp, g_pid.Ki, g_pid.Kd);
    send_message("[TEMP_CTRL] Hardware PWM Mode (TIM3), Period: %dms\n", PWM_PERIOD_MS);
    
    // 打印温度区间信息
    #if (TARGET_TEMP_INT < 50)
    send_message("[TEMP_CTRL] Temperature Range: LOW (30-50°C)\n");
    #elif (TARGET_TEMP_INT < 70)
    send_message("[TEMP_CTRL] Temperature Range: MID (50-70°C)\n");
    #else
    send_message("[TEMP_CTRL] Temperature Range: HIGH (70-100°C)\n");
    #endif
}

/**
 * @brief  获取PID控制器指针（用于外部调整PID参数）
 * @retval PID控制器结构体指针
 */
PID_Controller_t* TempCtrl_GetPID(void)
{
    return &g_pid;
}
