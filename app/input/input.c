/**
 * @file    input.c
 * @brief   STUB của foundation. Task 1 thay ruột file này (và có thể thêm file
 *          khác trong app/input/, CMake tự nhận).
 *
 * Việc cần làm (xem guide mục 7, Task 1):
 *  - Input_Process(): cứ KEYPAD_SCAN_MS quét ma trận 4x4 (BOARD_KP_* trong board_pins.h),
 *    chống rung KEY_DEBOUNCE_SAMPLES, chỉ gửi ở cạnh nhấn: Q_SendInput({EVT_KEY, k}).
 *  - Cùng nhịp đọc DOOR_SW (PB10), chống rung DOOR_DEBOUNCE_SAMPLES; khi đổi:
 *    ghi g_doorOpen (1 = mở) RỒI gửi EVT_DOOR_OPEN / EVT_DOOR_CLOSED.
 *  - Input_Init(): đọc trạng thái cửa ban đầu, ghi g_doorOpen và gửi 1 sự kiện.
 *  - Input_OnEmergencyIrq() (trong ngắt): bỏ qua nếu < EMERG_DEBOUNCE_MS từ lần trước;
 *    g_emergencyReq = 1; Q_SendInput({EVT_EMERGENCY}). Không in, không điều khiển servo.
 */
#include "input.h"

#include "app_config.h"
#include "board_pins.h"
#include "queues.h"

void Input_Init(void)
{
    /* TODO(Task 1) */
}

void Input_Process(void)
{
    /* TODO(Task 1) */
}

void Input_OnEmergencyIrq(void)
{
    /* TODO(Task 1) */
}
