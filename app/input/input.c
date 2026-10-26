/**
 * @file    input.c
 * @brief   Task 1 - Lớp HAL mỏng của khối nhập liệu: quét keypad 4x4, đọc công tắc cửa PB10,
 *          chia nhịp KEYPAD_SCAN_MS (mốc = app_now_ms() mỗi lần quét, không quét dồn khi vòng lặp
 *          trễ) rồi đưa mẫu vào input_core (chống rung, gửi sự kiện).
 *          Ngắt nút khẩn cấp PB11 chỉ chuyển tiếp sang InputCore_OnEmergency().
 */
#include "input.h"

#include "app_config.h"
#include "app_time.h"
#include "board_pins.h"
#include "input_core.h"

/* Số vòng __NOP chờ tín hiệu cột ổn định sau khi kéo hàng xuống LOW: ≈ 5–10 µs @72 MHz.
 * Tinh chỉnh phần cứng của riêng Task 1, không phải thông số chia sẻ giữa các task. */
#define KP_SETTLE_LOOPS  60u

#define KP_ROWS  4u
#define KP_COLS  4u

static uint32_t s_scanT0; /* mốc của lần quét trước */

static void kp_settle(void)
{
    for (volatile uint32_t i = 0; i < KP_SETTLE_LOOPS; i++) {
        __NOP();
    }
}

/* Quét ma trận: trả phím đầu tiên theo thứ tự R1..R4 / C1..C4, 0 nếu không nhấn.
 * Hàng luôn được trả về HIGH trước khi thoát. */
static char keypad_scan_raw(void)
{
    static GPIO_TypeDef *const rowPort[KP_ROWS] = BOARD_KP_ROW_PORTS;
    static const uint16_t      rowPin[KP_ROWS]  = BOARD_KP_ROW_PINS;
    static GPIO_TypeDef *const colPort[KP_COLS] = BOARD_KP_COL_PORTS;
    static const uint16_t      colPin[KP_COLS]  = BOARD_KP_COL_PINS;
    static const char          keymap[KP_ROWS][KP_COLS] = BOARD_KEYMAP;

    char key = 0;

    for (uint32_t r = 0; r < KP_ROWS && key == 0; r++) {
        HAL_GPIO_WritePin(rowPort[r], rowPin[r], GPIO_PIN_RESET);
        kp_settle();
        for (uint32_t c = 0; c < KP_COLS; c++) {
            if (HAL_GPIO_ReadPin(colPort[c], colPin[c]) == GPIO_PIN_RESET) {
                key = keymap[r][c];
                break;
            }
        }
        HAL_GPIO_WritePin(rowPort[r], rowPin[r], GPIO_PIN_SET);
    }
    return key;
}

static uint8_t door_is_open(void)
{
    return (HAL_GPIO_ReadPin(DOOR_SW_GPIO_Port, DOOR_SW_Pin) != BOARD_DOOR_CLOSED_LEVEL) ? 1u : 0u;
}

void Input_Init(void)
{
    InputCore_Init(door_is_open());
    s_scanT0 = app_now_ms();
}

void Input_Process(void)
{
    if (app_elapsed_ms(s_scanT0) < KEYPAD_SCAN_MS) {
        return;
    }
    s_scanT0 = app_now_ms();
    InputCore_Tick(keypad_scan_raw(), door_is_open());
}

void Input_OnEmergencyIrq(void)
{
    InputCore_OnEmergency();
}
