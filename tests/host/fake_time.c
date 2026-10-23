#include "fake_time.h"

#include "app_time.h"

static uint32_t s_now;

uint32_t app_now_ms(void)       { return s_now; }
void fake_time_set(uint32_t ms) { s_now = ms; }
void fake_time_advance(uint32_t ms) { s_now += ms; }
