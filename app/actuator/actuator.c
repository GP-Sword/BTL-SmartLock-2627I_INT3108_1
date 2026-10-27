/**
 * @file    actuator.c
 * @brief   Task 3 (Actuator): Xử lý Servo, LED, Buzzer và Logic Tự động khóa
 */
#include "actuator.h"

#include <stdbool.h>

#include "app_config.h"
#include "board_pins.h"
#include "queues.h"
#include "soft_timer.h"
#include "app_time.h"

// ---------------- Khai báo phần cứng ngoại vi ---------------- //
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim2;

// ---------------- Biến trạng thái nội bộ ---------------- //
static volatile bool s_toneOn = false; /* Bật/tắt xung PWM buzzer ở hàm ngắt OnToneTick */

/* Quản lý Timer Buzzer Non-blocking */
static SoftTimer_t s_beepTimer;      // Dùng cho BEEP_TICK_MS
static SoftTimer_t s_alarmTimer;     // Dùng luân phiên chu kỳ ALARM_ON / ALARM_OFF
static bool s_isAlarmActive = false; // Trạng thái cảnh báo khẩn cấp đang bật
static bool s_alarmPhaseOn = false;  // Đang ở pha ON hay OFF của Alarm

/* Quản lý Tự động khóa (Relock - UC5) */
static SoftTimer_t s_relockTimer;
static bool s_isUnlocked = false;    // Trạng thái khóa đang mở (90 độ)

// ================= HÀM ĐIỀU KHIỂN PHẦN CỨNG CƠ BẢN ================= //
static void SetLockState(bool unlock) {
    if (unlock) {
        // MỞ KHÓA: UC4, UC9
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, SERVO_UNLOCK_CCR);
        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
        s_isUnlocked = true;
        soft_timer_start(&s_relockTimer, RELOCK_MS); // Bắt đầu bộ đếm tự khóa
    } else {
        // KHÓA LẠI: UC5
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, SERVO_LOCK_CCR);
        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
        s_isUnlocked = false;
        soft_timer_stop(&s_relockTimer);
    }
}

// ================= HÀM KHỞI TẠO ================= //
void Actuator_Init(void)
{
    // Cài đặt Servo ở vị trí KHÓA
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, SERVO_LOCK_CCR);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

    // Bật TIM2 chạy ngầm cho Wokwi Buzzer (gọi Actuator_OnToneTick)
    HAL_TIM_Base_Start_IT(&htim2);

    // Tắt LED Xanh
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    SetLockState(true);
}

// ================= HÀM VÒNG LẶP CHÍNH ================= //
void Actuator_Process(void)
{
    /* 1. KIỂM TRA NGẮT KHẨN CẤP (UC9) */
    if (g_emergencyReq) {
        g_emergencyReq = 0u; // Xóa cờ ngay lập tức
        SetLockState(false);  // Mở khóa, sáng LED, bắt đầu đếm tự khóa
    }

    /* 2. NHẬN VÀ XỬ LÝ LỆNH TỪ TASK 2 */
    ActCmd_t cmd;
    while (Q_RecvAct(&cmd)) {
        switch (cmd) {
            case ACT_UNLOCK:
                SetLockState(true);
                break;

            case ACT_LOCK:
                SetLockState(false);
                // Gửi thông báo đã khóa thành công cho Task 2
                {
                    InputEvt_t relockEvt = { .type = EVT_RELOCKED };
                    Q_SendInput(relockEvt);
                }
                break;

            case ACT_BEEP_TICK:
                if (!s_isAlarmActive) {
                    s_toneOn = true;
                    soft_timer_start(&s_beepTimer, BEEP_TICK_MS);
                }
                break;

            case ACT_ALARM_ON:
                s_isAlarmActive = true;
                s_alarmPhaseOn = true;
                s_toneOn = true;
                soft_timer_start(&s_alarmTimer, ALARM_ON_MS);
                break;

            case ACT_ALARM_OFF:
                s_isAlarmActive = false;
                s_toneOn = false;
                soft_timer_stop(&s_alarmTimer);
                break;

            default:
                break;
        }
    }

    /* 3. CẬP NHẬT TRẠNG THÁI BUZZER (Non-blocking) */
    if (s_isAlarmActive) {
        // Xử lý còi báo động chớp tắt liên tục
        if (soft_timer_expired(&s_alarmTimer)) {
            s_alarmPhaseOn = !s_alarmPhaseOn;
            s_toneOn = s_alarmPhaseOn;
            // Nạp lại timer tùy theo chu kỳ ON hay OFF
            soft_timer_start(&s_alarmTimer, s_alarmPhaseOn ? ALARM_ON_MS : ALARM_OFF_MS);
        }
    } else {
        // Xử lý tiếng Tích (Beep) ngắn 1 lần
        if (soft_timer_expired(&s_beepTimer)) {
            s_toneOn = false;
        }
    }

    /* 4. XỬ LÝ TỰ ĐỘNG KHÓA (RELOCK - UC5) */
    if (s_isUnlocked) {
        if (g_doorOpen != 0u) {
            // NẾU CỬA ĐANG MỞ: Tạm ngưng đếm ngược tự khóa (Reset liên tục Timer)
            soft_timer_start(&s_relockTimer, RELOCK_MS);
        } else {
            // NẾU CỬA ĐÃ ĐÓNG: Hết thời gian thì tự khóa lại
            if (soft_timer_expired(&s_relockTimer)) {
                SetLockState(false);
                // Báo cho Task 2 biết đã khóa
                InputEvt_t relockEvt = { .type = EVT_RELOCKED };
                Q_SendInput(relockEvt);
            }
        }
    }
}

// ================= HÀM NGẮT TIMER (BUZZER) ================= //
void Actuator_OnToneTick(void)
{
#if BUZZER_ACTIVE
    /* Còi chủ động (board thật): chỉ cần giữ mức */
    BUZZER_GPIO_Port->BSRR = s_toneOn ? BUZZER_Pin : ((uint32_t)BUZZER_Pin << 16u);
#else
    /* wokwi-buzzer cần sóng vuông: đảo chân 4 kHz -> tiếng 2 kHz */
    if (s_toneOn) {
        BUZZER_GPIO_Port->ODR ^= BUZZER_Pin;
    } else {
        BUZZER_GPIO_Port->BSRR = (uint32_t)BUZZER_Pin << 16u;
    }
#endif
}
