/**
 * @file    test_hook.h
 * @brief   Điều khiển firmware bằng ký tự gõ trên Serial. Chỉ có trong preset `test`
 *          (APP_TEST_HOOKS). Dùng cho scenario Wokwi CI và cho từng task thử riêng.
 *
 *   Ký tự                      Tác dụng
 *   0-9 A-D * #                Q_SendInput({EVT_KEY, ký tự})   (thay keypad)
 *   o / c                      g_doorOpen = 1/0 + EVT_DOOR_OPEN / EVT_DOOR_CLOSED
 *   e                          g_emergencyReq = 1 + EVT_EMERGENCY (thay nút khẩn cấp)
 *   u l t a s                  Q_SendAct: UNLOCK, LOCK, BEEP_TICK, ALARM_ON, ALARM_OFF
 *   \r \n khoảng trắng         bỏ qua
 *
 * Chữ IN HOA là phím, chữ thường là lệnh: 'A' (vào admin) khác 'a' (bật còi).
 */
#ifndef TEST_HOOK_H
#define TEST_HOOK_H

void TestHook_Process(void);

#endif /* TEST_HOOK_H */
