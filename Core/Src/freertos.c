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
#include "WF5803F.h"
#include "NTC.h"
#include "V_detect.h"
#include "usart.h"
#include "i2c.h"
#include "adc.h"
#include <string.h>
#include <stdio.h>

/* USER CODE BEGIN Includes */
// 使用send_message替代printf，通过串口1发送调试信息
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
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId sensorTaskHandle;
osThreadId ntcTaskHandle;
osThreadId voltageMonitorHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartSensorTask(void const * argument);
void StartNTCTask(void const * argument);
void StartVoltageMonitorTask(void const * argument);

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
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  
  /* definition and creation of defaultTask - 最高优先级，用于上电检测 */
  /* 注意：使用 osPriorityRealtime 确保最先执行 */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityRealtime, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);
  
  /* definition and creation of sensorTask - 延时启动 */
  osThreadDef(sensorTask, StartSensorTask, osPriorityNormal, 0, 512);
  sensorTaskHandle = osThreadCreate(osThread(sensorTask), NULL);
  
  /* definition and creation of ntcTask - 延时启动 */
  osThreadDef(ntcTask, StartNTCTask, osPriorityNormal, 0, 256);
  ntcTaskHandle = osThreadCreate(osThread(ntcTask), NULL);
  
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
  /* USER CODE BEGIN StartDefaultTask */
  float voltage;
  uint8_t voltageStatus;
  
  // send_message("\n========================================\n");
  // send_message("System Power-On Initialization\n");
  // send_message("========================================\n");
  
  // 延时等待系统稳定，使用阻塞式延时确保其他任务未启动
  Delay_Blocking_ms(500);
  // ========== 上电电压检测 ==========
  // send_message("\n[BOOT] Power-on voltage check...\n");
  voltageStatus = Check_Voltage(&voltage);
  
  // send_message("Voltage: %.2fV, Status: %s\n", voltage, voltageStatus ? "OK" : "LOW");
  // send_message("Threshold: %.2fV (70%% of %.1fV)\n", VOLTAGE_THRESHOLD, VOLTAGE_NORMAL);
  
  if (!voltageStatus) {
    // 电压过低，设置标志并挂起其他任务
    // g_lowVoltageFlag = 1;
    
    // 发送低压警告
    Send_VoltageWarning(voltage, "LOW");
    // send_message("\n!!! LOW VOLTAGE ALERT !!!\n");
    // send_message("System will run in low-power mode.\n");
    // send_message("Sensor and NTC tasks will be suspended.\n");
    
    // 挂起其他任务（稍后创建时会被挂起）
    // 注意：此时其他任务可能还未创建，所以在各任务启动时检查标志
    
  } else {
    // 电压正常
    Send_VoltageWarning(voltage, "OK");
    // send_message("Voltage OK. All tasks will run normally.\n");
  }
  
  // send_message("\n========================================\n");
  // send_message("Initialization complete.\n");
  // send_message("Deleting default task...\n");
  // send_message("========================================\n\n");
  
  // 延时一下，确保消息打印完成
  Delay_Blocking_ms(100);
  
  // 删除自己，释放资源
  vTaskDelete(NULL);
  
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
  * @brief  Function implementing the sensorTask thread.
  * @param  argument: Not used
  * @retval None
  */
void StartSensorTask(void const * argument)
{
  float temperature;
  float pressure;
  HAL_StatusTypeDef status;

  // send_message("SensorTask Started!\n");
  
  /* Infinite loop */
  for(;;)
  {
    // 检查低压标志，如果电压过低则挂起任务
    if (g_lowVoltageFlag) {
      send_message("Sensor task suspended due to low voltage!\n");
      vTaskSuspend(NULL);  // 挂起自己
    }
    
    // send_message("Reading sensor...\n");
    
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
    
    // 通过串口1发送数据
    send_message("WF5803-Temp: %.2f C, Press: %.2f kPa\n", temperature, pressure);
    
    // 延时1秒
    osDelay(1000);
  }
}

/**
  * @brief  Function implementing the ntcTask thread.
  * @param  argument: Not used
  * @retval None
  */
void StartNTCTask(void const * argument)
{
  float Temp_NTC;
  uint32_t adcValue;

  
  send_message("=== NTC Task Started! ===\n");
  
  /* Infinite loop */
  for(;;)
  {
    // 检查低压标志，如果电压过低则挂起任务
    if (g_lowVoltageFlag) {
      send_message("NTC task suspended due to low voltage!\n");
      vTaskSuspend(NULL);  // 挂起自己
    }
    
    // 读取 ADC 值
    adcValue = Read_ADC0();
    
    // 计算温度
    Temp_NTC = compute_ntc_temperature(adcValue);
    
    // 通过串口1发送结果
    send_message("NTC-Temp: %.2fC\n", Temp_NTC);
    
    // 延时1秒
    osDelay(1000);
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
        
        send_message("!!! LOW VOLTAGE ALERT !!!\n");
        send_message("Suspending All tasks...\n");
        
        // 挂起其他任务
        if (sensorTaskHandle != NULL) {
          vTaskSuspend(sensorTaskHandle);
        }
        if (ntcTaskHandle != NULL) {
          vTaskSuspend(ntcTaskHandle);
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

/* USER CODE END Application */
