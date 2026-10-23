/**
 * @file    test_common.c
 * @brief   Unit test cho app/common: fifo, hàng đợi Q_*, soft_timer. OWNER: foundation.
 */
#include "fake_time.h"
#include "mini_test.h"

#include "app_config.h"
#include "fifo.h"
#include "queues.h"
#include "soft_timer.h"

void setUp(void)
{
    fake_time_set(1000);
    queues_init();
}

void tearDown(void) {}

/* ---- fifo ---------------------------------------------------------------- */
static void test_fifo_push_pop_order(void)
{
    int storage[4];
    Fifo_t f;
    fifo_init(&f, storage, sizeof(int), 4);

    for (int i = 1; i <= 3; i++) {
        TEST_ASSERT_TRUE(fifo_push(&f, &i));
    }
    TEST_ASSERT_EQUAL(3, fifo_count(&f));

    int v;
    for (int i = 1; i <= 3; i++) {
        TEST_ASSERT_TRUE(fifo_pop(&f, &v));
        TEST_ASSERT_EQUAL(i, v);
    }
    TEST_ASSERT_FALSE(fifo_pop(&f, &v));
}

static void test_fifo_full_drops_and_counts(void)
{
    int storage[2];
    Fifo_t f;
    fifo_init(&f, storage, sizeof(int), 2);
    int a = 1, b = 2, c = 3;
    TEST_ASSERT_TRUE(fifo_push(&f, &a));
    TEST_ASSERT_TRUE(fifo_push(&f, &b));
    TEST_ASSERT_FALSE(fifo_push(&f, &c));
    TEST_ASSERT_EQUAL(1, f.dropped);
}

static void test_fifo_wraps_around(void)
{
    int storage[3];
    Fifo_t f;
    fifo_init(&f, storage, sizeof(int), 3);
    int v;
    for (int round = 0; round < 10; round++) {
        TEST_ASSERT_TRUE(fifo_push(&f, &round));
        TEST_ASSERT_TRUE(fifo_pop(&f, &v));
        TEST_ASSERT_EQUAL(round, v);
    }
    TEST_ASSERT_EQUAL(0, fifo_count(&f));
}

/* ---- Q_* ----------------------------------------------------------------- */
static void test_queue_input_roundtrip(void)
{
    InputEvt_t in = { .type = EVT_KEY, .key = '7' }, out;
    TEST_ASSERT_TRUE(Q_SendInput(in));
    TEST_ASSERT_TRUE(Q_RecvInput(&out));
    TEST_ASSERT_EQUAL(EVT_KEY, out.type);
    TEST_ASSERT_EQUAL_CHAR('7', out.key);
    TEST_ASSERT_FALSE(Q_RecvInput(&out));
}

static void test_queue_ui_value_goes_to_right_field(void)
{
    UIMsg_t m;
    TEST_ASSERT_TRUE(Q_SendUI(UI_ENTER_PIN, 3));
    TEST_ASSERT_TRUE(Q_SendUI(UI_LOCKED, 57));
    TEST_ASSERT_TRUE(Q_RecvUI(&m));
    TEST_ASSERT_EQUAL(UI_ENTER_PIN, m.screen);
    TEST_ASSERT_EQUAL(3, m.pinLen);
    TEST_ASSERT_TRUE(Q_RecvUI(&m));
    TEST_ASSERT_EQUAL(UI_LOCKED, m.screen);
    TEST_ASSERT_EQUAL(57, m.seconds);
}

static void test_keypad_queue_capacity(void)
{
    InputEvt_t e = { .type = EVT_KEY, .key = '1' };
    for (unsigned i = 0; i < Q_KEYPAD_LEN; i++) {
        TEST_ASSERT_TRUE(Q_SendInput(e));
    }
    TEST_ASSERT_FALSE(Q_SendInput(e));
}

static void test_names(void)
{
    TEST_ASSERT_EQUAL_STRING("UNLOCK", act_name(ACT_UNLOCK));
    TEST_ASSERT_EQUAL_STRING("ALARM_OFF", act_name(ACT_ALARM_OFF));
    TEST_ASSERT_EQUAL_STRING("DOOR_AJAR", ui_name(UI_DOOR_AJAR));
}

/* ---- soft_timer ---------------------------------------------------------- */
static void test_soft_timer_fires_once(void)
{
    SoftTimer_t t;
    soft_timer_start(&t, RELOCK_MS);
    fake_time_advance(RELOCK_MS - 1);
    TEST_ASSERT_FALSE(soft_timer_expired(&t));
    TEST_ASSERT_EQUAL(1, soft_timer_remaining(&t));
    fake_time_advance(1);
    TEST_ASSERT_TRUE(soft_timer_expired(&t));
    TEST_ASSERT_FALSE(soft_timer_expired(&t)); /* chỉ 1 lần */
    TEST_ASSERT_FALSE(soft_timer_running(&t));
}

static void test_soft_timer_survives_tick_wrap(void)
{
    SoftTimer_t t;
    fake_time_set(0xFFFFFF00u);
    soft_timer_start(&t, 0x200u);
    fake_time_advance(0x1FFu);
    TEST_ASSERT_FALSE(soft_timer_expired(&t));
    fake_time_advance(1u);
    TEST_ASSERT_TRUE(soft_timer_expired(&t));
}

static void test_soft_timer_stop(void)
{
    SoftTimer_t t;
    soft_timer_start(&t, 100);
    soft_timer_stop(&t);
    fake_time_advance(1000);
    TEST_ASSERT_FALSE(soft_timer_expired(&t));
}

int main(void)
{
    TEST_BEGIN();
    RUN_TEST(test_fifo_push_pop_order);
    RUN_TEST(test_fifo_full_drops_and_counts);
    RUN_TEST(test_fifo_wraps_around);
    RUN_TEST(test_queue_input_roundtrip);
    RUN_TEST(test_queue_ui_value_goes_to_right_field);
    RUN_TEST(test_keypad_queue_capacity);
    RUN_TEST(test_names);
    RUN_TEST(test_soft_timer_fires_once);
    RUN_TEST(test_soft_timer_survives_tick_wrap);
    RUN_TEST(test_soft_timer_stop);
    return TEST_END();
}
