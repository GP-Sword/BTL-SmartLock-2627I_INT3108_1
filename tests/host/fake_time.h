/**
 * @file    fake_time.h
 * @brief   Đồng hồ giả cho unit test: thay app_now_ms() của firmware.
 *          Dùng: fake_time_set(0); ... fake_time_advance(59000);
 */
#ifndef FAKE_TIME_H
#define FAKE_TIME_H

#include <stdint.h>

void fake_time_set(uint32_t ms);
void fake_time_advance(uint32_t ms);

#endif /* FAKE_TIME_H */
