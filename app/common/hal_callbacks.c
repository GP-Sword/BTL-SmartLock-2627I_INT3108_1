/**
 * @file    hal_callbacks.c
 * @brief   MỘT định nghĩa duy nhất cho mỗi callback HAL. OWNER: foundation.
 *
 * HAL khai báo các callback là __weak: nếu hai branch cùng tự viết
 * HAL_GPIO_EXTI_Callback thì khi merge sẽ lỗi link "multiple definition".
 * Vì vậy mọi callback nằm ở đây và chỉ chuyển tiếp sang hàm của module sở hữu.
 * Module KHÔNG tự định nghĩa HAL_*Callback.
 *
 * Code trong các hàm *_On*() chạy trong ngắt: ngắn, không in UART, không HAL_Delay.
 */
#include "board_pins.h"

#include "actuator.h"
#include "input.h"
#include "uart_port.h"

/* EXTI line 15..10: PB11 nút khẩn cấp -> Task 1 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_EMERG_Pin) {
        Input_OnEmergencyIrq();
    }
}

/* TIM2 4 kHz: tạo âm buzzer -> Task 3 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        Actuator_OnToneTick();
    }
}

/* USART1: chỉ uart_port.c (Task 4) dùng */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        UartPort_OnTxCplt();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        UartPort_OnRxByte();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        UartPort_OnError();
    }
}
