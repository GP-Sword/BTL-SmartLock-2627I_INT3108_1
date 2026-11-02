/**
 * @file    test_security.c
 * @brief   Unit test máy trạng thái Task 2. OWNER: Task 2.
 *          Phủ từng dòng của bảng chuyển trạng thái trong guide (mục 6, Task 2).
 */
#include "fake_time.h"
#include "mini_test.h"

#include "app_config.h"
#include "queues.h"
#include "security.h"


/* Bộ ghi: sau mỗi Security_Process() xả cả 3 hàng đợi ra mảng, giống Task 3/4 tiêu thụ
 * ở firmware (nếu không hàng đợi 8 chỗ sẽ tràn trong test dài). */
#define REC_MAX 128
static ActCmd_t recAct[REC_MAX];
static UIMsg_t  recUI[REC_MAX];
static LogEvt_t recLog[REC_MAX];
static int      nAct, nUI, nLog;

static void pump(void)
{
    ActCmd_t a;
    UIMsg_t  u;
    LogEvt_t l;
    Security_Process();
    while (Q_RecvAct(&a) && nAct < REC_MAX) { recAct[nAct++] = a; }
    while (Q_RecvUI(&u) && nUI < REC_MAX)   { recUI[nUI++] = u; }
    while (Q_RecvLog(&l) && nLog < REC_MAX) { recLog[nLog++] = l; }
}

/* Quên những gì đã ghi để kiểm tra "từ giờ trở đi". */
static void drain_all(void)
{
    nAct = nUI = nLog = 0;
}

void setUp(void)
{
    fake_time_set(0);
    queues_init();
    Security_Init();
    nAct = nUI = nLog = 0;
}

void tearDown(void) {}

/* Gõ một chuỗi phím như thể từ Task 1, mỗi phím chạy Security_Process() một lần. */
static void press(const char *keys)
{
    for (; *keys; keys++) {
        InputEvt_t e = { .type = EVT_KEY, .key = *keys };
        Q_SendInput(e);
        pump();
    }
}

static void send(InputEvtType_t t)
{
    InputEvt_t e = { .type = t, .key = 0 };
    Q_SendInput(e);
    pump();
}

/* Có lệnh `want` kể từ lần drain_all() gần nhất không? */
static int act_seen(ActCmd_t want)
{
    for (int i = 0; i < nAct; i++) {
        if (recAct[i] == want) {
            return 1;
        }
    }
    return 0;
}

/* Có màn `want` không; giá trị (pinLen/seconds) của lần cuối ghi vào *val. */
static int ui_seen(UIScreen_t want, int *val)
{
    int seen = 0;
    for (int i = 0; i < nUI; i++) {
        if (recUI[i].screen == want) {
            seen = 1;
            if (val) {
                *val = (want == UI_LOCKED) ? (int)recUI[i].seconds : (int)recUI[i].pinLen;
            }
        }
    }
    return seen;
}

static int log_seen(LogEvt_t want)
{
    for (int i = 0; i < nLog; i++) {
        if (recLog[i] == want) {
            return 1;
        }
    }
    return 0;
}

static void lock_out(void)
{
    press("0000#1111#2222#");
}

static void test_starts_idle_and_consumes_events(void)
{
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    press("12");
    TEST_ASSERT_EQUAL(0, fifo_count(&xKeypadQueue));
}

static void test_digit_enters_pin_and_beeps(void)
{
    int v = -1;
    press("1");
    TEST_ASSERT_EQUAL(STATE_ENTERING_PIN, Security_GetState());
    TEST_ASSERT_TRUE(ui_seen(UI_ENTER_PIN, &v));
    TEST_ASSERT_EQUAL(1, v);
    press("23");
    TEST_ASSERT_TRUE(ui_seen(UI_ENTER_PIN, &v));
    TEST_ASSERT_EQUAL(3, v);
}

static void test_digit_beeps(void)
{
    press("1");
    TEST_ASSERT_TRUE(act_seen(ACT_BEEP_TICK));
}

static void test_extra_digits_ignored(void)
{
    int v = -1;
    press("1234567");
    TEST_ASSERT_TRUE(ui_seen(UI_ENTER_PIN, &v));
    TEST_ASSERT_EQUAL(PIN_MAX_LEN, v);
}

static void test_star_clears_to_idle(void)
{
    press("12*");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    TEST_ASSERT_TRUE(ui_seen(UI_IDLE, NULL));
    press(PIN_MASTER_DEFAULT "#");      /* bộ đệm đã sạch */
    TEST_ASSERT_EQUAL(STATE_UNLOCKED, Security_GetState());
}

static void test_C_ignored_without_admin(void)
{
    press("12C");
    TEST_ASSERT_EQUAL(STATE_ENTERING_PIN, Security_GetState());
}

static void test_correct_pin_unlocks(void)
{
    press(PIN_MASTER_DEFAULT "#");
    TEST_ASSERT_EQUAL(STATE_UNLOCKED, Security_GetState());
    TEST_ASSERT_TRUE(act_seen(ACT_UNLOCK));
}

static void test_correct_pin_ui_and_log(void)
{
    press(PIN_MASTER_DEFAULT "#");
    TEST_ASSERT_TRUE(ui_seen(UI_GRANTED, NULL));
    TEST_ASSERT_TRUE(log_seen(LOG_UNLOCK_OK));
}

static void test_wrong_pin_returns_idle_and_logs(void)
{
    press("9999#");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    TEST_ASSERT_TRUE(log_seen(LOG_UNLOCK_FAIL));
}

static void test_wrong_pin_does_not_unlock(void)
{
    press("9999#");
    TEST_ASSERT_FALSE(act_seen(ACT_UNLOCK));
}

static void test_length_mismatch_rejected(void)
{
    press("123#");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    press("12345#");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
}

static void test_success_resets_fail_count(void)
{
    press("0000#1111#" PIN_MASTER_DEFAULT "#");
    send(EVT_RELOCKED);
    press("0000#1111#");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
}

static void test_three_failures_lock_out(void)
{
    lock_out();
    TEST_ASSERT_EQUAL(STATE_LOCKED_OUT, Security_GetState());
    TEST_ASSERT_TRUE(act_seen(ACT_ALARM_ON));
    fake_time_advance(LOCKOUT_MS - 1000);
    pump();
    TEST_ASSERT_EQUAL(STATE_LOCKED_OUT, Security_GetState());
    fake_time_advance(1500);
    pump();
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    TEST_ASSERT_TRUE(act_seen(ACT_ALARM_OFF));
}

static void test_lockout_shows_countdown(void)
{
    int v = -1;
    lock_out();
    TEST_ASSERT_TRUE(ui_seen(UI_LOCKED, &v));
    TEST_ASSERT_EQUAL(LOCKOUT_MS / 1000, v);
    fake_time_advance(3000);
    pump();
    TEST_ASSERT_TRUE(ui_seen(UI_LOCKED, &v));
    TEST_ASSERT_EQUAL(LOCKOUT_MS / 1000 - 3, v);
}

static void test_lockout_ignores_keys_without_beep(void)
{
    lock_out();
    drain_all();
    press("1234#A5");
    TEST_ASSERT_EQUAL(STATE_LOCKED_OUT, Security_GetState());
    TEST_ASSERT_FALSE(act_seen(ACT_BEEP_TICK));
}

static void test_lockout_end_resets_fail_count(void)
{
    lock_out();
    fake_time_advance(LOCKOUT_MS);
    pump();
    press("0000#1111#");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
}

static void test_relocked_returns_idle(void)
{
    press(PIN_MASTER_DEFAULT "#");
    drain_all();
    send(EVT_RELOCKED);
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    TEST_ASSERT_TRUE(ui_seen(UI_IDLE, NULL));
}

static void test_admin_login_and_cancel(void)
{
    int v = -1;
    press("A");
    TEST_ASSERT_EQUAL(STATE_ENTERING_PIN, Security_GetState());
    TEST_ASSERT_TRUE(ui_seen(UI_ADMIN_LOGIN, &v));
    TEST_ASSERT_EQUAL(0, v);
    TEST_ASSERT_FALSE(ui_seen(UI_ENTER_PIN, NULL));
    press("C");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    press("A1*");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
}

static void test_admin_login_digits_use_admin_screen(void)
{
    int v = -1;
    press("A12");
    TEST_ASSERT_TRUE(ui_seen(UI_ADMIN_LOGIN, &v));
    TEST_ASSERT_EQUAL(2, v);
    TEST_ASSERT_FALSE(ui_seen(UI_ENTER_PIN, NULL));
    drain_all();
    send(EVT_DOOR_OPEN);
    send(EVT_DOOR_CLOSED);
    TEST_ASSERT_TRUE(ui_seen(UI_ADMIN_LOGIN, &v));   /* vẽ lại đúng màn admin */
}

static void test_wrong_pin_shows_denied_with_retries(void)
{
    int v = -1;
    press("9999#");
    TEST_ASSERT_TRUE(ui_seen(UI_DENIED, &v));
    TEST_ASSERT_EQUAL(MAX_FAIL_COUNT - 1, v);
    press("9999#");
    TEST_ASSERT_TRUE(ui_seen(UI_DENIED, &v));
    TEST_ASSERT_EQUAL(MAX_FAIL_COUNT - 2, v);
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
}

