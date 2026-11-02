/**
 * @file    security.c
 * @brief   Task 2 - máy trạng thái bảo mật (UC4, UC6, UC7, UC8 hẹn giờ).
 *          Logic thuần: chỉ include app_time.h, queues.h, app_config.h, trace.h.
 */
#include "security.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_config.h"
#include "app_time.h"
#include "queues.h"
#include "trace.h"

static SysState_t state = STATE_IDLE;

static char    masterPin[PIN_MAX_LEN + 1u];
static char    buf[PIN_MAX_LEN + 1u];   /* PIN đang gõ (hoặc PIN mới ở ADMIN_RESET) */
static uint8_t bufLen;
static uint8_t failCount;
static bool    adminLogin;

static uint32_t lockoutSince;
static uint16_t lockoutShown;           /* số giây đã gửi lên UI gần nhất */

static bool     doorTimerOn;
static uint32_t doorSince;
static bool     doorAlarm;
static bool     alarmOn;

static void buf_clear(void)
{
    memset(buf, 0, sizeof buf);
    bufLen = 0u;
}

static void buf_add(char c)
{
    buf[bufLen++] = c;
    buf[bufLen] = '\0';
}

static const char *state_name(SysState_t s)
{
    switch (s) {
    case STATE_IDLE:         return "IDLE";
    case STATE_ENTERING_PIN: return "ENTERING_PIN";
    case STATE_UNLOCKED:     return "UNLOCKED";
    case STATE_LOCKED_OUT:   return "LOCKED_OUT";
    case STATE_ADMIN_RESET:  return "ADMIN_RESET";
    default:                 return "?";
    }
}

static void set_state(SysState_t s)
{
    if (s != state) {
        state = s;
        TRACE("STATE:%s", state_name(s));
    }
}

/* Vẽ lại màn hình ứng với trạng thái hiện tại (dùng khi đóng cửa). */
static void ui_redraw(void)
{
    switch (state) {
    case STATE_IDLE:         Q_SendUI(UI_IDLE, 0u); break;
    case STATE_ENTERING_PIN: Q_SendUI(UI_ENTER_PIN, bufLen); break;
    case STATE_UNLOCKED:     Q_SendUI(UI_GRANTED, 0u); break;
    case STATE_LOCKED_OUT:   Q_SendUI(UI_LOCKED, lockoutShown); break;
    case STATE_ADMIN_RESET:  Q_SendUI(UI_ADMIN, bufLen); break;
    default: break;
    }
}

static void go_idle(void)
{
    buf_clear();
    adminLogin = false;
    set_state(STATE_IDLE);
    Q_SendUI(UI_IDLE, 0u);
}

static void start_lockout(void)
{
    buf_clear();
    adminLogin = false;
    lockoutSince = app_now_ms();
    lockoutShown = (uint16_t)(LOCKOUT_MS / 1000u);
    set_state(STATE_LOCKED_OUT);
    Q_SendUI(UI_LOCKED, lockoutShown);
}

/* Một nơi duy nhất quyết định còi báo động. */
static void alarm_update(void)
{
    bool want = (state == STATE_LOCKED_OUT) || doorAlarm;
    if (want != alarmOn) {
        alarmOn = want;
        Q_SendAct(want ? ACT_ALARM_ON : ACT_ALARM_OFF);
    }
}

/* ---- Xử lý phím theo trạng thái ------------------------------------------ */

static void enter_digit(char c)
{
    if (bufLen >= PIN_MAX_LEN) {
        return;                         /* số thừa bỏ qua, không beep */
    }
    buf_add(c);
    Q_SendAct(ACT_BEEP_TICK);
    set_state(STATE_ENTERING_PIN);
    Q_SendUI(UI_ENTER_PIN, bufLen);
}

static void confirm_pin(void)
{
    bool match = (bufLen == strlen(masterPin)) && (memcmp(buf, masterPin, bufLen) == 0);
    bool admin = adminLogin;

    buf_clear();
    adminLogin = false;

    if (match) {
        failCount = 0u;
        if (admin) {
            set_state(STATE_ADMIN_RESET);
            Q_SendUI(UI_ADMIN, 0u);
        } else {
            Q_SendAct(ACT_UNLOCK);
            Q_SendUI(UI_GRANTED, 0u);
            Q_SendLog(LOG_UNLOCK_OK);
            set_state(STATE_UNLOCKED);
        }
        return;
    }

    failCount++;
    Q_SendLog(LOG_UNLOCK_FAIL);
    if (failCount == MAX_FAIL_COUNT) {
        start_lockout();
    } else {
        go_idle();
    }
}

