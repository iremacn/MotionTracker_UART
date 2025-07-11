#include "motor.h"
#include "tim.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern volatile uint8_t motorSpeed;  // From main.h
extern char debugMsg[UART_BUFFER_SIZE];  // From main.h
extern uint8_t rxBuffer[RX_BUFFER_SIZE]; // From main.h

// Function declaration - defined in main.c
extern void SendDebugMessage(const char* msg);

void SetPWMDuty(uint8_t duty)
{
    if (duty > 100) duty = 100;
    
    // PWM duty cycle hesaplama (0-1000 arası)
    uint32_t pulse = (duty * htim3.Init.Period) / 100;
    
    // PWM değerini ayarla
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
    
    // Debug mesajı
    sprintf(debugMsg, "PWM Duty: %d%% (Pulse: %lu)\r\n", duty, pulse);
    SendDebugMessage(debugMsg);
}

void SetMotorSpeed(uint8_t speed)
{
    if (speed > 100) speed = 100;  // Limit speed to 100%
    
    // Update global variable
    motorSpeed = speed;
    
    // Update PWM duty cycle
    uint32_t pulse = (speed * htim3.Init.Period) / 100;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
    
    // Debug bilgisi
    sprintf(debugMsg, "HW-153 Motor: PWM=%d (%d%%)\r\n", (int)pulse, speed);
    SendDebugMessage(debugMsg);
}

/* HW-153 Motor Driver - Yön ve Hız Kontrolü */
void HW153_SetMotor(uint8_t speed, uint8_t direction)
{
    if (speed > 100) speed = 100;
    
    // HW-153 V1 Motor Driver Kontrol Tablosu:
    // INA (PA6/PWM)  | INB (PA7)  | Motor Durumu
    // PWM            | LOW        | İleri yön (CW)
    // LOW            | PWM        | Geri yön (CCW)  
    // PWM            | PWM        | Fren
    // LOW            | LOW        | Serbest (Coast)
    
    if (speed == 0) {
        // Motor durdur - Serbest bırak (Coast)
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);  // PA6 = LOW
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // PA7 = LOW
        sprintf(debugMsg, "HW-153: Motor DURDURULDU\r\n");
    }
    else if (direction == MOTOR_DIRECTION_FORWARD) {
        // İleri yön: INA=PWM, INB=LOW
        uint32_t pulse = (speed * htim3.Init.Period) / 100;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);  // PA6 = PWM
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // PA7 = LOW
        sprintf(debugMsg, "HW-153: İLERİ Yön, Hız: %d%%\r\n", speed);
    }
    else if (direction == MOTOR_DIRECTION_BACKWARD) {
        // Geri yön: INA=LOW, INB=HIGH (PWM yerine digital high kullanıyoruz)
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);  // PA6 = LOW
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);   // PA7 = HIGH
        sprintf(debugMsg, "HW-153: GERİ Yön, Hız: %d%%\r\n", speed);
    }
    
    SendDebugMessage(debugMsg);
}

void ProcessCommand(void)
{
    // "Dxx" formatında komut (xx = hız yüzdesi)
    if (rxBuffer[0] == 'D')
    {
        int speed = atoi((char*)&rxBuffer[1]);
        if (speed >= 0 && speed <= 100)
        {
            SetPWMDuty(speed);
            sprintf(debugMsg, "Motor speed set to %d%%\r\n", speed);
            SendDebugMessage(debugMsg);
        }
    }
    
    // UART'ı yeni komut için hazırla
    HAL_UART_Receive_IT(&huart2, rxBuffer, 1);
}

void Motor_Init(void)
{
    // Timer ve PWM zaten main.c'de başlatılıyor
    // İlk başta motoru durdur
    SetMotorSpeed(0);
    SendDebugMessage("Motor: Initialized and stopped\r\n");
}

/* Motor Test Fonksiyonu */
void Motor_Test(void)
{
    SendDebugMessage("\r\n=== MOTOR PWM TEST BAŞLADI ===\r\n");
    
    // TIM3 PWM başlatma kontrolü
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK) {
        SendDebugMessage("HATA: PWM başlatılamadı!\r\n");
        return;
    }
    
    // 0%'dan 100%'e kadar test
    for (uint8_t duty = 0; duty <= 100; duty += 10) {
        sprintf(debugMsg, "PWM Test: %d%% duty cycle\r\n", duty);
        SendDebugMessage(debugMsg);
        
        // PWM değerini hesapla ve ayarla
        uint32_t pulse_value = (duty * htim3.Init.Period) / 100;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse_value);
        
        sprintf(debugMsg, "Period: %lu, Pulse: %lu, Duty: %d%%\r\n", 
                htim3.Init.Period, pulse_value, duty);
        SendDebugMessage(debugMsg);
        
        HAL_Delay(2000); // Her adımda 2 saniye bekle
    }
    
    // Motoru durdur
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    SendDebugMessage("=== MOTOR PWM TEST BİTTİ ===\r\n");
}

