/**
 * @file    uart_port.c
 * @brief   BẢN SEED của foundation:
 *            - TX: tạm gửi chặn từng byte (vẫn đọc RX trong lúc gửi).
 *            - RX: nhận từng byte bằng ngắt vào FIFO, kèm rx_poll() dự phòng mỗi vòng lặp
 *              (đã hoàn chỉnh, giữ nguyên).
 *          Task 4 thay phần TX bằng ring buffer UART_TX_BUF_SIZE + HAL_UART_Transmit_IT
 *          (UartPort_Process khởi động gửi, UartPort_OnTxCplt dời tail và gửi tiếp).
 */
#include "uart_port.h"

#include "app_config.h"
#include "board_pins.h"
#include "fifo.h"

static uint8_t s_rxByte;
static char    s_rxStorage[UART_RX_BUF_SIZE];
static Fifo_t  s_rxFifo;

static void rx_arm(void)
{
    (void)HAL_UART_Receive_IT(&huart1, &s_rxByte, 1u);
}

void UartPort_Init(void)
{
    fifo_init(&s_rxFifo, s_rxStorage, sizeof(char), UART_RX_BUF_SIZE);
    rx_arm();
}

/* Dự phòng RX: nếu byte nằm trong DR mà ngắt chưa lấy (trình mô phỏng không phát ngắt RXNE,
 * hoặc ngắt nhận chưa được bật lại), đọc thẳng thanh ghi. Khi ngắt hoạt động bình thường,
 * ISR luôn đọc DR trước nên đoạn này không làm gì. Đọc SR rồi DR cũng xóa cờ ORE. */
static void rx_poll(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t sr = huart1.Instance->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        char c = (char)(huart1.Instance->DR & 0xFFu);
        if (sr & USART_SR_RXNE) {
            (void)fifo_push(&s_rxFifo, &c);
        }
    }
    __set_PRIMASK(primask);
}

void UartPort_Process(void)
{
    rx_poll();
    /* TODO(Task 4): nếu UART rảnh và ring buffer còn dữ liệu -> HAL_UART_Transmit_IT */
}

void uart_write(const char *s)
{
    /* SEED: gửi chặn từng byte, vẫn đọc RX giữa các byte để chuỗi lệnh gửi liền
     * (ví dụ "1234#") không bị tràn khi đang in. UART kẹt quá 50 ms thì bỏ phần còn lại. */
    uint32_t t0 = HAL_GetTick();
    for (; *s != '\0'; s++) {
        while ((huart1.Instance->SR & USART_SR_TXE) == 0u) {
            rx_poll();
            if (HAL_GetTick() - t0 > 50u) {
                return;
            }
        }
        huart1.Instance->DR = (uint8_t)*s;
        rx_poll();
    }
}

bool UartPort_GetChar(char *c)
{
    return fifo_pop(&s_rxFifo, c);
}

void UartPort_OnTxCplt(void)
{
    /* TODO(Task 4): dời tail, gửi tiếp nếu còn */
}

void UartPort_OnRxByte(void)
{
    char c = (char)s_rxByte;
    (void)fifo_push(&s_rxFifo, &c);
    rx_arm();
}

void UartPort_OnError(void)
{
    /* Lỗi overrun/noise: bật nhận lại để không mất RX vĩnh viễn */
    rx_arm();
}
