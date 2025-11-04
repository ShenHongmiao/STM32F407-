/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
#include "usart.h"
#include <stdio.h>

/* USER CODE BEGIN 0 */


// 定义发送缓冲区大小
#define UART_TX_BUFFER_SIZE 256
#define UART_RX_BUFFER_SIZE 64          // 环形或临时接收缓冲区总大小

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN 1 */

// ============ 双缓冲区 DMA 发送系统 ============
// 双缓冲区定义
static char buffer_a[UART_TX_BUFFER_SIZE];
static char buffer_b[UART_TX_BUFFER_SIZE];

// 状态变量
static volatile uint8_t is_tx_busy = 0;           // DMA 是否忙碌 (0=空闲, 1=忙碌)
static volatile uint16_t idle_buf_len = 0;        // 空闲缓冲区中待发送的数据长度
static volatile uint8_t active_buf_index = 0;     // 当前 DMA 正在发送的缓冲区 (0=A, 1=B)

// 互斥锁句柄（保护缓冲区切换和状态变量）
static osMutexId uart_tx_mutex = NULL;

//static uint8_t rx_buffer[UART_RX_BUFFER_SIZE]; // 用于累积数据的缓冲区
uint8_t rx_byte;               // 用于单字节接收



/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */
    //设置PD3 PD4的输出电平（初始化）（分开设置）
  HAL_GPIO_WritePin(GPIOD, PD3_OUT_Pin, GPIO_PIN_RESET);// PD3 输出电平，高电平应该是 HAL_GPIO_WritePin(GPIOD, PD3_OUT_Pin, GPIO_PIN_SET),低电平应该是 HAL_GPIO_WritePin(GPIOD, PD3_OUT_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, PD4_OUT_Pin, GPIO_PIN_RESET);// PD4 输出电平，高电平应该是 HAL_GPIO_WritePin(GPIOD, PD4_OUT_Pin, GPIO_PIN_SET),低电平应该是 HAL_GPIO_WritePin(GPIOD, PD4_OUT_Pin, GPIO_PIN_RESET);
  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN USART1_MspInit 1 */
    /* USART1 中断配置 */
    /* 注意: 优先级必须 >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5) */
    HAL_NVIC_SetPriority(USART1_IRQn, 6, 0); // 设置中断优先级为6
    HAL_NVIC_EnableIRQ(USART1_IRQn);         // 启用中断

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PD5     ------> USART2_TX
    PD6     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_TX Init */
    hdma_usart2_tx.Instance = DMA1_Stream6;
    hdma_usart2_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);

  /* USER CODE BEGIN USART2_MspInit 1 */
    /* USART2 中断配置 */
    /* 注意: 优先级必须 >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5) */
    /* 才能在中断中调用 FreeRTOS API。数值越大，优先级越低 */
    HAL_NVIC_SetPriority(USART2_IRQn, 6, 0); // 设置中断优先级为6
    HAL_NVIC_EnableIRQ(USART2_IRQn);         // 启用中断
  /* USER CODE END USART2_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    HAL_NVIC_DisableIRQ(USART1_IRQn); 
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);

  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PD5     ------> USART2_TX
    PD6     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_5|GPIO_PIN_6);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/**
 * @brief  串口2发送格式化字符串（双缓冲区非阻塞DMA发送）
 * @param  format: 格式化字符串
 * @param  ...: 可变参数
 * @retval None
 * 
 * @note   使用示例:
 *         send_message("Hello World!\n");
 *         send_message("Temperature: %.2f°C\n", temp);
 *         send_message("ADC: %d, Voltage: %.2fV\n", adc_value, voltage);
 * 
 * @note   RTOS启动前：使用阻塞发送
 * @note   RTOS启动后：使用双缓冲区DMA，完全非阻塞，数据覆盖策略
 * @note   数据新鲜度 > 完整性，高频发送时自动丢弃旧数据
 */
