/**
 * @file    ui.h
 * @brief   Task 4 - Màn hình LCD 16x2 I2C (UC3). OWNER: Task 4.
 *          Chỉ module này dùng hi2c1.
 */
#ifndef UI_H
#define UI_H

void UI_Init(void);     /* lcd_init(): được phép HAL_Delay (chạy trước vòng lặp) */
void UI_Process(void);  /* mỗi lần gọi gửi tối đa vài ký tự, chỉ ghi ô thay đổi  */

#endif /* UI_H */
