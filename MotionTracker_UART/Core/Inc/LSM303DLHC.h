#ifndef __LSM303DLHC_H
#define __LSM303DLHC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f3xx_hal.h"

/* LSM303DLHC Accelerometer Defines */
#define LSM303DLHC_ACC_ADDR         0x32  // 0x19 << 1
#define LSM303DLHC_WHO_AM_I_A       0x0F
#define LSM303DLHC_CTRL_REG1_A      0x20
#define LSM303DLHC_CTRL_REG4_A      0x23
#define LSM303DLHC_OUT_X_L_A        0x28

/* LSM303DLHC Structure */
typedef struct
{
    int16_t x;       // Raw x-axis value
    int16_t y;       // Raw y-axis value
    int16_t z;       // Raw z-axis value
    float x_g;       // x-axis value in g
    float y_g;       // y-axis value in g
    float z_g;       // z-axis value in g
} LSM303DLHC_t;

/* Function Prototypes */
HAL_StatusTypeDef LSM303DLHC_Init(void);
void LSM303DLHC_Read_All(LSM303DLHC_t *DataStruct);
uint8_t CalculateMotorSpeed(LSM303DLHC_t *accel);
void SendDebugMessage(const char* msg);

#ifdef __cplusplus
}
#endif

#endif /* __LSM303DLHC_H */ 