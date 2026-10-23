/**
 * @file    queues.c
 * @brief   Bộ nhớ của 4 hàng đợi, cờ dùng chung và hàm Q_* có vết. OWNER: foundation.
 */
#include "queues.h"

#include "app_config.h"
#include "trace.h"

/* ---- Cờ dùng chung (khai báo trong app_types.h) -------------------------- */
volatile uint8_t g_emergencyReq = 0u;
volatile uint8_t g_doorOpen     = 0u;

/* ---- Hàng đợi ------------------------------------------------------------ */
Fifo_t xKeypadQueue;
Fifo_t xActuatorQueue;
Fifo_t xUIQueue;
Fifo_t xLogQueue;

static InputEvt_t s_keypadBuf[Q_KEYPAD_LEN];
static ActCmd_t   s_actBuf[Q_ACTUATOR_LEN];
static UIMsg_t    s_uiBuf[Q_UI_LEN];
static LogEvt_t   s_logBuf[Q_LOG_LEN];

void queues_init(void)
{
    fifo_init(&xKeypadQueue,   s_keypadBuf, sizeof(InputEvt_t), Q_KEYPAD_LEN);
    fifo_init(&xActuatorQueue, s_actBuf,    sizeof(ActCmd_t),   Q_ACTUATOR_LEN);
    fifo_init(&xUIQueue,       s_uiBuf,     sizeof(UIMsg_t),    Q_UI_LEN);
    fifo_init(&xLogQueue,      s_logBuf,    sizeof(LogEvt_t),   Q_LOG_LEN);
    g_emergencyReq = 0u;
    g_doorOpen     = 0u;
}

/* ---- Tên chuỗi ----------------------------------------------------------- */
const char *act_name(ActCmd_t cmd)
{
    switch (cmd) {
    case ACT_UNLOCK:    return "UNLOCK";
    case ACT_LOCK:      return "LOCK";
    case ACT_BEEP_TICK: return "BEEP_TICK";
    case ACT_ALARM_ON:  return "ALARM_ON";
    case ACT_ALARM_OFF: return "ALARM_OFF";
    default:            return "?";
    }
}

const char *ui_name(UIScreen_t screen)
{
    switch (screen) {
    case UI_IDLE:      return "IDLE";
    case UI_ENTER_PIN: return "ENTER_PIN";
    case UI_GRANTED:   return "GRANTED";
    case UI_LOCKED:    return "LOCKED";
    case UI_DOOR_AJAR: return "DOOR_AJAR";
    case UI_ADMIN:     return "ADMIN";
    default:           return "?";
    }
}

/* ---- xKeypadQueue -------------------------------------------------------- */
bool Q_SendInput(InputEvt_t evt)
{
    return fifo_push(&xKeypadQueue, &evt);
}

bool Q_RecvInput(InputEvt_t *evt)
{
    if (!fifo_pop(&xKeypadQueue, evt)) {
        return false;
    }
    switch (evt->type) {
    case EVT_KEY:         TRACE("KEY %c", evt->key); break;
    case EVT_DOOR_OPEN:   TRACE("DOOR OPEN");        break;
    case EVT_DOOR_CLOSED: TRACE("DOOR CLOSED");      break;
    case EVT_EMERGENCY:   TRACE("EMERGENCY");        break;
    case EVT_RELOCKED:    TRACE("RELOCKED");         break;
    default:              TRACE("EVT %d", (int)evt->type); break;
    }
    return true;
}

/* ---- xActuatorQueue ------------------------------------------------------ */
bool Q_SendAct(ActCmd_t cmd)
{
    TRACE("ACT:%s", act_name(cmd));
    return fifo_push(&xActuatorQueue, &cmd);
}

bool Q_RecvAct(ActCmd_t *cmd)
{
    return fifo_pop(&xActuatorQueue, cmd);
}

/* ---- xUIQueue ------------------------------------------------------------ */
bool Q_SendUI(UIScreen_t screen, uint16_t value)
{
    UIMsg_t msg = { .screen = screen, .pinLen = 0u, .seconds = 0u };

    switch (screen) {
    case UI_ENTER_PIN:
    case UI_ADMIN:
        msg.pinLen = (uint8_t)value;
        TRACE("UI:%s %u", ui_name(screen), (unsigned)value);
        break;
    case UI_LOCKED:
        msg.seconds = value;
        TRACE("UI:%s %u", ui_name(screen), (unsigned)value);
        break;
    default:
        TRACE("UI:%s", ui_name(screen));
        break;
    }
    return fifo_push(&xUIQueue, &msg);
}

bool Q_RecvUI(UIMsg_t *msg)
{
    return fifo_pop(&xUIQueue, msg);
}

/* ---- xLogQueue ----------------------------------------------------------- */
bool Q_SendLog(LogEvt_t evt)
{
    TRACE("LOG:%s", (evt == LOG_UNLOCK_OK) ? "OK" : "FAIL");
    return fifo_push(&xLogQueue, &evt);
}

bool Q_RecvLog(LogEvt_t *evt)
{
    return fifo_pop(&xLogQueue, evt);
}
