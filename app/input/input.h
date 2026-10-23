/**
 * @file    input.h
 * @brief   Task 1 - Khối nhập liệu & cảm biến (UC1, UC8 phần đọc, UC9 phần ngắt).
 *          OWNER: Task 1. Giao diện (tên hàm) do foundation chốt.
 */
#ifndef INPUT_H
#define INPUT_H

void Input_Init(void);
void Input_Process(void);       /* gọi mỗi vòng lặp; tự chia nhịp KEYPAD_SCAN_MS */
void Input_OnEmergencyIrq(void); /* gọi từ HAL_GPIO_EXTI_Callback (trong ngắt)    */

#endif /* INPUT_H */