static void test_denied_returns_to_idle_after_timeout(void)
{
    press("9999#");
    drain_all();
    fake_time_advance(DENIED_SHOW_MS - 1);
    pump();
    TEST_ASSERT_FALSE(ui_seen(UI_IDLE, NULL));
    fake_time_advance(1);
    pump();
    TEST_ASSERT_TRUE(ui_seen(UI_IDLE, NULL));
    drain_all();
    fake_time_advance(DENIED_SHOW_MS);
    pump();
    TEST_ASSERT_FALSE(ui_seen(UI_IDLE, NULL));   /* chỉ một lần */
}

static void test_denied_cleared_by_any_key(void)
{
    press("9999#");
    drain_all();
    press("#");
    TEST_ASSERT_TRUE(ui_seen(UI_IDLE, NULL));
    press("9999#");
    drain_all();
    press("5");
    TEST_ASSERT_FALSE(ui_seen(UI_IDLE, NULL));   /* số đã tự đổi sang ENTER_PIN */
    fake_time_advance(DENIED_SHOW_MS);
    pump();
    TEST_ASSERT_FALSE(ui_seen(UI_IDLE, NULL));   /* và không bị timer kéo về IDLE */
}

static void test_denied_timer_cancelled_by_emergency(void)
{
    press("9999#");
    send(EVT_EMERGENCY);
    drain_all();
    fake_time_advance(DENIED_SHOW_MS);
    pump();
    TEST_ASSERT_FALSE(ui_seen(UI_IDLE, NULL));
}

static void test_last_failure_shows_locked_not_denied(void)
{
    press("0000#1111#");
    drain_all();
    press("2222#");
    TEST_ASSERT_FALSE(ui_seen(UI_DENIED, NULL));
}

static void test_admin_wrong_pin_counts_fail(void)
{
    press("A0000#");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    TEST_ASSERT_TRUE(log_seen(LOG_UNLOCK_FAIL));
    press("A1111#A2222#");
    TEST_ASSERT_EQUAL(STATE_LOCKED_OUT, Security_GetState());
}

static void test_lockout_ignores_admin_key(void)
{
    lock_out();
    press("A");
    TEST_ASSERT_EQUAL(STATE_LOCKED_OUT, Security_GetState());
}

static void test_admin_change_pin(void)
{
    int v = -1;
    press("A" PIN_MASTER_DEFAULT "#");
    TEST_ASSERT_EQUAL(STATE_ADMIN_RESET, Security_GetState());
    TEST_ASSERT_FALSE(act_seen(ACT_UNLOCK));
    press("5678");
    TEST_ASSERT_TRUE(ui_seen(UI_ADMIN, &v));
    TEST_ASSERT_EQUAL(4, v);
    press("#");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    press(PIN_MASTER_DEFAULT "#");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());   /* PIN cũ hết hiệu lực */
    press("5678#");
    TEST_ASSERT_EQUAL(STATE_UNLOCKED, Security_GetState());
}

static void test_admin_short_pin_ignored_and_star_clears(void)
{
    int v = -1;
    press("A" PIN_MASTER_DEFAULT "#" "12#");
    TEST_ASSERT_EQUAL(STATE_ADMIN_RESET, Security_GetState());   /* ít số: bỏ qua */
    press("*");
    TEST_ASSERT_TRUE(ui_seen(UI_ADMIN, &v));
    TEST_ASSERT_EQUAL(0, v);
    press("98765#");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    press("98765#");
    TEST_ASSERT_EQUAL(STATE_UNLOCKED, Security_GetState());
}

static void test_admin_cancel_keeps_pin(void)
{
    press("A" PIN_MASTER_DEFAULT "#" "5678C");
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
    press(PIN_MASTER_DEFAULT "#");
    TEST_ASSERT_EQUAL(STATE_UNLOCKED, Security_GetState());
}

static void test_emergency_unlocks_from_idle(void)
{
    send(EVT_EMERGENCY);
    TEST_ASSERT_EQUAL(STATE_UNLOCKED, Security_GetState());
    TEST_ASSERT_TRUE(ui_seen(UI_GRANTED, NULL));
    TEST_ASSERT_TRUE(log_seen(LOG_UNLOCK_OK));
}

static void test_emergency_cancels_lockout(void)
{
    lock_out();
    drain_all();
    send(EVT_EMERGENCY);
    TEST_ASSERT_EQUAL(STATE_UNLOCKED, Security_GetState());
    TEST_ASSERT_TRUE(act_seen(ACT_ALARM_OFF));
    send(EVT_RELOCKED);
    press("0000#1111#");                 /* failCount đã về 0 */
    TEST_ASSERT_EQUAL(STATE_IDLE, Security_GetState());
}

