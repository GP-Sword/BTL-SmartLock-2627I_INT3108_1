/**
 * @file    queues.h
 * @brief   4 hàng đợi dùng chung + hàm gửi/nhận có in vết.
 *
 * Module KHÔNG gọi fifo_push/fifo_pop trực tiếp mà dùng các hàm Q_* dưới đây.
 * Khi bật APP_TRACE (preset debug/test) các hàm này in dòng vết ra UART,
 * nên khi thử riêng một module không cần tự viết stub in lệnh.
 *
 * Gọi được từ ISR: chỉ Q_SendInput (không in gì).
 */
#ifndef QUEUES_H
#define QUEUES_H

#include <stdbool.h>
#include <stdint.h>

#include "app_types.h"
#include "fifo.h"

/* Tên giữ nguyên như RTOS để sau này đổi sang xQueue* dễ dàng. */
extern Fifo_t xKeypadQueue;
extern Fifo_t xActuatorQueue;
extern Fifo_t xUIQueue;
extern Fifo_t xLogQueue;

void queues_init(void);

/* xKeypadQueue: Task 1 / Task 3 / test hook -> Task 2 */
bool Q_SendInput(InputEvt_t evt);              /* ISR-safe, không in vết           */
bool Q_RecvInput(InputEvt_t *evt);             /* in "KEY 5", "DOOR OPEN", ...     */

/* xActuatorQueue: Task 2 -> Task 3 */
bool Q_SendAct(ActCmd_t cmd);                  /* in "ACT:UNLOCK", ...             */
bool Q_RecvAct(ActCmd_t *cmd);

/* xUIQueue: Task 2 -> Task 4. value = pinLen (ENTER_PIN, ADMIN) hoặc số giây (LOCKED) */
bool Q_SendUI(UIScreen_t screen, uint16_t value); /* in "UI:LOCKED 57", ...        */
bool Q_RecvUI(UIMsg_t *msg);

/* xLogQueue: Task 2 -> Task 4 */
bool Q_SendLog(LogEvt_t evt);                  /* in "LOG:OK" / "LOG:FAIL"         */
bool Q_RecvLog(LogEvt_t *evt);

/* Tên dạng chuỗi (dùng cho vết và cho Task 4) */
const char *act_name(ActCmd_t cmd);
const char *ui_name(UIScreen_t screen);

#endif /* QUEUES_H */
