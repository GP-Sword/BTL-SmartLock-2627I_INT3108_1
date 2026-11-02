/**
 * @file    test_input.c
 * @brief   Unit test lõi nhập liệu Task 1 (app/input/input_core.c). OWNER: Task 1.
 */
#include "fake_time.h"
#include "mini_test.h"

#include "app_config.h"
#include "input_core.h"
#include "queues.h"

/* Chặn Q_SendInput (link --wrap) để ghi lại cờ ngay lúc sự kiện được gửi. */
bool __real_Q_SendInput(InputEvt_t evt);
bool __wrap_Q_SendInput(InputEvt_t evt);

static uint8_t s_doorAtSend = 0xFFu;
static uint8_t s_emergAtSend = 0xFFu;

bool __wrap_Q_SendInput(InputEvt_t evt)
{
    if (evt.type == EVT_DOOR_OPEN || evt.type == EVT_DOOR_CLOSED) {
        s_doorAtSend = g_doorOpen;
    } else if (evt.type == EVT_EMERGENCY) {
        s_emergAtSend = g_emergencyReq;
    }
    return __real_Q_SendInput(evt);
}

/* Xả hàng đợi bằng fifo_pop để không phụ thuộc TRACE của Q_RecvInput. */
static bool pop(InputEvt_t *e)
{
    return fifo_pop(&xKeypadQueue, e);
}

static void drain(void)
{
    InputEvt_t e;
    while (pop(&e)) {
    }
}

void setUp(void)
{
    fake_time_set(0);
    queues_init();
    InputCore_Init(0);
    drain(); /* bỏ sự kiện cửa ban đầu */
}

void tearDown(void) {}

/* n nhịp quét, mỗi nhịp KEYPAD_SCAN_MS. */
static void tick(char k, uint8_t door, unsigned n)
{
    for (unsigned i = 0; i < n; i++) {
        fake_time_advance(KEYPAD_SCAN_MS);
        InputCore_Tick(k, door);
    }
}

/* Gõ một phím: nhấn đủ mẫu rồi nhả đủ mẫu. */
static void tap(char k)
{
    tick(k, 0, KEY_DEBOUNCE_SAMPLES);
    tick(0, 0, KEY_DEBOUNCE_SAMPLES);
}

static unsigned count_events(void)
{
    return fifo_count(&xKeypadQueue);
}

/* ---- phím ---------------------------------------------------------------- */
static void test_key_fires_on_nth_sample(void)
{
    InputEvt_t e;
    tick('5', 0, KEY_DEBOUNCE_SAMPLES - 1u);
    TEST_ASSERT_EQUAL(0, count_events());
    tick('5', 0, 1);
    TEST_ASSERT_EQUAL(1, count_events());
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL(EVT_KEY, e.type);
    TEST_ASSERT_EQUAL_CHAR('5', e.key);
}

static void test_hold_does_not_repeat(void)
{
    tick('5', 0, 500);
    TEST_ASSERT_EQUAL(1, count_events());
}

static void test_bounce_rejected(void)
{
    for (unsigned i = 0; i < 40; i++) {
        tick((i % 2u) ? 0 : '5', 0, 1);
    }
    TEST_ASSERT_EQUAL(0, count_events());
}

static void test_release_sends_nothing(void)
{
    InputEvt_t e;
    tick('5', 0, KEY_DEBOUNCE_SAMPLES);
    TEST_ASSERT_TRUE(pop(&e));
    tick(0, 0, 50);
    TEST_ASSERT_EQUAL(0, count_events());
}

static void test_press_release_press_sends_two(void)
{
    InputEvt_t e;
    tap('5');
    tap('5');
    TEST_ASSERT_EQUAL(2, count_events());
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL_CHAR('5', e.key);
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL_CHAR('5', e.key);
}

static void test_direct_key_change(void)
{
    InputEvt_t e;
    tick('1', 0, KEY_DEBOUNCE_SAMPLES);
    tick('2', 0, KEY_DEBOUNCE_SAMPLES); /* đổi thẳng, không qua 0 */
    tick('2', 0, 50);
    TEST_ASSERT_EQUAL(1, count_events());
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL_CHAR('1', e.key);
}

static void test_direct_change_then_release_then_press(void)
{
    InputEvt_t e;
    tick('1', 0, KEY_DEBOUNCE_SAMPLES);
    tick('2', 0, KEY_DEBOUNCE_SAMPLES);
    TEST_ASSERT_EQUAL(1, count_events());
    tick(0, 0, KEY_DEBOUNCE_SAMPLES);
    tick('2', 0, KEY_DEBOUNCE_SAMPLES);
    TEST_ASSERT_EQUAL(2, count_events());
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL_CHAR('1', e.key);
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL_CHAR('2', e.key);
}