/* PA6 ve PA7 Pin Test - HW-153 için GPIO kontrolü */
void Motor_PinTest(void)
{
    SendDebugMessage("\r\n=== HW-153 PA6 & PA7 PIN TEST ===\r\n");
    
    // PA6 ve PA7'yi GPIO output yap
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Her iki pini test et
    for(int i = 0; i < 5; i++) {
        // PA6 HIGH, PA7 LOW
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
        SendDebugMessage("PA6=HIGH, PA7=LOW\r\n");
        HAL_Delay(1000);
        
        // PA6 LOW, PA7 HIGH  
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
        SendDebugMessage("PA6=LOW, PA7=HIGH\r\n");
        HAL_Delay(1000);
        
        // Her ikisi LOW
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
        SendDebugMessage("PA6=LOW, PA7=LOW\r\n");
        HAL_Delay(1000);
        
        // Her ikisi HIGH
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
        SendDebugMessage("PA6=HIGH, PA7=HIGH\r\n");
        HAL_Delay(1000);
    }
    
    // PA6'yı PWM, PA7'yi GPIO output yap (normal HW-153 config)
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    SendDebugMessage("=== HW-153 PIN TEST BİTTİ ===\r\n");
}

/* HW-153 V1 Motor Driver Test Fonksiyonu */
void HW153_MotorTest(void)
{
    SendDebugMessage("\r\n=== HW-153 V1 MOTOR DRIVER TEST ===\r\n");
    
    // Test 1: İleri yön - Artan hız
    SendDebugMessage("Test 1: İleri yön - 0%->100%\r\n");
    for(uint8_t speed = 0; speed <= 100; speed += 25) {
        HW153_SetMotor(speed, MOTOR_DIRECTION_FORWARD);
        HAL_Delay(2000);
    }
    
    // Motor durdur
    HW153_SetMotor(0, MOTOR_DIRECTION_FORWARD);
    HAL_Delay(1000);
    
    // Test 2: Geri yön - Sabit hız
    SendDebugMessage("Test 2: Geri yön - 50% hız\r\n");
    HW153_SetMotor(50, MOTOR_DIRECTION_BACKWARD);
    HAL_Delay(3000);
    
    // Motor durdur
    HW153_SetMotor(0, MOTOR_DIRECTION_FORWARD);
    HAL_Delay(1000);
    
    // Test 3: Yön değiştirme testi
    SendDebugMessage("Test 3: Yön değiştirme testi\r\n");
    for(int i = 0; i < 3; i++) {
        sprintf(debugMsg, "İleri - Adım %d\r\n", i+1);
        SendDebugMessage(debugMsg);
        HW153_SetMotor(40, MOTOR_DIRECTION_FORWARD);
        HAL_Delay(2000);
        
        HW153_SetMotor(0, MOTOR_DIRECTION_FORWARD);  // Durdur
        HAL_Delay(500);
        
        sprintf(debugMsg, "Geri - Adım %d\r\n", i+1);
        SendDebugMessage(debugMsg);
        HW153_SetMotor(40, MOTOR_DIRECTION_BACKWARD);
        HAL_Delay(2000);
        
        HW153_SetMotor(0, MOTOR_DIRECTION_FORWARD);  // Durdur
        HAL_Delay(500);
    }
    
    SendDebugMessage("=== HW-153 MOTOR TEST TAMAMLANDI ===\r\n");
}

