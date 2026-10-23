/**
 * @file    app_types.h
 * @brief   Hợp đồng dữ liệu giữa 4 task: kiểu tin trong hàng đợi và cờ dùng chung.
 *
 * OWNER: foundation. Sửa file này = đổi giao diện của cả nhóm:
 * chỉ qua PR `iface/...`, cần cả 4 người approve.
 *
 * Luồng dữ liệu:
 *   xKeypadQueue   Task 1 (+ Task 3, test hook) -> Task 2   InputEvt_t
 *   xActuatorQueue Task 2 -> Task 3                         ActCmd_t
 *   xUIQueue       Task 2 -> Task 4                         UIMsg_t
 *   xLogQueue      Task 2 -> Task 4                         LogEvt_t
 *   g_emergencyReq Task 1 (ISR) -> Task 3                   cờ, không qua hàng đợi
 *   g_doorOpen     Task 1 -> Task 3                         cờ chỉ đọc
 */
#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>

/* ---- xKeypadQueue -------------------------------------------------------- */
typedef enum {
    EVT_KEY = 0,        /* key = '0'..'9', 'A'..'D', '*', '#'          */
    EVT_DOOR_OPEN,
    EVT_DOOR_CLOSED,
    EVT_EMERGENCY,      /* nút khẩn cấp PB11 (đã chống rung)            */
    EVT_RELOCKED        /* Task 3 báo đã khóa lại (tự khóa hoặc LOCK)  */
} InputEvtType_t;

typedef struct {
    InputEvtType_t type;
    char           key; /* chỉ dùng khi type == EVT_KEY                */
} InputEvt_t;

/* ---- xActuatorQueue ------------------------------------------------------ */
typedef enum {
    ACT_UNLOCK = 0,
    ACT_LOCK,
    ACT_BEEP_TICK,
    ACT_ALARM_ON,
    ACT_ALARM_OFF
} ActCmd_t;

/* ---- xUIQueue ------------------------------------------------------------ */
typedef enum {
    UI_IDLE = 0,
    UI_ENTER_PIN,       /* dùng pinLen  */
    UI_GRANTED,
    UI_LOCKED,          /* dùng seconds */
    UI_DOOR_AJAR,
    UI_ADMIN            /* dùng pinLen (số chữ số PIN mới) */
} UIScreen_t;

typedef struct {
    UIScreen_t screen;
    uint8_t    pinLen;  /* Task 4 chỉ nhận số chữ số, KHÔNG BAO GIỜ nhận giá trị PIN */
    uint16_t   seconds;
} UIMsg_t;

/* ---- xLogQueue ----------------------------------------------------------- */
typedef enum {
    LOG_UNLOCK_OK = 0,
    LOG_UNLOCK_FAIL
} LogEvt_t;

/* ---- Cờ dùng chung (định nghĩa trong queues.c) --------------------------- */
extern volatile uint8_t g_emergencyReq; /* Task 1 (ISR) ghi 1, Task 3 đọc rồi xóa      */
extern volatile uint8_t g_doorOpen;     /* Task 1 ghi (đã chống rung, 1 = mở), Task 3 đọc */

/* ---- Chuỗi vết (APP_TRACE) là một phần của hợp đồng ------------------------
 * Scenario test trong tests/scenarios bắt theo đúng các chuỗi này:
 *   Q_RecvInput : "KEY 5", "DOOR OPEN", "DOOR CLOSED", "EMERGENCY", "RELOCKED"
 *   Q_SendAct   : "ACT:UNLOCK", "ACT:LOCK", "ACT:BEEP_TICK", "ACT:ALARM_ON", "ACT:ALARM_OFF"
 *   Q_SendUI    : "UI:IDLE", "UI:ENTER_PIN 3", "UI:GRANTED", "UI:LOCKED 57",
 *                 "UI:DOOR_AJAR", "UI:ADMIN 2"
 *   Q_SendLog   : "LOG:OK", "LOG:FAIL"
 *   Task 2      : "STATE:IDLE", "STATE:ENTERING_PIN", "STATE:UNLOCKED",
 *                 "STATE:LOCKED_OUT", "STATE:ADMIN_RESET"
 *   Task 4 (log thật, luôn bật): "[YYYY-MM-DD HH:MM:SS] UNLOCK_OK" / "UNLOCK_FAIL"
 * ------------------------------------------------------------------------- */

#endif /* APP_TYPES_H */
