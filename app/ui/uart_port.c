/**
 * @file    uart_port.c
 * @brief   Driver USART1 cho mọi module (Task 4 sở hữu).
 *          - RX: ngắt vào FIFO, kèm rx_poll() dự phòng.
 *          - TX: ring buffer UART_TX_BUF_SIZE truyền bằng HAL_UART_Transmit_IT.
 */
#include "uart_port.h"

#include <string.h>

#include "app_config.h"
#include "board_pins.h"
#include "fifo.h"

/* ---- RX ------------------------------------------------------------------ */
static uint8_t s_rxByte;
static char    s_rxStorage[UART_RX_BUF_SIZE];
static Fifo_t  s_rxFifo;

/* ---- TX ------------------------------------------------------------------ */
static char              s_txBuf[UART_TX_BUF_SIZE];
static volatile uint16_t s_txHead = 0u;
static volatile uint16_t s_txTail = 0u;
static volatile uint16_t s_txCount = 0u;
static volatile uint16_t s_txSending = 0u;
static volatile bool     s_txBusy = false;

static void rx_arm(void)
{
    (void)HAL_UART_Receive_IT(&huart1, &s_rxByte, 1u);
}

static void tx_start_chunk(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    if (!s_txBusy && s_txCount > 0u && huart1.gState == HAL_UART_STATE_READY) {
        uint16_t chunk;
        if (s_txHead > s_txTail) {
            chunk = (uint16_t)(s_txHead - s_txTail);
        } else {
            chunk = (uint16_t)(UART_TX_BUF_SIZE - s_txTail);
        }
        s_txSending = chunk;
        s_txBusy = true;
        uint16_t tail_idx = s_txTail;
        __set_PRIMASK(primask);

        if (HAL_UART_Transmit_IT(&huart1, (uint8_t *)&s_txBuf[tail_idx], chunk) != HAL_OK) {
            primask = __get_PRIMASK();
            __disable_irq();
            s_txBusy = false;
            s_txSending = 0u;
            __set_PRIMASK(primask);
        }
        return;
    }

    __set_PRIMASK(primask);
}

void UartPort_Init(void)
{
    fifo_init(&s_rxFifo, s_rxStorage, sizeof(char), UART_RX_BUF_SIZE);
    s_txHead = 0u;
    s_txTail = 0u;
    s_txCount = 0u;
    s_txSending = 0u;
    s_txBusy = false;
    rx_arm();
}

/* Dự phòng RX: đọc DR thẳng nếu có byte mà ngắt chưa lấy */
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
    tx_start_chunk();
}

void uart_write(const char *s)
{
    if (s == NULL) {
        return;
    }
    size_t len = strlen(s);
    if (len == 0u || len > UART_TX_BUF_SIZE) {
        return;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    uint16_t avail = (uint16_t)(UART_TX_BUF_SIZE - s_txCount);
    if ((uint16_t)len > avail) {
        /* Đầy thì bỏ cả dòng mới */
        __set_PRIMASK(primask);
        return;
    }

    for (size_t i = 0u; i < len; i++) {
        s_txBuf[s_txHead] = s[i];
        s_txHead = (uint16_t)((s_txHead + 1u) % UART_TX_BUF_SIZE);
    }
    s_txCount = (uint16_t)(s_txCount + (uint16_t)len);

    __set_PRIMASK(primask);

    tx_start_chunk();
}

bool UartPort_GetChar(char *c)
{
    return fifo_pop(&s_rxFifo, c);
}

void UartPort_OnTxCplt(void)
{
    s_txTail = (uint16_t)((s_txTail + s_txSending) % UART_TX_BUF_SIZE);
    s_txCount = (uint16_t)(s_txCount - s_txSending);
    s_txSending = 0u;

    if (s_txCount > 0u) {
        uint16_t chunk;
        if (s_txHead > s_txTail) {
            chunk = (uint16_t)(s_txHead - s_txTail);
        } else {
            chunk = (uint16_t)(UART_TX_BUF_SIZE - s_txTail);
        }
        s_txSending = chunk;
        if (HAL_UART_Transmit_IT(&huart1, (uint8_t *)&s_txBuf[s_txTail], chunk) != HAL_OK) {
            s_txBusy = false;
            s_txSending = 0u;
        }
    } else {
        s_txBusy = false;
    }
}

void UartPort_OnRxByte(void)
{
    char c = (char)s_rxByte;
    (void)fifo_push(&s_rxFifo, &c);
    rx_arm();
}

void UartPort_OnError(void)
{
    s_txBusy = false;
    s_txSending = 0u;
    rx_arm();
}
