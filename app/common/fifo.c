/**
 * @file    fifo.c
 * @brief   Cài đặt FIFO vòng. OWNER: foundation.
 */
#include "fifo.h"

#include <string.h>

#ifdef UNIT_TEST
/* Build trên PC: không có ngắt. */
#define CRIT_ENTER()  do { } while (0)
#define CRIT_EXIT()   do { } while (0)
#else
#include "stm32f1xx.h"
/* Lưu/khôi phục PRIMASK để gọi lồng trong ISR vẫn đúng. */
#define CRIT_ENTER()  uint32_t primask_ = __get_PRIMASK(); __disable_irq()
#define CRIT_EXIT()   __set_PRIMASK(primask_)
#endif

void fifo_init(Fifo_t *f, void *storage, uint16_t item_size, uint16_t capacity)
{
    f->buf       = (uint8_t *)storage;
    f->item_size = item_size;
    f->capacity  = capacity;
    f->head      = 0;
    f->tail      = 0;
    f->count     = 0;
    f->dropped   = 0;
}

bool fifo_push(Fifo_t *f, const void *item)
{
    bool ok = false;
    CRIT_ENTER();
    if (f->count < f->capacity) {
        memcpy(&f->buf[(uint32_t)f->head * f->item_size], item, f->item_size);
        f->head = (uint16_t)((f->head + 1u) % f->capacity);
        f->count++;
        ok = true;
    } else {
        f->dropped++;
    }
    CRIT_EXIT();
    return ok;
}

bool fifo_pop(Fifo_t *f, void *item)
{
    bool ok = false;
    CRIT_ENTER();
    if (f->count > 0u) {
        memcpy(item, &f->buf[(uint32_t)f->tail * f->item_size], f->item_size);
        f->tail = (uint16_t)((f->tail + 1u) % f->capacity);
        f->count--;
        ok = true;
    }
    CRIT_EXIT();
    return ok;
}

uint16_t fifo_count(const Fifo_t *f)
{
    return f->count;
}

void fifo_clear(Fifo_t *f)
{
    CRIT_ENTER();
    f->head  = 0;
    f->tail  = 0;
    f->count = 0;
    CRIT_EXIT();
}
