/**
 * @file    uart_port.h
 * @brief   Driver USART1 dùng chung cho mọi module. OWNER: Task 4.
 *
 * Mọi module in qua uart_write(); KHÔNG ai khác gọi HAL_UART_* trên huart1.
 * KHÔNG gọi uart_write() trong ngắt.
 */
#ifndef UART_PORT_H
#define UART_PORT_H

#include <stdbool.h>

void UartPort_Init(void);
void UartPort_Process(void);         /* đẩy ring buffer TX ra UART nếu rảnh */

void uart_write(const char *s);      /* chép cả chuỗi vào hàng gửi; đầy thì bỏ cả dòng */
bool UartPort_GetChar(char *c);      /* lấy 1 ký tự đã nhận (test hook dùng)          */

/* Gọi từ hal_callbacks.c (trong ngắt) */
void UartPort_OnTxCplt(void);
void UartPort_OnRxByte(void);
void UartPort_OnError(void);

#endif /* UART_PORT_H */