void send_message(const char *format, ...)
{
    va_list args;
    int len;
    
    // 开始可变参数处理
    va_start(args, format);
    
    // 检查 RTOS 是否已启动
    if (osKernelRunning()) {
        // ========== RTOS 已启动：双缓冲区非阻塞 DMA 发送 ==========
        
        // 创建互斥锁（首次调用时）
        if (uart_tx_mutex == NULL) {
            osMutexDef(uart_tx_mutex);
            uart_tx_mutex = osMutexCreate(osMutex(uart_tx_mutex));
        }
        
        // 尝试获取互斥锁（超时为0，立即返回）
        if (osMutexWait(uart_tx_mutex, 0) != osOK) {
            va_end(args);
            return;  // 获取失败，舍弃本次发送
        }
        
        // 确定空闲缓冲区（与 active_buf_index 相反的那个）
        char *idle_buffer = (active_buf_index == 0) ? buffer_b : buffer_a;
        
        // 格式化数据到空闲缓冲区（无条件覆盖）
        len = vsnprintf(idle_buffer, UART_TX_BUFFER_SIZE, format, args);
        va_end(args);
        
        // 验证长度
        if (len <= 0 || len >= UART_TX_BUFFER_SIZE) {
            osMutexRelease(uart_tx_mutex);
            return;
        }
        
        // 更新空闲缓冲区长度
        idle_buf_len = len;
        
        // 检查 DMA 是否空闲
        if (is_tx_busy == 0) {
            // DMA 空闲，立即启动发送
            
            // 切换缓冲区（空闲区变为活动区）
            active_buf_index = (active_buf_index == 0) ? 1 : 0;
            char *active_buffer = (active_buf_index == 0) ? buffer_a : buffer_b;
            
            // 清空空闲区长度
            uint16_t send_len = idle_buf_len;
            idle_buf_len = 0;
            
            // 设置忙碌标志
            is_tx_busy = 1;
            
            // 启动 DMA 发送
            if (HAL_UART_Transmit_DMA(&huart2, (uint8_t*)active_buffer, send_len) != HAL_OK) {
                // 启动失败，清除忙碌标志
                is_tx_busy = 0;
            }
        }
        // 否则 DMA 正忙，数据已写入空闲区，等待回调函数发送
        
        // 释放互斥锁
        osMutexRelease(uart_tx_mutex);
        
    } else {
        // ========== RTOS 未启动：阻塞发送 ==========
        char temp_buffer[UART_TX_BUFFER_SIZE];
        
        len = vsnprintf(temp_buffer, UART_TX_BUFFER_SIZE, format, args);
        va_end(args);
        
        if (len > 0 && len < UART_TX_BUFFER_SIZE) {
            HAL_UART_Transmit(&huart2, (uint8_t*)temp_buffer, len, 1000);
        }
    }
}


/**
 * @brief  UART DMA 发送完成回调函数（流水线切换）
 * @param  huart: UART 句柄
 * @retval None
 * @note   在 DMA 中断中调用，负责检查并启动下一帧发送
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        // 清除忙碌标志
        is_tx_busy = 0;
        
        // 检查空闲缓冲区是否有待发送数据
        if (idle_buf_len > 0) {
            // 有新数据，切换缓冲区并启动发送
            
            // 切换缓冲区（空闲区变为活动区）
            active_buf_index = (active_buf_index == 0) ? 1 : 0;
            char *active_buffer = (active_buf_index == 0) ? buffer_a : buffer_b;
            
            // 获取待发送长度并清零
            uint16_t send_len = idle_buf_len;
            idle_buf_len = 0;
            
            // 设置忙碌标志
            is_tx_busy = 1;
            
            // 启动 DMA 发送
            if (HAL_UART_Transmit_DMA(&huart2, (uint8_t*)active_buffer, send_len) != HAL_OK) {
                // 启动失败，清除忙碌标志
                is_tx_busy = 0;
            }
        }
        // 否则无新数据，DMA 保持空闲状态，等待下次 send_message 调用
    }
}

/**
 * @brief  UART 错误回调函数
 * @param  huart: UART 句柄
 * @retval None
 * @note   当 DMA 传输出错时调用
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        // DMA 传输错误处理
        // 可以在这里记录错误、重启传输等
        // 目前简单地忽略错误，让系统继续运行
    }
}

/**
 * @brief  UART 接收完成中断回调函数
 * @param  huart: UART 句柄
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        // 将接收到的字节放入队列
        // 注意：osMessagePut 可以在 ISR 中调用，但中断优先级必须正确配置
        // 队列满时会返回错误，不会阻塞
        osMessagePut(usart_rx_queueHandle, (uint32_t)rx_byte, 0);
        
        
        // 重新启动接收
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}
/* USER CODE END 1 */
