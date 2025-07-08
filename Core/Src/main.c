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
#define RX_BUFFER_SIZE 64

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

// LED Functions - STM32F3 Discovery LEDs
void LED_Init_All(void);
void LED_Set_All(uint8_t state);
void LED_Rainbow_Effect(void);
void LED_Speed_Display(uint8_t speed);
void LED_Gyro_Effect(L3GD20_Data_t* gyro_data);

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
  LED_Init_All();  // Tüm LED'leri başlat

  // Startup LED Show! 🌈
  for(int i = 0; i < 3; i++) {
    LED_Rainbow_Effect();
    HAL_Delay(200);
  }
  
  HAL_UART_Transmit(&huart2, (uint8_t*)"🌈 LED Show Tamamlandı! 🎉\r\n", 35, HAL_MAX_DELAY);

  while (1)
  {
    L3GD20_ReadData(&gyro_data);
    current_motor_speed = L3GD20_CalculateMotorSpeed(&gyro_data);
    HW153_SetMotor(current_motor_speed, MOTOR_DIRECTION_FORWARD);

    // LED Effects! ✨
    LED_Speed_Display(current_motor_speed);  // Motor hızına göre LED'ler
    LED_Gyro_Effect(&gyro_data);             // Gyroscope efekti
    
    if (loop_counter % 3 == 0)
    {
        L3GD20_DisplayOnTerminal(&gyro_data, current_motor_speed);
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

// ================================
// 🌈 LED Functions - STM32F3 Discovery LEDs 🌈
// ================================

/**
 * @brief STM32F3 Discovery üzerindeki 8 LED'i başlatır
 * LD3(PE9)-Red, LD4(PE8)-Blue, LD5(PE10)-Orange, LD6(PE15)-Green
 * LD7(PE11)-Green, LD8(PE14)-Orange, LD9(PE12)-Blue, LD10(PE13)-Red
 */
void LED_Init_All(void)
{
    // PE8-PE15 pinleri zaten GPIO_Init'de yapılandırılmış
    // Tüm LED'leri kapat
    LED_Set_All(0);
    
    HAL_UART_Transmit(&huart2, (uint8_t*)"🔥 8 LED Başlatıldı! 🔥\r\n", 28, HAL_MAX_DELAY);
}

/**
 * @brief Tüm LED'leri açar veya kapatır
 * @param state: 1=Aç, 0=Kapat
 */
void LED_Set_All(uint8_t state)
{
    GPIO_PinState pin_state = state ? GPIO_PIN_SET : GPIO_PIN_RESET;
    
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, pin_state);   // LD4 - Blue
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, pin_state);   // LD3 - Red
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, pin_state);  // LD5 - Orange
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, pin_state);  // LD7 - Green
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, pin_state);  // LD9 - Blue
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, pin_state);  // LD10 - Red
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, pin_state);  // LD8 - Orange
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, pin_state);  // LD6 - Green
}

/**
 * @brief Gökkuşağı efekti - LED'leri sırayla yakar
 */
void LED_Rainbow_Effect(void)
{
    uint16_t leds[] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, 
                       GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};
    
    // Sırayla yak
    for(int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(GPIOE, leds[i], GPIO_PIN_SET);
        HAL_Delay(50);
    }
    
    // Sırayla söndür
    for(int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(GPIOE, leds[i], GPIO_PIN_RESET);
        HAL_Delay(50);
    }
}

/**
 * @brief Motor hızına göre LED gösterimi
 * @param speed: Motor hızı (0-100%)
 */
void LED_Speed_Display(uint8_t speed)
{
    // Hızı 8 LED'e böl (her LED %12.5'lik dilimi temsil eder)
    uint8_t led_count = (speed * 8) / 100;
    
    // Tüm LED'leri söndür
    LED_Set_All(0);
    
    // Hıza göre LED'leri yak
    uint16_t leds[] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, 
                       GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};
    
    for(int i = 0; i < led_count && i < 8; i++) {
        HAL_GPIO_WritePin(GPIOE, leds[i], GPIO_PIN_SET);
    }
    
    // En yüksek LED'i yanıp söndür (motor çalışıyor göstergesi)
    if(speed > 0 && led_count > 0) {
        HAL_GPIO_TogglePin(GPIOE, leds[led_count-1]);
    }
}

/**
 * @brief Gyroscope verilerine göre LED efekti
 * @param gyro_data: Gyroscope verileri
 */
void LED_Gyro_Effect(L3GD20_Data_t* gyro_data)
{
    // X eksenine göre mavi LED'ler (PE8, PE12)
    if(fabsf(gyro_data->x) > 2.0f) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET);   // LD4 - Blue
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_SET);  // LD9 - Blue
    }
    
    // Y eksenine göre kırmızı LED'ler (PE9, PE13)
    if(fabsf(gyro_data->y) > 2.0f) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, GPIO_PIN_SET);   // LD3 - Red
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, GPIO_PIN_SET);  // LD10 - Red
    }
    
    // Z eksenine göre yeşil LED'ler (PE11, PE15)
    if(fabsf(gyro_data->z) > 2.0f) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET);  // LD7 - Green
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET);  // LD6 - Green
    }
    
    // Magnitude'e göre turuncu LED'ler (PE10, PE14)
    if(gyro_data->magnitude > 5.0f) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_SET);  // LD5 - Orange
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_SET);  // LD8 - Orange
    }
}
