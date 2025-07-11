/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Gyroscope-Based Motor Control
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c.h"
#include "spi.h" 
#include "tim.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>  // atoi() için
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
extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_FS;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void MX_I2C1_Init(void);
static void MX_USB_PCD_Init(void);
void MX_USART2_UART_Init(void);
void MX_TIM3_Init(void);
void MX_SPI1_Init(void);
void MX_USART3_UART_Init(void);
void MX_UART4_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// ESP32 WiFi Functions

// Debug mesaj gönderme fonksiyonu
void SendDebugMessage(const char* msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USB_PCD_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_SPI1_Init();
  MX_USART3_UART_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(1000);
  
  // UART2 test (debug için)
  HAL_UART_Transmit(&huart2, (uint8_t*)"STM32 Started Successfully\r\n", 28, 1000);
  
  // ✅ TIM3 PWM BAŞLATMA - MOTOR İÇİN
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK) {
    HAL_UART_Transmit(&huart2, (uint8_t*)"❌ TIM3 PWM Start FAILED!\r\n", 29, 1000);
  } else {
    HAL_UART_Transmit(&huart2, (uint8_t*)"✅ TIM3 PWM Started - Motor Ready\r\n", 37, 1000);
  }
  
  // UART4 test - ESP32'ye ilk sinyal gönder
  HAL_UART_Transmit(&huart4, (uint8_t*)"A", 1, 1000);
  HAL_Delay(100);
  HAL_UART_Transmit(&huart4, (uint8_t*)"B", 1, 1000);
  HAL_Delay(100);
  HAL_UART_Transmit(&huart4, (uint8_t*)"C", 1, 1000);
  HAL_Delay(100);

  HAL_UART_Transmit(&huart2, (uint8_t*)"UART4 Test Signals Sent (A,B,C)\r\n", 34, 1000);
  HAL_UART_Transmit(&huart2, (uint8_t*)"System Ready - UART4 @ 9600 baud!\r\n", 36, 1000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    // DASHBOARD İÇİN GYROSCOPE SİMÜLASYONU
    static uint32_t counter = 0;
    static char delayed_status_check[100] = {0};  // STATUS komutunu geciktirmek için
    counter++;
    
    // LED'leri yanıp söndür (çalıştığını görmek için)
    if(counter % 5 == 0) {
        HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);  // Mavi LED (daha yavaş)
    }
    
    // Simüle edilmiş gyroscope verileri
    float gyro_x = 10.0f * sin(counter * 0.1f);     // -10 ile +10 arası sinüs
    float gyro_y = 5.0f * cos(counter * 0.15f);     // -5 ile +5 arası kosinus  
    float gyro_z = 8.0f * sin(counter * 0.08f);     // -8 ile +8 arası sinüs
    
    // Motor hızını hesapla (otomatik mode için)
    float gyro_magnitude = sqrt(gyro_x*gyro_x + gyro_y*gyro_y + gyro_z*gyro_z);
    uint8_t auto_motor_speed = (uint8_t)(gyro_magnitude > 100.0f ? 100 : gyro_magnitude);
    
    // ESP32'ye JSON format gönder (ESP32 beklediği format)
    char json_data[200];
    
    // ESP32'den motor kontrol komutlarını dinle
    static char uart_rx_buffer[100];
    static uint8_t rx_index = 0;
    static uint8_t manual_motor_speed = 0;  // Manuel ayarlanan motor hızı
    static uint8_t use_manual_speed = 0;    // Manuel kontrol aktif mi?
    static uint8_t force_stop = 0;          // Zorla durdur (STOP butonu)
    
    if(__HAL_UART_GET_FLAG(&huart4, UART_FLAG_RXNE)) {
        uint8_t received_char;
        HAL_UART_Receive(&huart4, &received_char, 1, 1);
        
        if(received_char == '\n' || received_char == '\r') {
            uart_rx_buffer[rx_index] = '\0';
            
            // ESP32'den gelen komutları debug et
            if(strlen(uart_rx_buffer) > 0) {
                char debug_rx[120];
                sprintf(debug_rx, "RX from ESP32: '%s'\r\n", uart_rx_buffer);
                HAL_UART_Transmit(&huart2, (uint8_t*)debug_rx, strlen(debug_rx), 1000);
            }
            
            // "MOTOR_SET:75" formatındaki komutları parse et
            if(strncmp(uart_rx_buffer, "MOTOR_SET:", 10) == 0) {
                manual_motor_speed = atoi(&uart_rx_buffer[10]);
                if(manual_motor_speed > 100) manual_motor_speed = 100;  // Clamp
                
                // ✅ ÖZEL DURUM: STOP (0%) → ZORLA DURDUR
                if(manual_motor_speed == 0) {
                    use_manual_speed = 0;  // Manual mode KAPALI
                    force_stop = 1;        // Zorla durma aktif
                    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);  // LED söndür
                    HAL_UART_Transmit(&huart2, (uint8_t*)"🛑 MOTOR STOPPED - FORCE STOP ACTIVE\r\n", 41, 1000);
                } else {
                    use_manual_speed = 1;  // Manuel mode aktif
                    force_stop = 0;        // Zorla durma kapalı
                    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);  // LED aç
                }
                
                // ✅ MOTOR PWM KONTROLÜ AKTİF
                uint32_t pulse = (manual_motor_speed * 999) / 100;  // TIM3 period = 999
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
                
                char ack_msg[100];
                sprintf(ack_msg, "✅ MOTOR: %d%% PWM=%lu (%s)\r\n", 
                       manual_motor_speed, pulse, 
                       (manual_motor_speed == 0) ? "FORCE STOP" : "MANUAL MODE");
                HAL_UART_Transmit(&huart2, (uint8_t*)ack_msg, strlen(ack_msg), 1000);
                
                // ESP32'ye ACK gönder
                sprintf(ack_msg, "ACK:MOTOR_SET:%d\n", manual_motor_speed);
                HAL_UART_Transmit(&huart4, (uint8_t*)ack_msg, strlen(ack_msg), 500);
            }
            else if(strcmp(uart_rx_buffer, "AUTO") == 0) {
                use_manual_speed = 0;
                force_stop = 0;  // Zorla durma iptal et
                HAL_UART_Transmit(&huart2, (uint8_t*)"✅ SWITCHED TO AUTO MODE (GYRO CONTROL)\r\n", 42, 1000);
                // LED'i söndür - otomatik mode
                HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);  // Kırmızı LED söndür
            }
            else if(strncmp(uart_rx_buffer, "ESP32_", 6) == 0) {
                // ESP32 test mesajlarını echo et
                char echo_msg[100];
                sprintf(echo_msg, "ECHO:%s\n", uart_rx_buffer);
                HAL_UART_Transmit(&huart4, (uint8_t*)echo_msg, strlen(echo_msg), 500);
            }
            else if(strcmp(uart_rx_buffer, "STATUS") == 0) {
                strcpy(delayed_status_check, "STATUS");  // Sonra işle
            }
            
            rx_index = 0;
        }
        else if(rx_index < sizeof(uart_rx_buffer) - 1) {
            uart_rx_buffer[rx_index++] = received_char;
        }
    }
    
    // Motor hızını belirle (manuel, otomatik veya zorla durma)
    uint8_t final_motor_speed;
    if(force_stop) {
        final_motor_speed = 0;  // Zorla durdur
    } else if(use_manual_speed) {
        final_motor_speed = manual_motor_speed;  // Manuel kontrol
    } else {
        final_motor_speed = auto_motor_speed;    // Otomatik kontrol
    }
    
    // ✅ PWM'i sürekli güncelle (hem auto hem manual için)
    uint32_t final_pulse = (final_motor_speed * 999) / 100;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, final_pulse);
    
    // STATUS komutunu burada işle (değişkenler hazır olduktan sonra)
    if(strlen(delayed_status_check) > 0 && strcmp(delayed_status_check, "STATUS") == 0) {
        char status_msg[120];
        sprintf(status_msg, "STATUS:Manual=%d,ForceStop=%d,ManualSpeed=%d,AutoSpeed=%d,FinalSpeed=%d\n", 
               use_manual_speed, force_stop, manual_motor_speed, auto_motor_speed, final_motor_speed);
        HAL_UART_Transmit(&huart4, (uint8_t*)status_msg, strlen(status_msg), 500);
        delayed_status_check[0] = '\0';  // Clear
    }
    
    // BASİT STRING TEST (JSON karmaşık, önce basit test)
    sprintf(json_data, "MOTOR:%d,X:%.1f,Y:%.1f,Z:%.1f\n", 
            final_motor_speed, gyro_x, gyro_y, gyro_z);
    HAL_UART_Transmit(&huart4, (uint8_t*)json_data, strlen(json_data), 500);
    
    // Debug mesajı UART2'ye (daha az frequent)
    if(counter % 20 == 0) {  // Her 2 saniyede bir
        char debug_msg[200];
        sprintf(debug_msg, "📊 FINAL:%d%% [%s] AUTO:%d MANUAL:%d STOP:%d | GYRO: X=%.1f Y=%.1f Z=%.1f\r\n", 
                final_motor_speed, 
                force_stop ? "FORCE_STOP" : (use_manual_speed ? "MANUAL" : "AUTO"),
                auto_motor_speed, manual_motor_speed, force_stop, gyro_x, gyro_y, gyro_z);
        HAL_UART_Transmit(&huart2, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
    }
    
    HAL_Delay(1000);  // 1 saniyede bir
    
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_USART2
                              |RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_UART4
                              |RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}













