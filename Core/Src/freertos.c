/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "usart.h"
#include "i2c.h"
#include "adc.h"
#include <string.h>
#include <stdio.h>

/* USER CODE BEGIN Includes */
#include "temp_pid_ctrl.h"
#include "WF5803F.h"
#include "NTC.h"
#include "V_detect.h"
/* USER CODE END Includes */

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VOLTAGE_CHECK_INTERVAL  600000  // 电压检测间隔: 10分钟 (600000 ms)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile uint8_t g_lowVoltageFlag = 0;  // 低电压标志: 0=正常, 1=低压
extern PID_Controller_t temp_pid_CN1; // CN1通道PID控制器
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId Sensors_and_computeHandle;
osThreadId voltageMonitorHandle;
osThreadId receiveAndTargetChangeHandle;
osMessageQId usart_rx_queueHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartSensors_and_compute(void const * argument);
void StartVoltageMonitorTask(void const * argument);
void StartReceiveAndTargetChangeTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* definition and creation of usart_rx_queue */
  osMessageQDef(usart_rx_queue, 16, uint32_t);
  usart_rx_queueHandle = osMessageCreate(osMessageQ(usart_rx_queue), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  

  /* definition and creation of defaultTask - 上电检测任务，最高优先级 */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityRealtime, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of receiveAndTargetChange - USART接收任务，高优先级 */
  osThreadDef(receiveAndTargetChange, StartReceiveAndTargetChangeTask, osPriorityHigh, 0, 256);
  receiveAndTargetChangeHandle = osThreadCreate(osThread(receiveAndTargetChange), NULL);
  
  /* definition and creation of Sensors_and_compute - 传感器与计算任务 */
  osThreadDef(Sensors_and_compute, StartSensors_and_compute, osPriorityNormal, 0, 512);
  Sensors_and_computeHandle = osThreadCreate(osThread(Sensors_and_compute), NULL);
  
  /* definition and creation of voltageMonitorTask - 最低优先级 */
  osThreadDef(voltageMonitor, StartVoltageMonitorTask, osPriorityLow, 0, 256);
  voltageMonitorHandle = osThreadCreate(osThread(voltageMonitor), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  * 
  * 功能：上电初始化任务（最高优先级）
  * - 执行上电电压检测
  * - 检测完成后删除自己，释放资源
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  // /* USER CODE BEGIN StartDefaultTask */
  osDelay(10000);
  // 删除自己，释放资源
  vTaskDelete(NULL);

  // /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
  * @brief  Function implementing the Sensors_and_compute thread.
  * @param  argument: Not used
  * @retval None
  * 
  * 功能：传感器读取与计算任务，包含：
  * - WF5803F 温度和气压检测
  * - NTC 温度检测
  * - 后续可添加其他传感器和计算逻辑
  */
void StartSensors_and_compute(void const * argument)
{
  float temperature;
  float pressure;
  float Temp_NTC;
  uint32_t adcValue;
  HAL_StatusTypeDef status;

  send_message("=== Sensors_and_compute Task Started! ===\n");
  
  /* Infinite loop */
  for(;;)
  {
    // 检查低压标志，如果电压过低则挂起任务
    if (g_lowVoltageFlag) {
      send_message("Sensors_and_compute task suspended due to low voltage!\n");
      vTaskSuspend(NULL);  // 挂起自己
    }
    
    // ========== WF5803F 温度和气压检测 ==========
    // 测试 I2C 通信
    uint8_t test_cmd = 0x0A;
    status = HAL_I2C_Mem_Write(&hi2c1, WF5803F_ADDR, WF5803F_REG_CTRL, I2C_MEMADD_SIZE_8BIT, &test_cmd, 1, 100);
    
    if (status == HAL_OK) {
      // send_message("I2C Write OK\n");
    } else {
      // send_message("I2C Write Failed: %d\n", status);
    }
    
    // 获取温度和气压数据
    WF5803F_GetData(&temperature, &pressure);
    
    
    
    // ========== NTC 温度检测 ==========
    // 读取 ADC 值
    adcValue = Read_ADC0();
    
    // 计算温度
    Temp_NTC = compute_ntc_temperature(adcValue);


    //调试使用，自动切换目标温度
    // if (Temp_NTC >30.0f){
    //   temp_pid_CN1.Kp = 0.0f;
    //   temp_pid_CN1.Ki = 0.0f;
    //   temp_pid_CN1.Kd = 0.0f;
    //   PID_SetSetpoint(&temp_pid_CN1, TARGET_TEMP_1);
    // }
    // else if (Temp_NTC < 26.5f){
    //   temp_pid_CN1.Kp = 140.0f;
    //   temp_pid_CN1.Ki = 0.0f;
    //   temp_pid_CN1.Kd = 0.0f;
    //   PID_SetSetpoint(&temp_pid_CN1, TARGET_TEMP_2);
    
    // }

    // ========== 后续可在此处添加其他传感器读取和计算逻辑 ==========

    PID_Compute(&temp_pid_CN1, Temp_NTC);
    Set_Heating_PWM((uint32_t)temp_pid_CN1.output);
    
    // 通过串口发送传感器数据 (JSON格式，分三条发送便于串口监控)
    send_message("{\"type\":\"data\",\"sensor\":\"WF5803\",\"temp\":%.2f,\"press\":%.2f}\n", temperature, pressure);
    send_message("{\"type\":\"data\",\"sensor\":\"NTC\",\"temp\":%.2f}\n", Temp_NTC);
    send_message("{\"type\":\"data\",\"sensor\":\"PID\",\"output\":%.2f}\n", temp_pid_CN1.output);
    // 延时
    osDelay(PID_SAMPLE_TIME_MS);
  }
}

/**
  * @brief  Function implementing the voltageMonitorTask thread.
  * @param  argument: Not used
  * @retval None
  * 
  * 功能：周期性监控电源电压，当电压低于 70% 时挂起其他任务并持续发送低压警告
  * 优先级：最低 (osPriorityLow)
  * 检测频率：每 10 分钟检测一次
  * 注意：上电检测已由 defaultTask 完成，此任务只负责周期检测
  */
void StartVoltageMonitorTask(void const * argument)
{
  float voltage;
  uint8_t voltageStatus;
  
  send_message("=== Voltage Monitor Task Started (Priority: Low) ===\n");
  send_message("Check interval: %d ms (%.1f minutes)\n", VOLTAGE_CHECK_INTERVAL, VOLTAGE_CHECK_INTERVAL/60000.0f);
  send_message("========================================\n\n\n");
  
  // ========== 进入周期检测循环 ==========
  for(;;)
  {
    // 延时 10 分钟
    osDelay(VOLTAGE_CHECK_INTERVAL);
    
    // 定期检测电压
    send_message("\n[PERIODIC] Voltage check...\n");
    voltageStatus = Check_Voltage(&voltage);
    
    send_message("Voltage: %.2fV\n", voltage);
    
    if (!voltageStatus) {
      // 电压过低
      if (g_lowVoltageFlag == 0) {
        // 检测到低压，挂起其他任务
        g_lowVoltageFlag = 1;
        // TempCtrl_EmergencyStop(&temp_pid_CN1); // 紧急关闭加热
        send_message("!!! LOW VOLTAGE ALERT !!!\n");
        send_message("Suspending All tasks...\n");
        
        // 挂起传感器计算任务
        if (Sensors_and_computeHandle != NULL) {
          vTaskSuspend(Sensors_and_computeHandle);
        }
      }
      
      // 持续发送低压警告（无论是否首次）
      Send_VoltageWarning(voltage, "LOW");
      
    } else {
      // 电压正常，发送正常消息
      Send_VoltageWarning(voltage, "OK");
      
      // 注意：即使电压恢复，也不再恢复任务
      // 任务保持挂起状态，直到系统重启
    }
  }
}



/**
  * @brief  Function implementing the receiveAndTargetChange thread.
  * @param  argument: Not used
  * @retval None
  * 
  * 功能：USART 接收与目标处理任务（最高优先级）
  * - 阻塞等待从 USART 接收队列读取数据
  * - 收到数据后立即处理
  * - 可用于接收命令、改变目标值等操作
  * 
  * 优先级：最高 (osPriorityRealtime)
  * 特点：阻塞式读取，死等数据到来
  */
void StartReceiveAndTargetChangeTask(void const * argument)
{
  osEvent event;
  uint8_t received_byte;
  
  send_message("=== USART Receive Task Started (Priority: Realtime) ===\n");
 
  /* Infinite loop */
  for(;;)
  {
    // 阻塞读取队列，永久等待直到有数据到来
    event = osMessageGet(usart_rx_queueHandle, osWaitForever);
    
    if (event.status == osEventMessage) {
      // 成功接收到数据
      received_byte = (uint8_t)event.value.v;
      
      // 发送接收到的数据信息
      send_message("Received byte from USART2: '%c' (0x%02X)\n", 
                   received_byte, received_byte);
      // ========== 在此处添加命令处理逻辑 ==========
      if(received_byte  == '1'){
        if(temp_pid_CN1.setpoint != TARGET_TEMP_1){
          PID_SetSetpoint(&temp_pid_CN1, TARGET_TEMP_1);
          send_message("Target temperature set to %.2f°C\n", TARGET_TEMP_1);
        } else{
          PID_SetSetpoint(&temp_pid_CN1, TARGET_TEMP_2);
          send_message("Target temperature already at %.2f°C\n", TARGET_TEMP_2);
        }
      } else if(received_byte  == '2'){
        
          PID_SetSetpoint(&temp_pid_CN1, 40.0f);
          send_message("Target temperature set to %.2f°C\n", 40.0f);
        }

      }
    
    // 注意：不需要 osDelay，因为 osMessageGet 本身就是阻塞的
    // 当没有数据时，任务会自动进入阻塞状态，让出 CPU
  }
}

/* USER CODE END Application */