/**
 * @file    ui.c
 * @brief   Task 4 - Màn hình LCD 16x2 I2C (UC3).
 *          Quản lý 6 màn hình, cập nhật vi sai (chỉ ghi ô thay đổi, không nhấp nháy).
 */
#include "ui.h"

#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "lcd1602_i2c.h"
#include "queues.h"

#ifdef UI_DEMO
#include "ui_demo.h"
#endif

static char s_shown[2][17];
static char s_want[2][17];

static void format_screen(const UIMsg_t *msg)
{
    switch (msg->screen) {
    case UI_IDLE:
        memcpy(s_want[0], "SMART DOOR LOCK ", 16u);
        memcpy(s_want[1], "ENTER PIN       ", 16u);
        break;

    case UI_ENTER_PIN:
        memcpy(s_want[0], "ENTER PIN:      ", 16u);
        memset(s_want[1], ' ', 16u);
        {
            uint8_t n = (msg->pinLen > 16u) ? 16u : msg->pinLen;
            for (uint8_t i = 0u; i < n; i++) {
                s_want[1][i] = '*';
            }
        }
        break;

    case UI_GRANTED:
        memcpy(s_want[0], "ACCESS GRANTED  ", 16u);
        memcpy(s_want[1], "                ", 16u);
        break;

    case UI_LOCKED: {
        char buf[17];
        (void)snprintf(buf, sizeof(buf), "LOCKED %us", msg->seconds);
        size_t len = strlen(buf);
        while (len < 16u) {
            buf[len++] = ' ';
        }
        buf[16] = '\0';
        memcpy(s_want[0], buf, 16u);
        memcpy(s_want[1], "                ", 16u);
        break;
    }

    case UI_DOOR_AJAR:
        memcpy(s_want[0], "DOOR AJAR       ", 16u);
        memcpy(s_want[1], "                ", 16u);
        break;

    case UI_ADMIN:
        memcpy(s_want[0], "ADMIN MODE      ", 16u);
        memset(s_want[1], ' ', 16u);
        {
            uint8_t n = (msg->pinLen > 16u) ? 16u : msg->pinLen;
            for (uint8_t i = 0u; i < n; i++) {
                s_want[1][i] = '*';
            }
        }
        break;

    default:
        break;
    }

    s_want[0][16] = '\0';
    s_want[1][16] = '\0';
}

void UI_Init(void)
{
    lcd_init();

    memcpy(s_want[0], "SMART DOOR LOCK ", 16u);
    memcpy(s_want[1], "ENTER PIN       ", 16u);
    s_want[0][16] = '\0';
    s_want[1][16] = '\0';

    lcd_print(0u, s_want[0]);
    lcd_print(1u, s_want[1]);

    memcpy(s_shown[0], s_want[0], 17u);
    memcpy(s_shown[1], s_want[1], 17u);

#ifdef UI_DEMO
    ui_demo_init();
#endif
}

void UI_Process(void)
{
#ifdef UI_DEMO
    ui_demo_process();
#endif

    UIMsg_t msg;
    while (Q_RecvUI(&msg)) {
        format_screen(&msg);
    }

    /* Quét cập nhật vi sai: gửi tối đa 2 ký tự khác nhau mỗi lần để không giữ I2C lâu */
    for (uint8_t r = 0u; r < 2u; r++) {
        for (uint8_t c = 0u; c < 16u; c++) {
            if (s_shown[r][c] != s_want[r][c]) {
                lcd_set_cursor(r, c);
                lcd_data((uint8_t)s_want[r][c]);
                s_shown[r][c] = s_want[r][c];

                if (c + 1u < 16u && s_shown[r][c + 1u] != s_want[r][c + 1u]) {
                    lcd_data((uint8_t)s_want[r][c + 1u]);
                    s_shown[r][c + 1u] = s_want[r][c + 1u];
                }
                return;
            }
        }
    }
}