static void on_key_idle(char k)
{
    if (k >= '0' && k <= '9') {
        enter_digit(k);
    } else if (k == 'A') {
        Q_SendAct(ACT_BEEP_TICK);
        adminLogin = true;
        buf_clear();
        set_state(STATE_ENTERING_PIN);
        Q_SendUI(UI_ENTER_PIN, 0u);
    }
}

static void on_key_entering(char k)
{
    if (k >= '0' && k <= '9') {
        enter_digit(k);
    } else if (k == '*' || (k == 'C' && adminLogin)) {
        Q_SendAct(ACT_BEEP_TICK);
        go_idle();
    } else if (k == '#') {
        Q_SendAct(ACT_BEEP_TICK);
        confirm_pin();
    }
}

static void on_key_admin(char k)
{
    if (k >= '0' && k <= '9') {
        if (bufLen < PIN_MAX_LEN) {
            buf_add(k);
            Q_SendAct(ACT_BEEP_TICK);
            Q_SendUI(UI_ADMIN, bufLen);
        }
    } else if (k == '*') {
        Q_SendAct(ACT_BEEP_TICK);
        buf_clear();
        Q_SendUI(UI_ADMIN, 0u);
    } else if (k == '#') {
        if (bufLen >= PIN_MIN_LEN) {
            Q_SendAct(ACT_BEEP_TICK);
            memcpy(masterPin, buf, bufLen + 1u);
            go_idle();
        }
    } else if (k == 'C') {
        Q_SendAct(ACT_BEEP_TICK);
        go_idle();
    }
}

static void on_key(char k)
{
    switch (state) {
    case STATE_IDLE:         on_key_idle(k); break;
    case STATE_ENTERING_PIN: on_key_entering(k); break;
    case STATE_ADMIN_RESET:  on_key_admin(k); break;
    case STATE_UNLOCKED:     break;     /* chờ EVT_RELOCKED */
    case STATE_LOCKED_OUT:   break;     /* bỏ qua mọi phím, không beep */
    default: break;
    }
}

/* ---- Sự kiện không phải phím --------------------------------------------- */

static void on_emergency(void)
{
    Q_SendLog(LOG_UNLOCK_OK);
    Q_SendUI(UI_GRANTED, 0u);
    if (state == STATE_LOCKED_OUT) {
        failCount = 0u;
    }
    buf_clear();
    adminLogin = false;
    set_state(STATE_UNLOCKED);
}

static void on_event(const InputEvt_t *e)
{
    switch (e->type) {
    case EVT_KEY:
        on_key(e->key);
        break;
    case EVT_DOOR_OPEN:
        doorTimerOn = true;
        doorSince = app_now_ms();
        break;
    case EVT_DOOR_CLOSED:
        doorTimerOn = false;
        doorAlarm = false;
        ui_redraw();
        break;
    case EVT_EMERGENCY:
        on_emergency();
        break;
    case EVT_RELOCKED:
        if (state == STATE_UNLOCKED) {
            go_idle();
        }
        break;
    default:
        break;
    }
}

/* ---- Mốc thời gian -------------------------------------------------------- */

static void check_timers(void)
{
    if (state == STATE_LOCKED_OUT) {
        uint32_t el = app_elapsed_ms(lockoutSince);
        if (el >= LOCKOUT_MS) {
            failCount = 0u;
            go_idle();
        } else {
            uint16_t left = (uint16_t)((LOCKOUT_MS - el + 999u) / 1000u);
            if (left != lockoutShown) {
                lockoutShown = left;
                Q_SendUI(UI_LOCKED, left);
            }
        }
    }

    if (doorTimerOn && !doorAlarm && app_elapsed_ms(doorSince) >= DOOR_AJAR_MS) {
        doorAlarm = true;
        Q_SendUI(UI_DOOR_AJAR, 0u);
    }
}

/* ---- API ------------------------------------------------------------------ */

void Security_Init(void)
{
    state = STATE_IDLE;
    memcpy(masterPin, PIN_MASTER_DEFAULT, sizeof PIN_MASTER_DEFAULT);
    buf_clear();
    failCount = 0u;
    adminLogin = false;
    lockoutShown = 0u;
    doorTimerOn = false;
    doorAlarm = false;
    alarmOn = false;
}

void Security_Process(void)
{
    InputEvt_t e;
    while (Q_RecvInput(&e)) {
        on_event(&e);
    }
    check_timers();
    alarm_update();
}

SysState_t Security_GetState(void)
{
    return state;
}