static void test_all_16_keys_in_order(void)
{
    const char *keys = "123A456B789C*0#D";
    InputEvt_t e;
    for (const char *p = keys; *p; p++) {
        tap(*p);
    }
    TEST_ASSERT_EQUAL(16, count_events());
    for (const char *p = keys; *p; p++) {
        TEST_ASSERT_TRUE(pop(&e));
        TEST_ASSERT_EQUAL(EVT_KEY, e.type);
        TEST_ASSERT_EQUAL_CHAR(*p, e.key);
    }
}

/* ---- cửa ----------------------------------------------------------------- */
static void test_door_init_publishes_once(void)
{
    InputEvt_t e;
    queues_init();
    InputCore_Init(1);
    TEST_ASSERT_EQUAL(1, g_doorOpen);
    TEST_ASSERT_EQUAL(1, count_events());
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL(EVT_DOOR_OPEN, e.type);
    tick(0, 1, 50); /* vẫn mở: không thêm sự kiện */
    TEST_ASSERT_EQUAL(0, count_events());
}

static void test_door_needs_n_samples(void)
{
    InputEvt_t e;
    tick(0, 1, DOOR_DEBOUNCE_SAMPLES - 1u);
    TEST_ASSERT_EQUAL(0, count_events());
    TEST_ASSERT_EQUAL(0, g_doorOpen);
    tick(0, 1, 1);
    TEST_ASSERT_EQUAL(1, g_doorOpen);
    TEST_ASSERT_EQUAL(1, count_events());
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL(EVT_DOOR_OPEN, e.type);
}

static void test_door_sets_flag_before_event(void)
{
    s_doorAtSend = 0xFFu;
    tick(0, 1, DOOR_DEBOUNCE_SAMPLES);
    TEST_ASSERT_EQUAL(1, s_doorAtSend); /* cờ đã = 1 lúc gửi EVT_DOOR_OPEN */
    tick(0, 0, DOOR_DEBOUNCE_SAMPLES);
    TEST_ASSERT_EQUAL(0, s_doorAtSend); /* cờ đã = 0 lúc gửi EVT_DOOR_CLOSED */
}

static void test_door_init_sets_flag_before_event(void)
{
    s_doorAtSend = 0xFFu;
    queues_init();
    InputCore_Init(1);
    TEST_ASSERT_EQUAL(1, s_doorAtSend);
}

static void test_door_bounce_rejected(void)
{
    for (unsigned i = 0; i < 40; i++) {
        tick(0, (uint8_t)(i % 2u), 1);
    }
    TEST_ASSERT_EQUAL(0, count_events());
    TEST_ASSERT_EQUAL(0, g_doorOpen);
}

static void test_door_open_then_close(void)
{
    InputEvt_t e;
    tick(0, 1, DOOR_DEBOUNCE_SAMPLES);
    TEST_ASSERT_EQUAL(1, g_doorOpen);
    tick(0, 0, DOOR_DEBOUNCE_SAMPLES);
    TEST_ASSERT_EQUAL(0, g_doorOpen);
    TEST_ASSERT_EQUAL(2, count_events());
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL(EVT_DOOR_OPEN, e.type);
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL(EVT_DOOR_CLOSED, e.type);
}

/* ---- khẩn cấp ------------------------------------------------------------ */
/* Nhả ổn định: đủ mẫu "nhả" để nút nạp lại (không đổi đồng hồ). */
static void emerg_release(unsigned n)
{
    for (unsigned i = 0; i < n; i++) {
        InputCore_EmergencyLevel(0);
    }
}

static void test_emergency_first_press_accepted_even_at_tick_0(void)
{
    InputEvt_t e;
    fake_time_set(0);
    queues_init();
    InputCore_Init(0);
    drain();
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(1, count_events());

    queues_init();
    InputCore_Init(0);
    drain();
    fake_time_set(150); /* < EMERG_DEBOUNCE_MS sau khởi động */
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(1, count_events());
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL(EVT_EMERGENCY, e.type);
}

static void test_emergency_ignored_inside_window(void)
{
    fake_time_set(1000);
    InputCore_OnEmergency();
    fake_time_advance(EMERG_DEBOUNCE_MS - 1u);
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(1, count_events());
}

static void test_emergency_accepted_at_window(void)
{
    fake_time_set(1000);
    InputCore_OnEmergency();
    fake_time_advance(EMERG_DEBOUNCE_MS);
    emerg_release(DOOR_DEBOUNCE_SAMPLES);
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(2, count_events());
}

static void test_dropped_irq_does_not_shift_window(void)
{
    fake_time_set(1000);
    InputCore_OnEmergency();
    fake_time_advance(EMERG_DEBOUNCE_MS / 2u);
    InputCore_OnEmergency(); /* bị bỏ, không dời mốc */
    fake_time_advance(EMERG_DEBOUNCE_MS / 2u);
    emerg_release(DOOR_DEBOUNCE_SAMPLES);
    InputCore_OnEmergency(); /* đúng t + EMERG_DEBOUNCE_MS */
    TEST_ASSERT_EQUAL(2, count_events());
}

