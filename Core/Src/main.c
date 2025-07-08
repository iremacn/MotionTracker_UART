/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Gyroscope-Based Motor Control
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "spi.h"
#include "i2c.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "motor.h"

// --- Definitions ---
typedef struct {
    float x;
    float y;
    float z;
    float magnitude;
} L3GD20_Data_t;

#define UART_BUFFER_SIZE 256

// --- Global Variables ---
L3GD20_Data_t gyro_data;
uint8_t current_motor_speed = 0;
uint32_t loop_counter = 0;
char uart_msg[UART_BUFFER_SIZE];

volatile uint8_t motorSpeed = 0;
char debugMsg[UART_BUFFER_SIZE];
uint8_t rxBuffer[RX_BUFFER_SIZE];  // RX_BUFFER_SIZE main.h içinde 64 olarak tanımlanmalı

// --- Function Prototypes ---
void SystemClock_Config(void);
void SendDebugMessage(const char* message);
void L3GD20_Init(void);
void L3GD20_ReadData(L3GD20_Data_t* data);
uint8_t L3GD20_ReadRegister(uint8_t reg);
void L3GD20_WriteRegister(uint8_t reg, uint8_t value);
uint8_t L3GD20_CalculateMotorSpeed(L3GD20_Data_t* gyro_data);
void L3GD20_DisplayOnTerminal(L3GD20_Data_t* gyro_data, uint8_t motor_speed);

// Motor functions from motor.c
void HW153_SetMotor(uint8_t speed, uint8_t direction);
#define MOTOR_DIRECTION_FORWARD  1
#define MOTOR_DIRECTION_BACKWARD 0

int main(void)
{
   HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();

  HAL_Delay(1000);
  HAL_UART_Transmit(&huart2, (uint8_t*)"STM32 Başlatıldı\r\n", 21, HAL_MAX_DELAY);

  Motor_Init();
  L3GD20_Init();

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET);

  while (1)
  {
    L3GD20_ReadData(&gyro_data);
    current_motor_speed = L3GD20_CalculateMotorSpeed(&gyro_data);
    HW153_SetMotor(current_motor_speed, MOTOR_DIRECTION_FORWARD);

    if (loop_counter % 3 == 0)
    {
        L3GD20_DisplayOnTerminal(&gyro_data, current_motor_speed);
        if (current_motor_speed > 0)
            HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11);
        else
            HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET);
    }

    sprintf(uart_msg, "Gyro[X:%.1f Y:%.1f Z:%.1f] |%.1f| -> Motor:%d%%\r\n",
            gyro_data.x, gyro_data.y, gyro_data.z, gyro_data.magnitude, current_motor_speed);
    SendDebugMessage(uart_msg);

    loop_counter++;
    HAL_Delay(500);
  }
}

void SendDebugMessage(const char* message)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)message, strlen(message), HAL_MAX_DELAY);
}

void L3GD20_Init(void)
{
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(10);
    L3GD20_WriteRegister(0x20, 0xFF);
    HAL_Delay(10);
    L3GD20_WriteRegister(0x23, 0x00);
    HAL_Delay(10);
}

void L3GD20_ReadData(L3GD20_Data_t* data)
{
    uint8_t buffer[6];
    int16_t raw_x, raw_y, raw_z;

    buffer[0] = L3GD20_ReadRegister(0x28);
    buffer[1] = L3GD20_ReadRegister(0x29);
    buffer[2] = L3GD20_ReadRegister(0x2A);
    buffer[3] = L3GD20_ReadRegister(0x2B);
    buffer[4] = L3GD20_ReadRegister(0x2C);
    buffer[5] = L3GD20_ReadRegister(0x2D);

    raw_x = (int16_t)((buffer[1] << 8) | buffer[0]);
    raw_y = (int16_t)((buffer[3] << 8) | buffer[2]);
    raw_z = (int16_t)((buffer[5] << 8) | buffer[4]);

    data->x = (float)raw_x * 0.00875f;
    data->y = (float)raw_y * 0.00875f;
    data->z = (float)raw_z * 0.00875f;

    data->magnitude = sqrtf(data->x * data->x + data->y * data->y + data->z * data->z);
}

uint8_t L3GD20_ReadRegister(uint8_t reg)
{
    uint8_t tx = reg | 0x80;
    uint8_t rx = 0;

    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &tx, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &rx, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);

    return rx;
}

void L3GD20_WriteRegister(uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = {reg, value};
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
}

uint8_t L3GD20_CalculateMotorSpeed(L3GD20_Data_t* gyro_data)
{
    float magnitude = gyro_data->magnitude;
    if (magnitude < 0.5f) return 0;
    if (magnitude > 10.0f) return 100;
    return (uint8_t)(((magnitude - 0.5f) / 9.5f) * 100);
}

void L3GD20_DisplayOnTerminal(L3GD20_Data_t* gyro_data, uint8_t motor_speed)
{
    char msg[128];
    sprintf(msg, "Gyro X: %.2f Y: %.2f Z: %.2f\r\n", gyro_data->x, gyro_data->y, gyro_data->z);
    SendDebugMessage(msg);
    sprintf(msg, "Magnitude: %.2f dps | Motor: %d%%\r\n\r\n", gyro_data->magnitude, motor_speed);
    SendDebugMessage(msg);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_15);
    HAL_Delay(200);
  }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_HCLK
                                  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
