/**
 * @file    ui.c
 * @brief   STUB của foundation: lấy tin khỏi xUIQueue (Q_SendUI đã in vết "UI:...").
 *          Task 4 thay ruột: driver PCF8574 (LCD_I2C_ADDR), 6 màn hình, chỉ ghi ô thay đổi.
 */
#include "ui.h"

#include "app_config.h"
#include "board_pins.h"
#include "queues.h"

void UI_Init(void)
{
    /* TODO(Task 4): lcd_init(); vẽ màn IDLE */
}

void UI_Process(void)
{
    UIMsg_t msg;
    while (Q_RecvUI(&msg)) {
        /* TODO(Task 4): cập nhật bộ đệm want[2][16] */
    }
}
