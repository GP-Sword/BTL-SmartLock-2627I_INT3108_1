/**
 * @file    board_pins.h
 * @brief   Pin map duy nhất của dự án. OWNER: foundation.
 *
 * Mọi macro *_Pin / *_GPIO_Port do CubeMX sinh từ User Label trong smart_lock.ioc
 * (xem Core/Inc/main.h). Module chỉ dùng tên ở đây, không gõ tay GPIO_PIN_x.
 * Đổi chân = sửa .ioc + diagram.json + file này trong một PR `iface/...`.
 *
 *   Chân      Chức năng                Task
 *   PA0-PA3   Hàng keypad (open-drain) 1
 *   PA4-PA7   Cột keypad (pull-up)     1
 *   PB10      Công tắc cửa, LOW=đóng   1
 *   PB11      Nút khẩn cấp, EXTI fall  1
 *   PA8       LED xanh (mở khóa)       3
 *   PB0       Servo, TIM3_CH3 50 Hz    3
 *   PB1       Buzzer (TIM2 4 kHz)      3
 *   PB6/PB7   I2C1 SCL/SDA, LCD 0x27   4
 *   PA9/PA10  USART1 TX/RX 115200      4
 *   PC13      LED onboard (nhịp tim)   foundation
 */
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "main.h"

/* Keypad 4x4: dùng như
 *   static GPIO_TypeDef *const rowPort[4] = BOARD_KP_ROW_PORTS;
 *   static const uint16_t       rowPin[4] = BOARD_KP_ROW_PINS;            */
#define BOARD_KP_ROW_PORTS { KP_R1_GPIO_Port, KP_R2_GPIO_Port, KP_R3_GPIO_Port, KP_R4_GPIO_Port }
#define BOARD_KP_ROW_PINS  { KP_R1_Pin, KP_R2_Pin, KP_R3_Pin, KP_R4_Pin }
#define BOARD_KP_COL_PORTS { KP_C1_GPIO_Port, KP_C2_GPIO_Port, KP_C3_GPIO_Port, KP_C4_GPIO_Port }
#define BOARD_KP_COL_PINS  { KP_C1_Pin, KP_C2_Pin, KP_C3_Pin, KP_C4_Pin }

#define BOARD_KEYMAP {              \
    { '1', '2', '3', 'A' },         \
    { '4', '5', '6', 'B' },         \
    { '7', '8', '9', 'C' },         \
    { '*', '0', '#', 'D' } }

/* Công tắc cửa: LOW = đóng (nam châm áp sát / công tắc Wokwi nối GND) */
#define BOARD_DOOR_CLOSED_LEVEL  GPIO_PIN_RESET

/* LED onboard PC13 sáng khi LOW */
#define BOARD_LED_ON_LEVEL       GPIO_PIN_RESET

/* Handle ngoại vi do CubeMX sinh */
#include "i2c.h"   /* hi2c1 - chỉ Task 4 dùng            */
#include "tim.h"   /* htim3 servo, htim2 âm buzzer (Task 3) */
#include "usart.h" /* huart1 - chỉ uart_port.c (Task 4)   */

#endif /* BOARD_PINS_H */
