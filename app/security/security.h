/**
 * @file    security.h
 * @brief   Task 2 - Logic bảo mật & máy trạng thái (UC4, UC6, UC7, UC8 timer).
 *          OWNER: Task 2. Logic thuần C: KHÔNG include HAL, thời gian lấy từ app_now_ms(),
 *          nhờ vậy unit test được trên PC (tests/host).
 */
#ifndef SECURITY_H
#define SECURITY_H

typedef enum {
    STATE_IDLE = 0,
    STATE_ENTERING_PIN,
    STATE_UNLOCKED,
    STATE_LOCKED_OUT,
    STATE_ADMIN_RESET
} SysState_t;

void       Security_Init(void);
void       Security_Process(void);   /* lấy hết sự kiện trong xKeypadQueue + kiểm tra mốc thời gian */
SysState_t Security_GetState(void);  /* cho unit test và debug */

#endif /* SECURITY_H */
