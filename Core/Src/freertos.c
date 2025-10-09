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
#include "usart.h"
#include "i2c.h"
#include "adc.h"
#include <string.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId sensorTaskHandle;
osThreadId ntcTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartSensorTask(void const * argument);
void StartNTCTask(void const * argument);

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
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* definition and creation of sensorTask */
  osThreadDef(sensorTask, StartSensorTask, osPriorityNormal, 0, 512);
  sensorTaskHandle = osThreadCreate(osThread(sensorTask), NULL);
  
  /* definition and creation of ntcTask */
  osThreadDef(ntcTask, StartNTCTask, osPriorityNormal, 0, 256);
  ntcTaskHandle = osThreadCreate(osThread(ntcTask), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
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
  
  printf("SensorTask Started!\n");
  
  // 延时等待系统稳定
  osDelay(100);
  
  /* Infinite loop */
  for(;;)
  {
    printf("Reading sensor...\n");
    
    // 测试 I2C 通信
    uint8_t test_cmd = 0x0A;
    status = HAL_I2C_Mem_Write(&hi2c1, WF5803F_ADDR, WF5803F_REG_CTRL, I2C_MEMADD_SIZE_8BIT, &test_cmd, 1, 100);
    
    if (status == HAL_OK) {
      printf("I2C Write OK\n");
    } else {
      printf("I2C Write Failed: %d\n", status);
    }
    
    // 获取温度和气压数据
    WF5803F_GetData(&temperature, &pressure);
    
    // 直接打印到终端
    printf("Temp: %.2f C, Press: %.2f kPa\n", temperature, pressure);
    
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

  printf("=== NTC Task Started! ===\n");
  
  // 延时等待系统稳定
  osDelay(300);
  
  /* Infinite loop */
  for(;;)
  {
    // 读取 ADC 值
    adcValue = Read_ADC0();
    
    // 计算温度
    Temp_NTC = compute_ntc_temperature(adcValue);
    
    // 打印结果
    printf("NTC - ADC: %lu, Temp: %.2f°C\n", adcValue, Temp_NTC);
    
    // 延时1秒
    osDelay(1000);
  }
}

/* USER CODE END Application */
