#include "LSM303DLHC.h"
#include "i2c.h"
#include "motor.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

extern char debugMsg[UART_BUFFER_SIZE];  // From main.c

#define LSM303DLHC_ACCEL_ADDR     0x32 // 0x19 << 1
#define LSM303DLHC_MAG_ADDR       0x3C // 0x1E << 1

// Registers
#define CTRL_REG1_A               0x20
#define CTRL_REG4_A               0x23
#define OUT_X_L_A                 0x28
#define OUT_X_H_A                 0x29
#define OUT_Y_L_A                 0x2A
#define OUT_Y_H_A                 0x2B
#define OUT_Z_L_A                 0x2C
#define OUT_Z_H_A                 0x2D

// Global variables
volatile float accel_x = 0;
volatile float accel_y = 0;
volatile float accel_z = 0;

HAL_StatusTypeDef LSM303DLHC_Init(void)
{
    uint8_t data = 0;
    HAL_StatusTypeDef status;
    
    sprintf(debugMsg, "Starting LSM303DLHC init...\r\n");
    SendDebugMessage(debugMsg);
    
    // Farklı I2C adreslerini dene
    uint8_t test_addresses[] = {0x32, 0x30, 0x18, 0x19};
    uint8_t working_addr = 0;
    
    for (int i = 0; i < 4; i++) {
        sprintf(debugMsg, "Testing I2C address: 0x%02X\r\n", test_addresses[i]);
        SendDebugMessage(debugMsg);
        
        status = HAL_I2C_IsDeviceReady(&hi2c1, test_addresses[i], 3, 1000);
        if (status == HAL_OK) {
            working_addr = test_addresses[i];
            sprintf(debugMsg, "I2C device found at: 0x%02X\r\n", working_addr);
            SendDebugMessage(debugMsg);
            break;
        }
        HAL_Delay(10);
    }
    
    if (working_addr == 0) {
        sprintf(debugMsg, "No I2C device found!\r\n");
        SendDebugMessage(debugMsg);
        return HAL_ERROR;
    }
    
    // Basit başlatma - sadece gerekli register'lar
    HAL_Delay(100); // Sensörün hazır olması için bekle
    
    // CTRL_REG1_A: Normal mode, 10Hz, XYZ enabled (daha düşük frekans)
    data = 0x27;  // 0010 0111b - 10Hz, normal mode, XYZ enabled
    status = HAL_I2C_Mem_Write(&hi2c1, working_addr, CTRL_REG1_A, 1, &data, 1, 2000);
    if (status != HAL_OK) {
        sprintf(debugMsg, "CTRL_REG1_A write failed: %d\r\n", status);
        SendDebugMessage(debugMsg);
        return status;
    }
    
    HAL_Delay(50);
    
    // CTRL_REG4_A: ±2g, normal resolution
    data = 0x00;  // 0000 0000b - ±2g, normal resolution
    status = HAL_I2C_Mem_Write(&hi2c1, working_addr, CTRL_REG4_A, 1, &data, 1, 2000);
    if (status != HAL_OK) {
        sprintf(debugMsg, "CTRL_REG4_A write failed: %d\r\n", status);
        SendDebugMessage(debugMsg);
        return status;
    }
    
    HAL_Delay(50);
    
    // Verification
    status = HAL_I2C_Mem_Read(&hi2c1, working_addr, CTRL_REG1_A, 1, &data, 1, 2000);
    if (status == HAL_OK) {
        sprintf(debugMsg, "CTRL_REG1_A readback: 0x%02X\r\n", data);
        SendDebugMessage(debugMsg);
    }
    
    sprintf(debugMsg, "LSM303DLHC init completed with address: 0x%02X\r\n", working_addr);
    SendDebugMessage(debugMsg);
    
    return HAL_OK;
}

HAL_StatusTypeDef LSM303DLHC_ReadAccel(void)
{
    uint8_t data[6];
    int16_t raw_x, raw_y, raw_z;
    HAL_StatusTypeDef status;
    
    // Read all acceleration registers at once (X, Y, Z)
    status = HAL_I2C_Mem_Read(&hi2c1, LSM303DLHC_ACCEL_ADDR, OUT_X_L_A | 0x80, 1, data, 6, 1000);
    if (status != HAL_OK) return status;
    
    // Combine high and low bytes
    raw_x = (int16_t)((data[1] << 8) | data[0]);
    raw_y = (int16_t)((data[3] << 8) | data[2]);
    raw_z = (int16_t)((data[5] << 8) | data[4]);
    
    // Convert to g (±2g range)
    // LSB sensitivity = 1mg/LSB = 0.001g/LSB
    accel_x = (float)raw_x * 0.001f;
    accel_y = (float)raw_y * 0.001f;
    accel_z = (float)raw_z * 0.001f;
    
    return HAL_OK;
}

void LSM303DLHC_Read_All(LSM303DLHC_t *DataStruct)
{
    uint8_t data[6];
    HAL_StatusTypeDef status;
    
    // 6 byte veriyi oku (X, Y, Z low ve high byte'ları)
    status = HAL_I2C_Mem_Read(&hi2c1, LSM303DLHC_ACCEL_ADDR, 
                             OUT_X_L_A | 0x80, 
                             I2C_MEMADD_SIZE_8BIT, data, 6, 1000);
    
    if (status != HAL_OK)
    {
        sprintf(debugMsg, "Accelerometer read error: %d\r\n", status);
        SendDebugMessage(debugMsg);
        return;
    }
    
    // Little endian format (düşük byte önce)
    DataStruct->x = (int16_t)(data[1] << 8 | data[0]);
    DataStruct->y = (int16_t)(data[3] << 8 | data[2]);
    DataStruct->z = (int16_t)(data[5] << 8 | data[4]);
    
    // ±2g için dönüşüm faktörü: 1mg/LSB
    DataStruct->x_g = (float)(DataStruct->x) * 0.001f;
    DataStruct->y_g = (float)(DataStruct->y) * 0.001f;
    DataStruct->z_g = (float)(DataStruct->z) * 0.001f;
    
    // Debug mesajları
    sprintf(debugMsg, "Raw: X=%d Y=%d Z=%d\r\n", DataStruct->x, DataStruct->y, DataStruct->z);
    SendDebugMessage(debugMsg);
    
    sprintf(debugMsg, "g: X=%.3f Y=%.3f Z=%.3f\r\n", DataStruct->x_g, DataStruct->y_g, DataStruct->z_g);
    SendDebugMessage(debugMsg);
}

uint8_t CalculateMotorSpeed(LSM303DLHC_t *accel)
{
    // Toplam linear acceleration hesapla
    double total_linear_acceleration = sqrt(pow(accel->x_g, 2) + pow(accel->y_g, 2) + pow(accel->z_g, 2));
    
    // 0-2g aralığını 0-100% motor hızına çevir
    uint8_t percent = (uint8_t)(total_linear_acceleration * 100.0 / 2.0);
    
    // Maximum %100 ile sınırla
    if (percent > 100) percent = 100;
    
    return percent;
} 