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

/* Exported functions prototypes ---------------------------------------------*/
void MX_USART2_UART_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */ 