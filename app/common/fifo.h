/**
 * @file    fifo.h
 * @brief   FIFO vòng kích thước phần tử cố định, an toàn giữa ISR và vòng lặp.
 *
 * Nhiều bên ghi (vòng lặp + ISR) được phép: push/pop tắt ngắt trong vài lệnh.
 * Khi chuyển sang RTOS: fifo_push -> xQueueSend, fifo_pop -> xQueueReceive.
 */
#ifndef FIFO_H
#define FIFO_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t          *buf;
    uint16_t          item_size;
    uint16_t          capacity;
    volatile uint16_t head;   /* vị trí ghi tiếp theo */
    volatile uint16_t tail;   /* vị trí đọc tiếp theo */
    volatile uint16_t count;
    volatile uint16_t dropped; /* số lần push thất bại vì đầy (để debug) */
} Fifo_t;

void     fifo_init(Fifo_t *f, void *storage, uint16_t item_size, uint16_t capacity);
bool     fifo_push(Fifo_t *f, const void *item); /* false nếu đầy  */
bool     fifo_pop(Fifo_t *f, void *item);        /* false nếu rỗng */
uint16_t fifo_count(const Fifo_t *f);
void     fifo_clear(Fifo_t *f);

#endif /* FIFO_H */
