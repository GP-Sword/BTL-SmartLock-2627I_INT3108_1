/**
 * @file    input_core.h
 * @brief   Task 1 - Logic thuần của khối nhập liệu: chống rung phím/cửa/nút khẩn cấp.
 *          OWNER: Task 1.
 *
 * KHÔNG include HAL: input.c (lớp HAL mỏng) đọc chân rồi đưa mẫu vào đây,
 * nên toàn bộ logic chạy được trong unit test trên PC (tests/host/test_input.c).
 */
#ifndef INPUT_CORE_H
#define INPUT_CORE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "app_time.h"
#include "queues.h"

/* Bộ chống rung: giá trị chỉ được chấp nhận khi `samples` mẫu liên tiếp giống nhau. */
typedef struct {
    uint8_t last;   /* mẫu gần nhất khác `stable` (ứng viên)  */
    uint8_t stable; /* giá trị đã được chấp nhận              */
    uint8_t count;  /* số mẫu liên tiếp giống `last`          */
} Debounce_t;

void debounce_init(Debounce_t *d, uint8_t initial);

/* Đưa vào một mẫu. Trả true đúng 1 lần khi `stable` đổi (ở mẫu thứ `samples`). */
bool debounce_update(Debounce_t *d, uint8_t sample, uint8_t samples);

/* Xóa mọi trạng thái; ghi g_doorOpen và gửi 1 sự kiện cửa theo `doorOpen` (1 = mở). */
void InputCore_Init(uint8_t doorOpen);

/* Gọi mỗi KEYPAD_SCAN_MS. rawKey: ký tự phím đang nhấn, 0 nếu không có. doorOpen: 1 = mở.
 * Gửi {EVT_KEY,k} khi phím ổn định đổi từ 0 sang phím (đổi thẳng phím này sang phím khác
 * không gửi); khi cửa ổn định đổi:
 * ghi g_doorOpen TRƯỚC rồi gửi EVT_DOOR_OPEN / EVT_DOOR_CLOSED. */
void InputCore_Tick(char rawKey, uint8_t doorOpen);

/* Gọi từ ngắt nút khẩn cấp: bỏ nếu cách lần được nhận trước < EMERG_DEBOUNCE_MS
 * (lần đầu luôn nhận; ngắt bị bỏ không dời mốc); g_emergencyReq = 1 rồi Q_SendInput({EVT_EMERGENCY}). */
void InputCore_OnEmergency(void);

#endif /* INPUT_CORE_H */
