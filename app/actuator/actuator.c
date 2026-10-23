/**
 * @file    actuator.c
 * @brief   STUB của foundation. Đã có sẵn: bật PWM servo ở 0 độ, bật ngắt TIM2,
 *          bật LED xanh khi có cờ khẩn cấp (để Task 1 thử riêng). Task 3 thay ruột.
 *
 * Việc cần làm (guide mục 7, Task 3):
 *  - Actuator_Process(): (1) g_emergencyReq -> xóa cờ, mở khóa; (2) Q_RecvAct hết lệnh;
 *    (3) cập nhật buzzer (BEEP_TICK_MS, ALARM_ON_MS/ALARM_OFF_MS);
 *    (4) tự khóa sau RELOCK_MS, chỉ đếm khi g_doorOpen == 0, rồi Q_SendInput({EVT_RELOCKED}).
 *  - Đổi góc servo: __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, SERVO_LOCK_CCR/UNLOCK_CCR).
 */
#include "actuator.h"

#include <stdbool.h>

#include "app_config.h"
#include "board_pins.h"
#include "queues.h"
#include "soft_timer.h"

static volatile bool s_toneOn = false; /* Task 3 bật/tắt; ngắt TIM2 đọc */

void Actuator_Init(void)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, SERVO_LOCK_CCR);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_Base_Start_IT(&htim2);
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
}

void Actuator_Process(void)
{
    /* STUB: cho Task 1 thấy ngắt khẩn cấp đã tới. Task 3 thay bằng mở khóa thật. */
    if (g_emergencyReq) {
        g_emergencyReq = 0u;
        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
    }

    ActCmd_t cmd;
    while (Q_RecvAct(&cmd)) {
        /* TODO(Task 3): xử lý UNLOCK / LOCK / BEEP_TICK / ALARM_ON / ALARM_OFF */
    }
}

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
