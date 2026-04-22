/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CHASSIS_TASK */
osThreadId_t CHASSIS_TASKHandle;
const osThreadAttr_t CHASSIS_TASK_attributes = {
  .name = "CHASSIS_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for POSITION_TASK */
osThreadId_t POSITION_TASKHandle;
const osThreadAttr_t POSITION_TASK_attributes = {
  .name = "POSITION_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for MAIN_TASK */
osThreadId_t MAIN_TASKHandle;
const osThreadAttr_t MAIN_TASK_attributes = {
  .name = "MAIN_TASK",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for UART_RX_TASK */
osThreadId_t UART_RX_TASKHandle;
const osThreadAttr_t UART_RX_TASK_attributes = {
  .name = "UART_RX_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal2,
};
/* Definitions for MUSIC_TASK */
osThreadId_t MUSIC_TASKHandle;
const osThreadAttr_t MUSIC_TASK_attributes = {
  .name = "MUSIC_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow2,
};
/* Definitions for KEY_TASK */
osThreadId_t KEY_TASKHandle;
const osThreadAttr_t KEY_TASK_attributes = {
  .name = "KEY_TASK",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal1,
};
/* Definitions for KeyQueue */
osMessageQueueId_t KeyQueueHandle;
const osMessageQueueAttr_t KeyQueue_attributes = {
  .name = "KeyQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
extern void chassis_task(void *argument);
extern void position_task(void *argument);
extern void main_task(void *argument);
extern void uart_rx_task(void *argument);
extern void music_task(void *argument);
extern void key_task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

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

  /* Create the queue(s) */
  /* creation of KeyQueue */
  KeyQueueHandle = osMessageQueueNew (16, sizeof(uint8_t), &KeyQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of CHASSIS_TASK */
  CHASSIS_TASKHandle = osThreadNew(chassis_task, NULL, &CHASSIS_TASK_attributes);

  /* creation of POSITION_TASK */
  POSITION_TASKHandle = osThreadNew(position_task, NULL, &POSITION_TASK_attributes);

  /* creation of MAIN_TASK */
  MAIN_TASKHandle = osThreadNew(main_task, NULL, &MAIN_TASK_attributes);

  /* creation of UART_RX_TASK */
  UART_RX_TASKHandle = osThreadNew(uart_rx_task, NULL, &UART_RX_TASK_attributes);

  /* creation of MUSIC_TASK */
  MUSIC_TASKHandle = osThreadNew(music_task, NULL, &MUSIC_TASK_attributes);

  /* creation of KEY_TASK */
  KEY_TASKHandle = osThreadNew(key_task, NULL, &KEY_TASK_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
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

/* USER CODE END Application */

