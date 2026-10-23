/**
 * @file    soft_timer.h
 * @brief   Bộ định thời phần mềm one-shot, thay TimerHandle_t của FreeRTOS.
 *
 * Ví dụ:
 *   static SoftTimer_t relock;
 *   soft_timer_start(&relock, RELOCK_MS);
 *   ...
 *   if (soft_timer_expired(&relock)) { lock(); }   // trả true đúng 1 lần
 */
#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include <stdbool.h>
#include <stdint.h>

#include "app_time.h"

typedef struct {
    uint32_t start;
    uint32_t period;
    bool     running;
} SoftTimer_t;

static inline void soft_timer_start(SoftTimer_t *t, uint32_t ms)
{
    t->start   = app_now_ms();
    t->period  = ms;
    t->running = true;
}

static inline void soft_timer_stop(SoftTimer_t *t)
{
    t->running = false;
}

static inline bool soft_timer_running(const SoftTimer_t *t)
{
    return t->running;
}

/* Trả true đúng MỘT lần khi hết hạn, sau đó timer dừng. */
static inline bool soft_timer_expired(SoftTimer_t *t)
{
    if (t->running && (app_now_ms() - t->start >= t->period)) {
        t->running = false;
        return true;
    }
    return false;
}

/* Thời gian còn lại (ms), 0 nếu đã dừng hoặc hết hạn. */
static inline uint32_t soft_timer_remaining(const SoftTimer_t *t)
{
    if (!t->running) {
        return 0u;
    }
    uint32_t el = app_now_ms() - t->start;
    return (el >= t->period) ? 0u : (t->period - el);
}

#endif /* SOFT_TIMER_H */
