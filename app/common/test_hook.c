/**
 * @file    test_hook.c
 * @brief   Xem test_hook.h. OWNER: foundation.
 */
#ifdef APP_TEST_HOOKS

#include "test_hook.h"

#include "queues.h"
#include "trace.h"
#include "uart_port.h"

static void send_key(char k)
{
    InputEvt_t e = { .type = EVT_KEY, .key = k };
    (void)Q_SendInput(e);
}

static void send_evt(InputEvtType_t t)
{
    InputEvt_t e = { .type = t, .key = 0 };
    (void)Q_SendInput(e);
}

static void handle(char c)
{
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'D') || c == '*' || c == '#') {
        send_key(c);
        return;
    }
    switch (c) {
    case 'o': g_doorOpen = 1u; send_evt(EVT_DOOR_OPEN);   break;
    case 'c': g_doorOpen = 0u; send_evt(EVT_DOOR_CLOSED); break;
    case 'e': g_emergencyReq = 1u; send_evt(EVT_EMERGENCY); break;
    case 'u': (void)Q_SendAct(ACT_UNLOCK);    break;
    case 'l': (void)Q_SendAct(ACT_LOCK);      break;
    case 't': (void)Q_SendAct(ACT_BEEP_TICK); break;
    case 'a': (void)Q_SendAct(ACT_ALARM_ON);  break;
    case 's': (void)Q_SendAct(ACT_ALARM_OFF); break;
    case '\r': case '\n': case ' ': break;
    default:  TRACE("HOOK? %c", c); break;
    }
}

void TestHook_Process(void)
{
    char c;
    /* Xử lý tối đa vài ký tự mỗi vòng để không giữ vòng lặp lâu. */
    for (int i = 0; i < 4 && UartPort_GetChar(&c); i++) {
        handle(c);
    }
}

#else
/* Tránh cảnh báo "ISO C forbids an empty translation unit". */
typedef int test_hook_unused_t;
#endif
