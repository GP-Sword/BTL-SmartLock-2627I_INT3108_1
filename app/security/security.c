/**
 * @file    security.c
 * @brief   STUB của foundation: chỉ lấy sự kiện ra khỏi hàng đợi (hàm Q_RecvInput
 *          tự in vết "KEY 5", "DOOR OPEN"...). Task 2 thay ruột file này.
 *
 * Việc cần làm (guide mục 7, Task 2): máy trạng thái 5 trạng thái, so khớp PIN,
 * khóa LOCKOUT_MS sau MAX_FAIL_COUNT lần sai, đổi PIN chủ, hẹn giờ cửa DOOR_AJAR_MS,
 * alarm_update() duy nhất quyết định ALARM_ON/OFF, TRACE("STATE:...") khi đổi trạng thái.
 * Chỉ include app_time.h / queues.h / app_config.h / trace.h - KHÔNG include HAL.
 */
/**
 * @file    security.c
 * @brief   Khối Logic Bảo mật & Máy trạng thái FSM (Bare-metal Super-loop)
 */
#include "security.h"

#include "app_config.h"
#include "app_time.h"
#include "queues.h"
#include "trace.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Biến trạng thái FSM
static SysState_t s_state = STATE_IDLE;

// Dữ liệu nội bộ Task 2
static char s_masterPIN[7] = "1234";
static char s_inputBuffer[7] = "";
static uint8_t s_inputIdx = 0;
static uint8_t s_failCount = 0;

// Bộ đếm thời gian Non-blocking (dùng ms từ app_now_ms())
static uint32_t s_lockoutStartMs = 0;
static uint32_t s_unlockStartMs = 0;
static bool s_doorAlarmActive = false;

// Trạng thái công tắc cửa (extern từ foundation hoặc cờ toàn cục)
extern volatile bool g_doorOpen;

// --- HÀM BỔ TRỢ CHUYỂN TRẠNG THÁI (Có TRACE) ---
static void SetState(SysState_t newState) {
    if (s_state != newState) {
        s_state = newState;
        switch (s_state) {
            case STATE_IDLE:
                TRACE("STATE: IDLE");
                break;
            case STATE_ENTER_PIN:
                TRACE("STATE: ENTER_PIN");
                break;
            case STATE_UNLOCKED:
                TRACE("STATE: UNLOCKED");
                break;
            case STATE_LOCKED_OUT:
                TRACE("STATE: LOCKED_OUT");
                break;
            case STATE_CHANGE_PIN:
                TRACE("STATE: CHANGE_PIN");
                break;
            default:
                break;
        }
    }
}

// Clear bộ đệm phím
static void ResetBuffer(void) {
    memset(s_inputBuffer, 0, sizeof(s_inputBuffer));
    s_inputIdx = 0;
}

// Cập nhật trạng thái Còi báo động (Hàm tập trung duy nhất quyết định ALARM)
static void Alarm_Update(void) {
    if (s_state == STATE_LOCKED_OUT || s_doorAlarmActive) {
        Q_SendAction(CMD_ALARM_ON);
    } else {
        Q_SendAction(CMD_ALARM_OFF);
    }
}

