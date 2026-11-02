/**
 * @file    lcd1602_i2c.h
 * @brief   Driver LCD 16x2 I2C dùng module PCF8574 ở chế độ 4-bit (Task 4 sở hữu).
 */
#ifndef LCD1602_I2C_H
#define LCD1602_I2C_H

#include <stdint.h>

void lcd_init(void);
void lcd_cmd(uint8_t cmd);
void lcd_data(uint8_t data);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print(uint8_t row, const char *str);

#endif /* LCD1602_I2C_H */
