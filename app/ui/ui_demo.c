/**
 * @file    ui_demo.c
 * @brief   Chế độ thử riêng Task 4: mỗi giây đẩy một tin vào xUIQueue / xLogQueue.
 *          Bọc toàn bộ trong #ifdef UI_DEMO để không ảnh hưởng firmware chính.
 */
#ifdef UI_DEMO

#include "ui_demo.h"

#include "app_time.h"
#include "queues.h"

static uint32_t s_lastStepMs = 0u;
static uint8_t  s_step = 0u;

void ui_demo_init(void)
{
    s_lastStepMs = app_now_ms();
    s_step = 0u;
}

void ui_demo_process(void)
{
    if (app_elapsed_ms(s_lastStepMs) < 1000u) {
        return;
    }
    s_lastStepMs = app_now_ms();

    switch (s_step) {
    case 0:
        Q_SendUI(UI_ENTER_PIN, 1u);
        break;
    case 1:
        Q_SendUI(UI_ENTER_PIN, 2u);
        break;
    case 2:
        Q_SendUI(UI_ENTER_PIN, 3u);
        break;
    case 3:
        Q_SendUI(UI_ENTER_PIN, 4u);
        break;
    case 4:
        Q_SendUI(UI_GRANTED, 0u);
        (void)Q_SendLog(LOG_UNLOCK_OK);
        break;
    case 5:
        (void)Q_SendLog(LOG_UNLOCK_FAIL);
        break;
    case 6:
        Q_SendUI(UI_LOCKED, 60u);
        break;
    case 7:
        Q_SendUI(UI_LOCKED, 59u);
        break;
    case 8:
        Q_SendUI(UI_DOOR_AJAR, 0u);
        break;
    case 9:
        Q_SendUI(UI_ADMIN, 2u);
        break;
    case 10:
    default:
        Q_SendUI(UI_IDLE, 0u);
        s_step = 0u;
        return;
    }

    s_step++;
}

#else

typedef int ui_demo_unused_t;

#endif /* UI_DEMO */
