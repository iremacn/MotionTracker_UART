#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"
#include "main.h"

/* Exported variables -------------------------------------------------------*/
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;  // ESP32 için UART3
extern UART_HandleTypeDef huart4;  // ESP32 için UART4 (PC10/PC11)

/* Exported functions prototypes ---------------------------------------------*/
void MX_USART2_UART_Init(void);
void MX_USART3_UART_Init(void);  // ESP32 için UART3
void MX_UART4_Init(void);        // ESP32 için UART4 (PC10/PC11)

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */ 