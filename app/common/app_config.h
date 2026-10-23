/**
 * @file    app_config.h
 * @brief   Mọi hằng số cấu hình của dự án (thời gian, kích thước, chân phụ).
 *
 * OWNER: foundation. Đổi giá trị qua PR riêng, không sửa trong branch tính năng.
 * Module chỉ đọc hằng số ở đây, không tự #define con số thời gian.
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* ---- Bảo mật (Task 2) ---------------------------------------------------- */
#define PIN_MASTER_DEFAULT      "1234"
#define PIN_MIN_LEN             4u
#define PIN_MAX_LEN             6u
#define MAX_FAIL_COUNT          3u

/* Bản build `test` (APP_TEST_HOOKS) rút ngắn thời gian để CI chạy nhanh. */
#ifdef APP_TEST_HOOKS
#define LOCKOUT_MS              6000u
#define DOOR_AJAR_MS            3000u
#else
#define LOCKOUT_MS              60000u
#define DOOR_AJAR_MS            15000u
#endif

/* ---- Nhập liệu (Task 1) -------------------------------------------------- */
#define KEYPAD_SCAN_MS          10u
#define KEY_DEBOUNCE_SAMPLES    3u     /* 3 x 10 ms = 30 ms */
#define DOOR_DEBOUNCE_SAMPLES   5u     /* 5 x 10 ms = 50 ms */
#define EMERG_DEBOUNCE_MS       200u

/* ---- Chấp hành (Task 3) -------------------------------------------------- */
#define RELOCK_MS               5000u
#define SERVO_LOCK_CCR          1000u  /* 1.0 ms -> 0 do  */
#define SERVO_UNLOCK_CCR        1500u  /* 1.5 ms -> 90 do */
#define BEEP_TICK_MS            50u
#define ALARM_ON_MS             500u
#define ALARM_OFF_MS            500u
/* 0: Wokwi (wokwi-buzzer cần sóng vuông, ngắt TIM2 đảo PB1)
 * 1: board thật dùng còi chủ động (chỉ cần giữ PB1 = HIGH)            */
#ifndef BUZZER_ACTIVE
#define BUZZER_ACTIVE           0
#endif

/* ---- Giao diện & log (Task 4) -------------------------------------------- */
#define LCD_I2C_ADDR            0x27u  /* 7-bit; một số module là 0x3F */
#define UART_TX_BUF_SIZE        1024u
#define UART_RX_BUF_SIZE        32u

/* ---- Hàng đợi ------------------------------------------------------------ */
#define Q_KEYPAD_LEN            16u
#define Q_ACTUATOR_LEN          8u
#define Q_UI_LEN                8u
#define Q_LOG_LEN               8u

/* ---- Hệ thống ------------------------------------------------------------ */
#define HEARTBEAT_MS            500u   /* LED PC13 nhấp nháy = vòng lặp còn chạy */

#endif /* APP_CONFIG_H */