// --- XỬ LÝ SỰ KIỆN PHÍM/ĐẦU VÀO ---
static void Security_HandleEvent(const InputEvt_t *evt) {
    // Nếu đang bị LOCKOUT -> Bỏ qua toàn bộ phím bấm
    if (s_state == STATE_LOCKED_OUT) {
        return;
    }

    char key = evt->key;

    // Phím '*' -> Reset nhập liệu và về IDLE ngay lập tức
    if (key == '*') {
        ResetBuffer();
        SetState(STATE_IDLE);
        Q_SendDisplay("  SMART LOCK  ", "Nhap PIN:", false, NULL);
        Alarm_Update();
        return;
    }

    switch (s_state) {
        // --- TRẠNG THÁI: IDLE / NHẬP PIN MỞ CỬA ---
        case STATE_IDLE:
        case STATE_ENTER_PIN:
            if (key >= '0' && key <= '9') {
                if (s_inputIdx < 6) {
                    s_inputBuffer[s_inputIdx++] = key;
                    s_inputBuffer[s_inputIdx] = '\0';

                    char mask[7] = "******";
                    mask[s_inputIdx] = '\0';
                    SetState(STATE_ENTER_PIN);
                    Q_SendDisplay("Nhap PIN:", mask, false, NULL);
                }
            }
            else if (key == '#') {
                if (s_inputIdx > 0) {
                    if (strcmp(s_inputBuffer, s_masterPIN) == 0) {
                        // PIN đúng -> Mở cửa
                        s_failCount = 0;
                        s_unlockStartMs = app_now_ms();
                        s_doorAlarmActive = false;

                        SetState(STATE_UNLOCKED);
                        Q_SendAction(CMD_UNLOCK_DOOR);
                        Q_SendDisplay("MO CUA THANH CONG", "Xin chao!", true, "UNLOCK_SUCCESS");
                    } else {
                        // PIN sai -> Tăng đếm lỗi
                        s_failCount++;
                        Q_SendAction(CMD_BEEP_ERROR);

                        if (s_failCount >= MAX_FAIL_COUNT) {
                            // Vượt quá số lần sai -> Khóa hệ thống
                            s_lockoutStartMs = app_now_ms();
                            SetState(STATE_LOCKED_OUT);
                            Q_SendDisplay("HE THONG BI KHOA", "Khoa 30 giay...", true, "SYSTEM_LOCKED");
                        } else {
                            char errStr[17];
                            snprintf(errStr, sizeof(errStr), "Sai! Thu lai(%d/%d)", s_failCount, MAX_FAIL_COUNT);
                            Q_SendDisplay("SAI MA PIN!", errStr, true, "WRONG_PIN");
                        }
                    }
                    ResetBuffer();
                    Alarm_Update();
                }
            }
            else if (key == 'A') {
                // Vào chế độ đổi PIN
                SetState(STATE_CHANGE_PIN);
                ResetBuffer();
                Q_SendDisplay("ADMIN MODE", "Nhap PIN moi:", false, NULL);
            }
            break;

        // --- TRẠNG THÁI: ĐỔI PIN CHỦ (UC7) ---
        case STATE_CHANGE_PIN:
            if (key >= '0' && key <= '9') {
                if (s_inputIdx < 6) {
                    s_inputBuffer[s_inputIdx++] = key;
                    s_inputBuffer[s_inputIdx] = '\0';
                    char mask[7] = "******";
                    mask[s_inputIdx] = '\0';
                    Q_SendDisplay("Nhap PIN moi:", mask, false, NULL);
                }
            }
            else if (key == '#') {
                if (s_inputIdx >= 4) {
                    strcpy(s_masterPIN, s_inputBuffer);
                    Q_SendDisplay("THANH CONG!", "PIN moi da luu", true, "CHANGE_PIN_OK");
                } else {
                    Q_SendDisplay("LOI: PIN >= 4 SO", "Huy doi PIN", false, NULL);
                }
                ResetBuffer();
                SetState(STATE_IDLE);
            }
            else if (key == 'C') {
                // Phím C: Hủy đổi PIN
                ResetBuffer();
                SetState(STATE_IDLE);
                Q_SendDisplay("HUY DOI PIN", "Nhap PIN:", false, NULL);
            }
            break;

        default:
            break;
    }
}

// --- XỬ LÝ ĐỊNH THỜI VÀ TRẠNG THÁI THEO THỜI GIAN (TICK) ---
static void Security_Tick(uint32_t nowMs) {
    // 1. Kiểm tra hết thời gian LOCKOUT (30s)
    if (s_state == STATE_LOCKED_OUT) {
        if ((nowMs - s_lockoutStartMs) >= LOCKOUT_MS) {
            s_failCount = 0;
            SetState(STATE_IDLE);
            Q_SendDisplay("DA MO KHOA HT", "Nhap PIN:", true, "SYSTEM_UNLOCKED");
            Alarm_Update();
        }
    }

    // 2. Kiểm tra Cửa mở quá 15s (DOOR_AJAR_MS)
    if (s_state == STATE_UNLOCKED) {
        if (g_doorOpen) {
            if (!s_doorAlarmActive && ((nowMs - s_unlockStartMs) >= DOOR_AJAR_MS)) {
                s_doorAlarmActive = true;
                Q_SendDisplay("CANH BAO!", "Cua mo qua 15s!", true, "ALARM_DOOR_15S");
                Alarm_Update();
            }
        } else {
            // Cửa đã đóng lại -> Khóa chốt Servo & tắt alarm
            s_doorAlarmActive = false;
            Q_SendAction(CMD_LOCK_DOOR);
            SetState(STATE_IDLE);
            Q_SendDisplay("CUA DA DONG", "Nhap PIN:", false, NULL);
            Alarm_Update();
        }
    }
}

// --- PUBLIC FUNCTIONS ---

void Security_Init(void) {
    s_state = STATE_IDLE;
    s_failCount = 0;
    s_doorAlarmActive = false;
    ResetBuffer();
    TRACE("STATE: IDLE");
}

void Security_Process(void) {
    InputEvt_t evt;
    // Đọc toàn bộ sự kiện trong Queue
    while (Q_RecvInput(&evt)) {
        Security_HandleEvent(&evt);
    }
    // Chạy kiểm tra Timer non-blocking bằng thời gian thực hệ thống
    Security_Tick(app_now_ms());
}

SysState_t Security_GetState(void) {
    return s_state;
}
