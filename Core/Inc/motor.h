#ifndef __MOTOR_H__
#define __MOTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* Function Prototypes */
void SetPWMDuty(uint8_t duty);
void SetMotorSpeed(uint8_t speed);
void ProcessCommand(void);
extern void SendDebugMessage(const char* msg);

/* Motor Döndürme Fonksiyonları */
void Motor_Init(void);
void Motor_Test(void);
void Motor_SimpleTest(void);
void Motor_PinTest(void);

/* HW-153 V1 Motor Driver Fonksiyonları */
void HW153_SetMotor(uint8_t speed, uint8_t direction);
void HW153_MotorTest(void);
void Motor_RotateClockwise(uint8_t speed, uint32_t duration_ms);
void Motor_RotateCounterClockwise(uint8_t speed, uint32_t duration_ms);
void Motor_RotateContinuous(uint8_t speed);
void Motor_Stop(void);
void Motor_SpeedRamp(uint8_t start_speed, uint8_t end_speed, uint32_t ramp_time_ms);
void Motor_Pulse(uint8_t speed, uint32_t on_time_ms, uint32_t off_time_ms, uint8_t pulse_count);

/* External Variables */
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim3;

#ifdef __cplusplus
}
#endif

#endif /* __MOTOR_H__ */ 