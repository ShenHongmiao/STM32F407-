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
  * - PC6: NMOS1_G (加热控制1) - 低电平导通，高电平关断
  * - PC7: NMOS2_G (加热控制2) - 低电平导通，高电平关断
  * 
  * 控制策略:
  * - 使用软件PWM控制NMOS占空比
  * - PID算法计算占空比 (0-100%)
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

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static PID_Controller_t g_pid;           // 全局PID控制器
static volatile float g_current_duty = 0.0f;  // 当前占空比
static volatile uint8_t g_heating_enabled = 1;  // 加热使能标志

/* Private function prototypes -----------------------------------------------*/
static void PWM_SetDuty(float duty);
static float Clamp(float value, float min, float max);

/* Function implementations --------------------------------------------------*/

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
 * @retval PID输出值 (0-100)
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
    
    // 输出限幅
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
 * @brief  设置PID参数
 * @param  pid: PID控制器结构体指针
 * @param  kp: 比例增益
 * @param  ki: 积分增益
 * @param  kd: 微分增益
 * @retval None
 */
void PID_SetTunings(PID_Controller_t *pid, float kp, float ki, float kd)
{
    if (pid == NULL) return;
    
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
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
 * @brief  软件PWM设置占空比
 * @param  duty: 占空比 (0-100)
 * @retval None
 * @note   此函数控制NMOS1和NMOS2，使用相同的占空比
 *         可根据需要修改为独立控制两路NMOS
 */
static void PWM_SetDuty(float duty)
{
    // 限制占空比范围
    duty = Clamp(duty, 0.0f, 100.0f);
    g_current_duty = duty;
    
    // 注意: NMOS低电平导通，高电平关断
    // duty = 0%  表示完全关断 (GPIO输出高电平)
    // duty = 100% 表示完全导通 (GPIO输出低电平)
    
    // 这里使用简单的软件PWM实现
    // 实际应用中建议使用硬件定时器PWM以获得更精确的控制
    
    // 如果占空比为0，直接关断
    if (duty < 0.1f) {
        NMOS1_OFF();
        NMOS2_OFF();
        return;
    }
    
    // 如果占空比为100%，直接导通
    if (duty > 99.9f) {
        NMOS1_ON();
        NMOS2_ON();
        return;
    }
    
    // 中间值使用软件PWM (需要在任务中周期调用)
    // 这里仅设置占空比值，实际PWM逻辑在TempCtrl_Task中实现
}

/**
 * @brief  根据PID输出控制NMOS占空比
 * @param  duty: 占空比 (0-100)
 * @retval None
 */
void TempCtrl_SetDuty(float duty)
{
    PWM_SetDuty(duty);
}

/**
 * @brief  温度控制任务 (在FreeRTOS任务中周期调用)
 * @param  current_temp: 当前温度值
 * @retval None
 */
void TempCtrl_Task(float current_temp)
{
    static uint32_t pwm_counter = 0;
    
    // 检查是否使能加热
    if (!g_heating_enabled) {
        NMOS1_OFF();
        NMOS2_OFF();
        return;
    }
    
    // 紧急温度保护
    if (current_temp >= TEMP_EMERGENCY_MAX) {
        send_message("[TEMP_CTRL] EMERGENCY! Temperature %.2f°C >= %.2f°C\n", 
               current_temp, TEMP_EMERGENCY_MAX);
        TempCtrl_EmergencyStop();
        return;
    }
    
    // 安全温度保护
    if (current_temp >= TEMP_SAFE_SHUTDOWN) {
        send_message("[TEMP_CTRL] WARNING! Temperature %.2f°C >= %.2f°C, reducing power\n", 
               current_temp, TEMP_SAFE_SHUTDOWN);
        // 强制降低输出到30%
        PWM_SetDuty(30.0f);
    } else {
        // PID控制
        float pid_output = PID_Compute(&g_pid, current_temp);
        PWM_SetDuty(pid_output);
    }
    
    // 软件PWM实现 (基于100个计数周期)
    // PWM频率 = 1000Hz / 100 = 10Hz
    pwm_counter++;
    if (pwm_counter >= 100) {
        pwm_counter = 0;
    }
    
    // 根据占空比控制NMOS
    uint32_t on_time = (uint32_t)(g_current_duty);  // 0-100
    
    if (pwm_counter < on_time) {
        // PWM高电平期间 - NMOS导通
        NMOS1_ON();
        NMOS2_ON();
    } else {
        // PWM低电平期间 - NMOS关断
        NMOS1_OFF();
        NMOS2_OFF();
    }
}

/**
 * @brief  紧急关闭加热
 * @retval None
 */
void TempCtrl_EmergencyStop(void)
{
    g_heating_enabled = 0;
    NMOS1_OFF();
    NMOS2_OFF();
    PID_Reset(&g_pid);
    g_current_duty = 0.0f;
    
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
    
    // 初始化NMOS为关断状态
    NMOS1_OFF();
    NMOS2_OFF();
    
    // 使能加热
    g_heating_enabled = 1;
    g_current_duty = 0.0f;
    
    send_message("[TEMP_CTRL] Temperature Control Initialized\n");
    send_message("[TEMP_CTRL] Target Temperature: %.2f°C\n", TARGET_TEMPERATURE);
    send_message("[TEMP_CTRL] PID Parameters: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", 
           g_pid.Kp, g_pid.Ki, g_pid.Kd);
    
    // 打印温度区间信息
    #if (TARGET_TEMPERATURE < 50.0f)
    send_message("[TEMP_CTRL] Temperature Range: LOW (30-50°C)\n");
    #elif (TARGET_TEMPERATURE < 70.0f)
    send_message("[TEMP_CTRL] Temperature Range: MID (50-70°C)\n");
    #else
    send_message("[TEMP_CTRL] Temperature Range: HIGH (70-100°C)\n");
    #endif
}

/**
 * @brief  获取当前PID输出值
 * @retval PID输出值 (0-100)
 */
float TempCtrl_GetOutput(void)
{
    return g_pid.output;
}

/**
 * @brief  获取PID控制器指针
 * @retval PID控制器结构体指针
 */
PID_Controller_t* TempCtrl_GetPID(void)
{
    return &g_pid;
}