/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_FS.Instance = USB;
  hpcd_USB_FS.Init.dev_endpoints = 8;
  hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3|LD4_Pin|LD3_Pin|LD5_Pin
                          |LD7_Pin|LD9_Pin|LD10_Pin|LD8_Pin
                          |LD6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DRDY_Pin MEMS_INT3_Pin MEMS_INT4_Pin MEMS_INT1_Pin
                           MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = DRDY_Pin|MEMS_INT3_Pin|MEMS_INT4_Pin|MEMS_INT1_Pin
                          |MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE3 LD4_Pin LD3_Pin LD5_Pin
                           LD7_Pin LD9_Pin LD10_Pin LD8_Pin
                           LD6_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_3|LD4_Pin|LD3_Pin|LD5_Pin
                          |LD7_Pin|LD9_Pin|LD10_Pin|LD8_Pin
                          |LD6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void ESP32_Init(void)
{
    HAL_Delay(500);  // Daha kısa boot bekleme

    // UART4 Test - Tek karakter gönder (PC10/PC11)
    HAL_UART_Transmit(&huart2, (uint8_t*)"Testing UART4 PC10/PC11 chars...\r\n", 35, 1000);

    for(int i = 0; i < 3; i++) {  // Daha az test
        HAL_UART_Transmit(&huart4, (uint8_t*)"A", 1, 100);
        HAL_Delay(50);
        HAL_UART_Transmit(&huart4, (uint8_t*)"B", 1, 100);
        HAL_Delay(50);
        HAL_UART_Transmit(&huart4, (uint8_t*)"C", 1, 100);
        HAL_Delay(100);
    }

    // Basit string test
    HAL_UART_Transmit(&huart4, (uint8_t*)"HELLO\n", 6, 100);
    HAL_Delay(50);
    HAL_UART_Transmit(&huart4, (uint8_t*)"WORLD\n", 6, 100);

    HAL_UART_Transmit(&huart4, (uint8_t*)"ESP32 UART4 Module Initialized!\r\n", 34, 1000);
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