/* Motor Basit Test - Hızlı debugging için */
void Motor_SimpleTest(void)
{
    SendDebugMessage("\r\n=== MOTOR DIAGNOSTIC TEST ===\r\n");
    
    // TIM3 durumu kontrol et
    sprintf(debugMsg, "TIM3 CR1: 0x%08lX\r\n", TIM3->CR1);
    SendDebugMessage(debugMsg);
    
    sprintf(debugMsg, "TIM3 ARR: %lu, PSC: %lu\r\n", TIM3->ARR, TIM3->PSC);
    SendDebugMessage(debugMsg);
    
    sprintf(debugMsg, "TIM3 CCR1: %lu, CCMR1: 0x%08lX\r\n", TIM3->CCR1, TIM3->CCMR1);
    SendDebugMessage(debugMsg);
    
    // PWM'i tekrar başlat
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    
    SendDebugMessage("Test 1: 25% PWM - 3 saniye\r\n");
    TIM3->CCR1 = 250;  // 25%
    HAL_Delay(3000);
    
    SendDebugMessage("Test 2: 50% PWM - 3 saniye\r\n");
    TIM3->CCR1 = 500;  // 50%
    HAL_Delay(3000);
    
    SendDebugMessage("Test 3: 75% PWM - 3 saniye\r\n");
    TIM3->CCR1 = 750;  // 75%
    HAL_Delay(3000);
    
    SendDebugMessage("Test 4: 100% PWM - 3 saniye\r\n");
    TIM3->CCR1 = 999;  // 100%
    HAL_Delay(3000);
    
    // PWM'i durdur
    TIM3->CCR1 = 0;
    SendDebugMessage("=== MOTOR TEST TAMAMLANDI ===\r\n");
    
    sprintf(debugMsg, "Final TIM3 CCR1: %lu\r\n", TIM3->CCR1);
    SendDebugMessage(debugMsg);
}

/* Motor Döndürme Fonksiyonları */

void Motor_RotateClockwise(uint8_t speed, uint32_t duration_ms)
{
    sprintf(debugMsg, "HW-153: Saat yönünde döndürülüyor - Hız: %d%%, Süre: %lums\r\n", speed, duration_ms);
    SendDebugMessage(debugMsg);
    
    HW153_SetMotor(speed, MOTOR_DIRECTION_FORWARD);  // İleri yön
    HAL_Delay(duration_ms);
    HW153_SetMotor(0, MOTOR_DIRECTION_FORWARD);      // Motor durdur
    
    SendDebugMessage("HW-153: Saat yönünde döndürme tamamlandı\r\n");
}

void Motor_RotateCounterClockwise(uint8_t speed, uint32_t duration_ms)
{
    sprintf(debugMsg, "HW-153: Saat yönünün tersine döndürülüyor - Hız: %d%%, Süre: %lums\r\n", speed, duration_ms);
    SendDebugMessage(debugMsg);
    
    HW153_SetMotor(speed, MOTOR_DIRECTION_BACKWARD);  // Geri yön
    HAL_Delay(duration_ms);
    HW153_SetMotor(0, MOTOR_DIRECTION_FORWARD);       // Motor durdur
    
    SendDebugMessage("HW-153: Saat yönünün tersine döndürme tamamlandı\r\n");
}

void Motor_RotateContinuous(uint8_t speed)
{
    sprintf(debugMsg, "Motor: Sürekli döndürme başlatıldı - Hız: %d%%\r\n", speed);
    SendDebugMessage(debugMsg);
    
    SetMotorSpeed(speed);
}

void Motor_Stop(void)
{
    SendDebugMessage("Motor: Durduruldu\r\n");
    SetMotorSpeed(0);
}

void Motor_SpeedRamp(uint8_t start_speed, uint8_t end_speed, uint32_t ramp_time_ms)
{
    sprintf(debugMsg, "Motor: Hız rampa - %d%% -> %d%%, Süre: %lums\r\n", 
            start_speed, end_speed, ramp_time_ms);
    SendDebugMessage(debugMsg);
    
    int32_t speed_diff = end_speed - start_speed;
    uint32_t step_delay = ramp_time_ms / 100; // 100 adımda ramp
    
    for (int i = 0; i <= 100; i++)
    {
        uint8_t current_speed = start_speed + (speed_diff * i) / 100;
        SetMotorSpeed(current_speed);
        HAL_Delay(step_delay);
    }
    
    sprintf(debugMsg, "Motor: Hız rampa tamamlandı - Son hız: %d%%\r\n", end_speed);
    SendDebugMessage(debugMsg);
}

void Motor_Pulse(uint8_t speed, uint32_t on_time_ms, uint32_t off_time_ms, uint8_t pulse_count)
{
    sprintf(debugMsg, "Motor: Pulse modu - Hız: %d%%, On: %lums, Off: %lums, Sayı: %d\r\n", 
            speed, on_time_ms, off_time_ms, pulse_count);
    SendDebugMessage(debugMsg);
    
    for (uint8_t i = 0; i < pulse_count; i++)
    {
        sprintf(debugMsg, "Pulse %d/%d\r\n", i+1, pulse_count);
        SendDebugMessage(debugMsg);
        
        SetMotorSpeed(speed);      // Motor aç
        HAL_Delay(on_time_ms);
        
        SetMotorSpeed(0);          // Motor kapat
        HAL_Delay(off_time_ms);
    }
    
    SendDebugMessage("Motor: Pulse modu tamamlandı\r\n");
} 