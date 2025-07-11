/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                               */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Configure GPIO pins */
  
  // HW-153 V1 Motor Driver - PA6 (INA/PWM) için TIM3_CH1
  GPIO_InitStruct.Pin = GPIO_PIN_6;  // PA6 - PWM/Speed Control
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // HW-153 V1 Motor Driver - PA7 (INB) Direction Control
  GPIO_InitStruct.Pin = GPIO_PIN_7;  // PA7 - Direction Control
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  // Motor başlangıçta durdurulmuş olsun
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

  // L3GD20 Gyroscope CS Pin - PE3
  GPIO_InitStruct.Pin = GPIO_PIN_3;  // PE3 - L3GD20 CS (Chip Select)
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  
  // CS pinini başlangıçta HIGH yap (deselected)
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);

  // I2C pins
  GPIO_InitStruct.Pin = I2C1_SCL_PIN | I2C1_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // LED pins
  GPIO_InitStruct.Pin = LD3_Pin | LD4_Pin | LD5_Pin | LD6_Pin |
                        LD7_Pin | LD8_Pin | LD9_Pin | LD10_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  // Set initial LED states
  HAL_GPIO_WritePin(GPIOE, LD3_Pin | LD4_Pin | LD5_Pin | LD6_Pin |
                          LD7_Pin | LD8_Pin | LD9_Pin | LD10_Pin, GPIO_PIN_RESET);
} 