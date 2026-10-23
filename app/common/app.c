/**
 * @file    app.c
 * @brief   Super-loop bare-metal: mỗi "task" là một cặp Xxx_Init()/Xxx_Process().
 *          OWNER: foundation.
 *
 * Quy tắc cho mọi Process():
 *  - Không HAL_Delay(), không vòng while chờ cờ phần cứng không có timeout.
 *  - Chờ thời gian bằng soft_timer / app_elapsed_ms().
 *  - Mỗi lần gọi chỉ làm một phần việc ngắn (vài trăm µs trở xuống).
 */
#include "app.h"

#include <stdio.h>

#include "app_config.h"
#include "app_time.h"
#include "board_pins.h"
#include "queues.h"

#include "actuator.h"
#include "input.h"
#include "logger.h"
#include "security.h"
#include "uart_port.h"
#include "ui.h"

#ifdef APP_TEST_HOOKS
#include "test_hook.h"
#endif

/* Nguồn thời gian cho firmware (unit test cài bản giả của riêng nó). */
uint32_t app_now_ms(void)
{
    return HAL_GetTick();
}

static void App_Banner(void)
{
    char line[48];
    snprintf(line, sizeof(line), "SMARTLOCK READY %luMHz\r\n",
             (unsigned long)(SystemCoreClock / 1000000u));
    uart_write(line); /* CI chờ chuỗi "SMARTLOCK READY" - không đổi */
#ifdef APP_TEST_HOOKS
    uart_write("TEST HOOKS ON\r\n");
#endif
}

static void App_Heartbeat(void)
{
    static uint32_t t0;
    if (app_elapsed_ms(t0) >= HEARTBEAT_MS) {
        t0 = app_now_ms();
        HAL_GPIO_TogglePin(LED_BOARD_GPIO_Port, LED_BOARD_Pin);
    }
}

void App_Init(void)
{
    queues_init();
    UartPort_Init();   /* Task 4: phải trước mọi lệnh in */
    Input_Init();      /* Task 1 */
    Security_Init();   /* Task 2 */
    Actuator_Init();   /* Task 3: bật PWM servo + ngắt TIM2 */
    UI_Init();         /* Task 4: lcd_init() được phép HAL_Delay ở đây */
    Logger_Init();     /* Task 4 */
    App_Banner();
}

void App_Loop(void)
{
    Actuator_Process();  /* xử lý g_emergencyReq trước tiên */
    Input_Process();     /* quét keypad + cửa mỗi KEYPAD_SCAN_MS */
    Security_Process();  /* lấy hết sự kiện trong xKeypadQueue */
    UI_Process();
    Logger_Process();
    UartPort_Process();  /* đẩy dữ liệu TX ra UART nếu đang rảnh */
#ifdef APP_TEST_HOOKS
    TestHook_Process();  /* lệnh gõ từ Serial, xem test_hook.h */
#endif
    App_Heartbeat();
}
