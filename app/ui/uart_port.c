/**
 * @file    uart_port.c
 * @brief   BẢN SEED của foundation:
 *            - TX: tạm gửi chặn bằng HAL_UART_Transmit (timeout theo độ dài).
 *            - RX: nhận từng byte bằng ngắt vào FIFO (đã hoàn chỉnh, giữ nguyên).
 *          Task 4 thay phần TX bằng ring buffer UART_TX_BUF_SIZE + HAL_UART_Transmit_IT
 *          (UartPort_Process khởi động gửi, UartPort_OnTxCplt dời tail và gửi tiếp).
 */
#include "uart_port.h"

#include <string.h>

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

void UartPort_Process(void)
{
    /* TODO(Task 4): nếu UART rảnh và ring buffer còn dữ liệu -> HAL_UART_Transmit_IT */
}

void uart_write(const char *s)
{
    /* SEED: gửi chặn. 115200 baud ~ 11.5 byte/ms -> timeout = len/10 + 5 ms */
    size_t len = strlen(s);
    if (len == 0u) {
        return;
    }
    (void)HAL_UART_Transmit(&huart1, (const uint8_t *)s, (uint16_t)len,
                            (uint32_t)(len / 10u + 5u));
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