static void test_door_open_too_long_raises_alarm(void)
{
    send(EVT_DOOR_OPEN);
    fake_time_advance(DOOR_AJAR_MS - 1);
    pump();
    TEST_ASSERT_FALSE(act_seen(ACT_ALARM_ON));
    fake_time_advance(1);
    pump();
    TEST_ASSERT_TRUE(ui_seen(UI_DOOR_AJAR, NULL));
    TEST_ASSERT_TRUE(act_seen(ACT_ALARM_ON));
}

static void test_door_close_stops_alarm(void)
{
    send(EVT_DOOR_OPEN);
    fake_time_advance(DOOR_AJAR_MS);
    pump();
    drain_all();
    send(EVT_DOOR_CLOSED);
    TEST_ASSERT_TRUE(act_seen(ACT_ALARM_OFF));
}

static void test_door_close_redraws_current_screen(void)
{
    int v = -1;
    press("12");
    send(EVT_DOOR_OPEN);
    drain_all();
    send(EVT_DOOR_CLOSED);
    TEST_ASSERT_TRUE(ui_seen(UI_ENTER_PIN, &v));
    TEST_ASSERT_EQUAL(2, v);
}

static void test_door_close_before_timeout_no_alarm(void)
{
    send(EVT_DOOR_OPEN);
    fake_time_advance(DOOR_AJAR_MS - 1);
    send(EVT_DOOR_CLOSED);
    fake_time_advance(DOOR_AJAR_MS);
    pump();
    TEST_ASSERT_FALSE(act_seen(ACT_ALARM_ON));
}

static void test_door_close_during_lockout_keeps_alarm(void)
{
    lock_out();
    send(EVT_DOOR_OPEN);
    fake_time_advance(DOOR_AJAR_MS);
    pump();
    drain_all();
    send(EVT_DOOR_CLOSED);
    TEST_ASSERT_FALSE(act_seen(ACT_ALARM_OFF));     /* còi phạt vẫn phải kêu */
    TEST_ASSERT_EQUAL(STATE_LOCKED_OUT, Security_GetState());
}

int main(void)
{
    TEST_BEGIN();
    RUN_TEST(test_starts_idle_and_consumes_events);
    RUN_TEST(test_digit_enters_pin_and_beeps);
    RUN_TEST(test_digit_beeps);
    RUN_TEST(test_extra_digits_ignored);
    RUN_TEST(test_star_clears_to_idle);
    RUN_TEST(test_C_ignored_without_admin);
    RUN_TEST(test_correct_pin_unlocks);
    RUN_TEST(test_correct_pin_ui_and_log);
    RUN_TEST(test_wrong_pin_returns_idle_and_logs);
    RUN_TEST(test_wrong_pin_does_not_unlock);
    RUN_TEST(test_length_mismatch_rejected);
    RUN_TEST(test_success_resets_fail_count);
    RUN_TEST(test_three_failures_lock_out);
    RUN_TEST(test_lockout_shows_countdown);
    RUN_TEST(test_lockout_ignores_keys_without_beep);
    RUN_TEST(test_lockout_end_resets_fail_count);
    RUN_TEST(test_relocked_returns_idle);
    RUN_TEST(test_admin_login_and_cancel);
    RUN_TEST(test_admin_login_digits_use_admin_screen);
    RUN_TEST(test_wrong_pin_shows_denied_with_retries);
    RUN_TEST(test_denied_returns_to_idle_after_timeout);
    RUN_TEST(test_denied_cleared_by_any_key);
    RUN_TEST(test_denied_timer_cancelled_by_emergency);
    RUN_TEST(test_last_failure_shows_locked_not_denied);
    RUN_TEST(test_admin_wrong_pin_counts_fail);
    RUN_TEST(test_lockout_ignores_admin_key);
    RUN_TEST(test_admin_change_pin);
    RUN_TEST(test_admin_short_pin_ignored_and_star_clears);
    RUN_TEST(test_admin_cancel_keeps_pin);
    RUN_TEST(test_emergency_unlocks_from_idle);
    RUN_TEST(test_emergency_cancels_lockout);
    RUN_TEST(test_door_open_too_long_raises_alarm);
    RUN_TEST(test_door_close_stops_alarm);
    RUN_TEST(test_door_close_redraws_current_screen);
    RUN_TEST(test_door_close_before_timeout_no_alarm);
    RUN_TEST(test_door_close_during_lockout_keeps_alarm);
    return TEST_END();
}
