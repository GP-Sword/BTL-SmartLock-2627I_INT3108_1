/**
 * @file    test_security.c
 * @brief   Unit test máy trạng thái Task 2. OWNER: Task 2.
 *          Foundation chỉ để sẵn khung + 1 test cho stub; Task 2 thêm test theo bảng
 *          chuyển trạng thái trong guide (mục 7, Task 2).
 */
#include "fake_time.h"
#include "mini_test.h"

#include "app_config.h"
#include "queues.h"
#include "security.h"

void setUp(void)
{
    fake_time_set(0);
    queues_init();
    Security_Init();
}

void tearDown(void) {}

/* Gõ một chuỗi phím như thể từ Task 1, mỗi phím chạy Security_Process() một lần. */
static void press(const char *keys)
{
    for (; *keys; keys++) {
        InputEvt_t e = { .type = EVT_KEY, .key = *keys };
        Q_SendInput(e);
        Security_Process();
    }
}

/* Xả xActuatorQueue, trả về true nếu thấy lệnh `want`. */
static int act_seen(ActCmd_t want)
{
    ActCmd_t c;
    int seen = 0;
    while (Q_RecvAct(&c)) {
        if (c == want) {
            seen = 1;
        }
    }
    return seen;
}

static void test_starts_idle_and_consumes_events(void)
{
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    press("12");
    TEST_ASSERT_EQUAL(0, fifo_count(&xKeypadQueue));
}

/* Ví dụ test Task 2 cần làm cho qua. Bỏ comment trong RUN_TEST khi bắt đầu Task 2. */
static void test_correct_pin_unlocks(void)
{
    press(PIN_MASTER_DEFAULT "#");
    TEST_ASSERT_EQUAL(STATE_UNLOCKED, Security_GetState());
    TEST_ASSERT_TRUE(act_seen(ACT_UNLOCK));
}

static void test_three_failures_lock_out(void)
{
    press("0000#1111#2222#");
    TEST_ASSERT_EQUAL(STATE_LOCKED_OUT, Security_GetState());
    fake_time_advance(LOCKOUT_MS - 1000);
    Security_Process();
    TEST_ASSERT_EQUAL(STATE_LOCKED_OUT, Security_GetState());
    fake_time_advance(1500);
    Security_Process();
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
}

int main(void)
{
    TEST_BEGIN();
    RUN_TEST(test_starts_idle_and_consumes_events);
    /* TODO(Task 2): bật các test dưới đây khi máy trạng thái xong */
    (void)test_correct_pin_unlocks;
    (void)test_three_failures_lock_out;
    /* RUN_TEST(test_correct_pin_unlocks); */
    /* RUN_TEST(test_three_failures_lock_out); */
    return TEST_END();
}
