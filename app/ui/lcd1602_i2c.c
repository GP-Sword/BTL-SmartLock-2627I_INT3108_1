/**
 * @file    lcd1602_i2c.c
 * @brief   Driver LCD 16x2 điều khiển qua PCF8574 (I2C1, địa chỉ LCD_I2C_ADDR).
 */
#include "lcd1602_i2c.h"

#include "app_config.h"
#include "i2c.h"

#define LCD_PIN_RS    (1u << 0)
#define LCD_PIN_RW    (1u << 1)
#define LCD_PIN_EN    (1u << 2)
#define LCD_PIN_BL    (1u << 3)

static void lcd_write_nibble(uint8_t nibble, uint8_t rs)
{
    uint8_t base = (uint8_t)((nibble & 0xF0u) | LCD_PIN_BL | (rs ? LCD_PIN_RS : 0u));
    uint8_t buf[2];
    buf[0] = (uint8_t)(base | LCD_PIN_EN);
    buf[1] = base;
    (void)HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(LCD_I2C_ADDR << 1), buf, 2u, 10u);
}

static void lcd_send_byte(uint8_t val, uint8_t rs)
{
    uint8_t high_base = (uint8_t)((val & 0xF0u) | LCD_PIN_BL | (rs ? LCD_PIN_RS : 0u));
    uint8_t low_base  = (uint8_t)(((val << 4u) & 0xF0u) | LCD_PIN_BL | (rs ? LCD_PIN_RS : 0u));

    uint8_t buf[4];
    buf[0] = (uint8_t)(high_base | LCD_PIN_EN);
    buf[1] = high_base;
    buf[2] = (uint8_t)(low_base | LCD_PIN_EN);
    buf[3] = low_base;

    (void)HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(LCD_I2C_ADDR << 1), buf, 4u, 10u);
}

void lcd_init(void)
{
    HAL_Delay(50u); /* Chờ nguồn ổn định (> 40 ms) */

    /* Bật đèn nền, EN = 0 */
    uint8_t init_bl = LCD_PIN_BL;
    (void)HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(LCD_I2C_ADDR << 1), &init_bl, 1u, 10u);

    /* Chuỗi khởi tạo 4-bit của HD44780 */
    lcd_write_nibble(0x30u, 0u);
    HAL_Delay(5u);

    lcd_write_nibble(0x30u, 0u);
    HAL_Delay(1u);

    lcd_write_nibble(0x30u, 0u);
    HAL_Delay(1u);

    /* Chuyển sang giao thức 4-bit */
    lcd_write_nibble(0x20u, 0u);
    HAL_Delay(1u);

    /* Function Set: 4-bit, 2 dòng, font 5x8 (0x28) */
    lcd_cmd(0x28u);
    HAL_Delay(1u);

    /* Display ON, cursor OFF, blink OFF (0x0C) */
    lcd_cmd(0x0Cu);
    HAL_Delay(1u);

    /* Clear Display (0x01) */
    lcd_cmd(0x01u);
    HAL_Delay(2u);

    /* Entry Mode Set: tăng con trỏ, không dịch màn hình (0x06) */
    lcd_cmd(0x06u);
    HAL_Delay(1u);
}

void lcd_cmd(uint8_t cmd)
{
    lcd_send_byte(cmd, 0u);
}

void lcd_data(uint8_t data)
{
    lcd_send_byte(data, 1u);
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0u) ? col : (uint8_t)(0x40u + col);
    lcd_cmd((uint8_t)(0x80u | addr));
}

void lcd_print(uint8_t row, const char *str)
{
    lcd_set_cursor(row, 0u);
    for (uint8_t i = 0u; i < 16u && str[i] != '\0'; i++) {
        lcd_data((uint8_t)str[i]);
    }
}
