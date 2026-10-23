/**
 * @file    security.c
 * @brief   STUB của foundation: chỉ lấy sự kiện ra khỏi hàng đợi (hàm Q_RecvInput
 *          tự in vết "KEY 5", "DOOR OPEN"...). Task 2 thay ruột file này.
 *
 * Việc cần làm (guide mục 7, Task 2): máy trạng thái 5 trạng thái, so khớp PIN,
 * khóa LOCKOUT_MS sau MAX_FAIL_COUNT lần sai, đổi PIN chủ, hẹn giờ cửa DOOR_AJAR_MS,
 * alarm_update() duy nhất quyết định ALARM_ON/OFF, TRACE("STATE:...") khi đổi trạng thái.
 * Chỉ include app_time.h / queues.h / app_config.h / trace.h - KHÔNG include HAL.
 */
#include "security.h"

#include "app_config.h"
#include "app_time.h"
#include "queues.h"
#include "trace.h"

static SysState_t s_state = STATE_IDLE;

void Security_Init(void)
{
    s_state = STATE_IDLE;
}

void Security_Process(void)
{
    InputEvt_t evt;
    while (Q_RecvInput(&evt)) {
        /* TODO(Task 2): Security_HandleEvent(&evt); */
    }
    /* TODO(Task 2): Security_Tick(app_now_ms()); */
}

SysState_t Security_GetState(void)
{
    return s_state;
}