static void test_emergency_sets_flag_and_event(void)
{
    InputEvt_t e;
    g_emergencyReq = 0;
    s_emergAtSend = 0xFFu;
    fake_time_set(500);
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(1, g_emergencyReq);
    TEST_ASSERT_EQUAL(1, s_emergAtSend); /* cờ đã = 1 lúc gửi EVT_EMERGENCY */
    TEST_ASSERT_TRUE(pop(&e));
    TEST_ASSERT_EQUAL(EVT_EMERGENCY, e.type);
}

static void test_emergency_survives_tick_wrap(void)
{
    fake_time_set(0xFFFFFF00u);
    InputCore_OnEmergency();
    fake_time_advance(EMERG_DEBOUNCE_MS - 1u); /* vẫn trong cửa sổ, chưa tràn */
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(1, count_events());
    fake_time_advance(1u + EMERG_DEBOUNCE_MS); /* qua tràn 32 bit */
    emerg_release(DOOR_DEBOUNCE_SAMPLES);
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(2, count_events());
}

/* Tràn tick NẰM TRONG cửa sổ: mốc ở 0xFFFFFFF0, now = mốc + 100 đã tràn về 0x54. */
static void test_emergency_window_across_wrap(void)
{
    fake_time_set(0xFFFFFFF0u);
    InputCore_OnEmergency();
    emerg_release(DOOR_DEBOUNCE_SAMPLES); /* armed lại: chỉ còn cửa sổ thời gian chặn */
    fake_time_advance(100u);
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(1, count_events());
    fake_time_advance(EMERG_DEBOUNCE_MS - 100u);
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(2, count_events());
}

static void test_emergency_hold_long_then_release_bounce_sends_one(void)
{
    fake_time_set(1000);
    InputCore_OnEmergency();
    for (unsigned i = 0; i < 100; i++) { /* giữ nút ~1 s */
        fake_time_advance(KEYPAD_SCAN_MS);
        InputCore_EmergencyLevel(1);
    }
    fake_time_advance(1000u);
    InputCore_OnEmergency(); /* cạnh rung khi nhả, chưa có mẫu nhả ổn định */
    TEST_ASSERT_EQUAL(1, count_events());
}

static void test_emergency_rearms_after_stable_release(void)
{
    fake_time_set(1000);
    InputCore_OnEmergency();
    emerg_release(DOOR_DEBOUNCE_SAMPLES);
    fake_time_advance(EMERG_DEBOUNCE_MS);
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(2, count_events());
}

static void test_emergency_not_rearmed_by_short_release(void)
{
    fake_time_set(1000);
    InputCore_OnEmergency();
    emerg_release(DOOR_DEBOUNCE_SAMPLES - 1u);
    InputCore_EmergencyLevel(1); /* nhấn lại: đếm về 0 */
    emerg_release(DOOR_DEBOUNCE_SAMPLES - 1u);
    fake_time_advance(EMERG_DEBOUNCE_MS);
    InputCore_OnEmergency();
    TEST_ASSERT_EQUAL(1, count_events());
}

/* ---- hàng đợi đầy -------------------------------------------------------- */
static void test_queue_full_does_not_crash(void)
{
    for (unsigned i = 0; i < Q_KEYPAD_LEN + 2u; i++) {
        tap((char)('0' + (i % 10u)));
    }
    InputCore_OnEmergency();
    TEST_ASSERT_TRUE(xKeypadQueue.dropped > 0);
    TEST_ASSERT_EQUAL(Q_KEYPAD_LEN, count_events());
}

int main(void)
{
    TEST_BEGIN();
    RUN_TEST(test_key_fires_on_nth_sample);
    RUN_TEST(test_hold_does_not_repeat);
    RUN_TEST(test_bounce_rejected);
    RUN_TEST(test_release_sends_nothing);
    RUN_TEST(test_press_release_press_sends_two);
    RUN_TEST(test_direct_key_change);
    RUN_TEST(test_direct_change_then_release_then_press);
    RUN_TEST(test_all_16_keys_in_order);
    RUN_TEST(test_door_init_publishes_once);
    RUN_TEST(test_door_needs_n_samples);
    RUN_TEST(test_door_sets_flag_before_event);
    RUN_TEST(test_door_init_sets_flag_before_event);
    RUN_TEST(test_door_bounce_rejected);
    RUN_TEST(test_door_open_then_close);
    RUN_TEST(test_emergency_first_press_accepted_even_at_tick_0);
    RUN_TEST(test_emergency_ignored_inside_window);
    RUN_TEST(test_emergency_accepted_at_window);
    RUN_TEST(test_dropped_irq_does_not_shift_window);
    RUN_TEST(test_emergency_sets_flag_and_event);
    RUN_TEST(test_emergency_survives_tick_wrap);
    RUN_TEST(test_emergency_window_across_wrap);
    RUN_TEST(test_emergency_hold_long_then_release_bounce_sends_one);
    RUN_TEST(test_emergency_rearms_after_stable_release);
    RUN_TEST(test_emergency_not_rearmed_by_short_release);
    RUN_TEST(test_queue_full_does_not_crash);
    return TEST_END();
}
