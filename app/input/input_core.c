/**
 * @file    input_core.c
 * @brief   Task 1 - Logic thuần của khối nhập liệu (không HAL). Xem input_core.h.
 *          Nút khẩn cấp dùng cờ armed: nhận một lần nhấn thì tắt, chỉ bật lại sau mẫu "nhả" ổn định.
 */
#include "input_core.h"

/* ---- Trạng thái ---------------------------------------------------------- */
static Debounce_t s_key;
static Debounce_t s_door;

/* Chống rung nút khẩn cấp: mốc là lần ĐƯỢC NHẬN gần nhất; lần đầu luôn nhận.
 * Ghi trong ngắt, nên khai báo volatile. */
static volatile uint8_t  s_emergSeen;
static volatile uint32_t s_emergLast;

/* Nút chỉ được nhận khi armed: = 1 sau Init, về 0 khi nhận một lần nhấn, về 1 lại khi mức
 * "nhả" ổn định DOOR_DEBOUNCE_SAMPLES mẫu liên tiếp (chặn cạnh rung khi nhả sau cửa sổ 200 ms). */
static volatile uint8_t s_emergArmed;
static uint8_t          s_emergRelCnt; /* chỉ ghi ngoài ngắt */

/* ---- Chống rung ---------------------------------------------------------- */
void debounce_init(Debounce_t *d, uint8_t initial)
{
    d->last   = initial;
    d->stable = initial;
    d->count  = 0u;
}

bool debounce_update(Debounce_t *d, uint8_t sample, uint8_t samples)
{
    if (sample == d->stable) {
        d->last  = sample;
        d->count = 0u;
        return false;
    }
    if (sample == d->last) {
        if (d->count < UINT8_MAX) {
            d->count++;
        }
    } else {
        d->last  = sample;
        d->count = 1u;
    }
    if (d->count >= ((samples > 0u) ? samples : 1u)) {
        d->stable = sample;
        d->count  = 0u;
        return true;
    }
    return false;
}

/* ---- Cửa ----------------------------------------------------------------- */
static void door_publish(uint8_t open)
{
    InputEvt_t evt = { .type = open ? EVT_DOOR_OPEN : EVT_DOOR_CLOSED, .key = 0 };

    g_doorOpen = open ? 1u : 0u; /* ghi cờ TRƯỚC khi gửi sự kiện */
    (void)Q_SendInput(evt);
}

/* ---- API ----------------------------------------------------------------- */
void InputCore_Init(uint8_t doorOpen)
{
    debounce_init(&s_key, 0u);
    debounce_init(&s_door, doorOpen ? 1u : 0u);
    s_emergSeen = 0u;
    s_emergLast = 0u;
    s_emergArmed = 1u;
    s_emergRelCnt = 0u;
    door_publish(doorOpen);
}

void InputCore_Tick(char rawKey, uint8_t doorOpen)
{
    /* Phím: chỉ gửi ở cạnh nhấn (stable 0 -> phím); đổi thẳng phím khác chỉ cập nhật stable. */
    uint8_t prev = s_key.stable;
    if (debounce_update(&s_key, (uint8_t)rawKey, KEY_DEBOUNCE_SAMPLES) &&
        prev == 0u && s_key.stable != 0u) {
        InputEvt_t evt = { .type = EVT_KEY, .key = (char)s_key.stable };
        (void)Q_SendInput(evt);
    }

    if (debounce_update(&s_door, doorOpen ? 1u : 0u, DOOR_DEBOUNCE_SAMPLES)) {
        door_publish(s_door.stable);
    }
}

void InputCore_EmergencyLevel(uint8_t pressed)
{
    if (pressed) {
        s_emergRelCnt = 0u;
    } else if (!s_emergArmed) {
        if (++s_emergRelCnt >= DOOR_DEBOUNCE_SAMPLES) {
            s_emergRelCnt = 0u;
            s_emergArmed  = 1u;
        }
    }
}

void InputCore_OnEmergency(void)
{
    if (!s_emergArmed) {
        return;
    }
    if (s_emergSeen && app_elapsed_ms(s_emergLast) < EMERG_DEBOUNCE_MS) {
        return;
    }
    s_emergSeen  = 1u;
    s_emergArmed = 0u;
    s_emergLast = app_now_ms();
    g_emergencyReq = 1u;
    InputEvt_t evt = { .type = EVT_EMERGENCY, .key = 0 };
    (void)Q_SendInput(evt);
}
