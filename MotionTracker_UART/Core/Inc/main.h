/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>  // atoi için
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define UART_BUFFER_SIZE 256
#define RX_BUFFER_SIZE   32
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
// Global değişkenler
extern volatile uint8_t motorSpeed;
extern char debugMsg[UART_BUFFER_SIZE];
extern uint8_t rxBuffer[RX_BUFFER_SIZE];

// Timer tanımlamaları
#define TIM3_PRESCALER  71
#define TIM3_PERIOD     999

// LED Pin Tanımlamaları
#define LD3_Pin                 GPIO_PIN_9
#define LD3_GPIO_Port          GPIOE
#define LD4_Pin                 GPIO_PIN_8
#define LD4_GPIO_Port          GPIOE
#define LD5_Pin                 GPIO_PIN_10
#define LD5_GPIO_Port          GPIOE
#define LD6_Pin                 GPIO_PIN_15  // Kırmızı LED
#define LD6_GPIO_Port          GPIOE
#define LD7_Pin                 GPIO_PIN_11  // Yeşil LED
#define LD7_GPIO_Port          GPIOE
#define LD8_Pin                 GPIO_PIN_14
#define LD8_GPIO_Port          GPIOE
#define LD9_Pin                 GPIO_PIN_12
#define LD9_GPIO_Port          GPIOE
#define LD10_Pin                GPIO_PIN_13
#define LD10_GPIO_Port         GPIOE

// HW-153 V1 Motor Driver Pin Tanımlamaları
#define MOTOR_PWM_PIN          GPIO_PIN_6   // PA6 - TIM3_CH1 (INA pin)
#define MOTOR_PWM_GPIO_Port    GPIOA
#define MOTOR_PWM_CHANNEL      TIM_CHANNEL_1

#define MOTOR_DIR_PIN          GPIO_PIN_7   // PA7 - Direction pin (INB pin)  
#define MOTOR_DIR_GPIO_Port    GPIOA

// Motor yön tanımlamaları
#define MOTOR_DIRECTION_FORWARD   0
#define MOTOR_DIRECTION_BACKWARD  1

// I2C Pin Tanımlamaları
#define I2C1_SCL_PIN           GPIO_PIN_6
#define I2C1_SCL_GPIO_Port     GPIOB
#define I2C1_SDA_PIN           GPIO_PIN_7
#define I2C1_SDA_GPIO_Port     GPIOB

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void SendDebugMessage(const char* msg);
void ESP32_SendData(const char* data);
void ESP32_Init(void);
void ESP32_CheckCommands(void);
void ESP32_ProcessCommand(char* command);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DRDY_Pin GPIO_PIN_2
#define DRDY_GPIO_Port GPIOE
#define MEMS_INT3_Pin GPIO_PIN_4
#define MEMS_INT3_GPIO_Port GPIOE
#define MEMS_INT4_Pin GPIO_PIN_5
#define MEMS_INT4_GPIO_Port GPIOE
#define OSC32_IN_Pin GPIO_PIN_14
#define OSC32_IN_GPIO_Port GPIOC
#define OSC32_OUT_Pin GPIO_PIN_15
#define OSC32_OUT_GPIO_Port GPIOC
#define OSC_IN_Pin GPIO_PIN_0
#define OSC_IN_GPIO_Port GPIOF
#define OSC_OUT_Pin GPIO_PIN_1
#define OSC_OUT_GPIO_Port GPIOF
#define B1_Pin GPIO_PIN_0
#define B1_GPIO_Port GPIOA
#define SPI1_SCK_Pin GPIO_PIN_5
#define SPI1_SCK_GPIO_Port GPIOA
#define SPI1_MISO_Pin GPIO_PIN_7
#define SPI1_MISO_GPIO_Port GPIOA
#define LD4_Pin GPIO_PIN_8
#define LD4_GPIO_Port GPIOE
#define LD3_Pin GPIO_PIN_9
#define LD3_GPIO_Port GPIOE
#define LD5_Pin GPIO_PIN_10
#define LD5_GPIO_Port GPIOE
#define LD7_Pin GPIO_PIN_11
#define LD7_GPIO_Port GPIOE
#define LD9_Pin GPIO_PIN_12
#define LD9_GPIO_Port GPIOE
#define LD10_Pin GPIO_PIN_13
#define LD10_GPIO_Port GPIOE
#define LD8_Pin GPIO_PIN_14
#define LD8_GPIO_Port GPIOE
#define LD6_Pin GPIO_PIN_15
#define LD6_GPIO_Port GPIOE
#define DP_Pin GPIO_PIN_12
#define DP_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define I2C1_SCL_Pin GPIO_PIN_6
#define I2C1_SCL_GPIO_Port GPIOB
#define I2C1_SDA_Pin GPIO_PIN_7
#define I2C1_SDA_GPIO_Port GPIOB
#define MEMS_INT1_Pin GPIO_PIN_0
#define MEMS_INT1_GPIO_Port GPIOE
#define MEMS_INT2_Pin GPIO_PIN_1
#define MEMS_INT2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
