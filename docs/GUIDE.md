# GUIDE – Tích hợp vào foundation Smart Lock (STM32F103C8, bare-metal)

Tài liệu này dành cho dev **và agent của dev**. Mục tiêu: mang phần code mỗi người đã tự tìm hiểu
(FreeRTOS, Arduino, HAL mẫu, code thử trên board…) vào foundation này mà không phá **hợp đồng** chung,
để 4 branch merge vào `main` không xung đột và CI xanh.

Cách đọc:

- Agent: đọc §1 → §5, rồi **chỉ** phần Task của mình ở §6. §7–§10 là tra cứu khi cần.
- Mọi thứ trong tài liệu khớp với code tại thời điểm tag `v0.1-foundation`. Khi code và tài liệu lệch nhau,
  **code là đúng**; sửa tài liệu trong cùng PR.

Từ khóa dùng xuyên suốt:

- **hợp đồng**: những gì 4 task cùng dựa vào: `app/common/app_types.h`, `queues.h`, `app_time.h`,
  `soft_timer.h`, `trace.h`, `board_pins.h`, `app_config.h`, `hal_callbacks.c`, `uart_port.h`, tên hàm trong header
  của mỗi module, chuỗi vết, `diagram.json`, `smart_lock.ioc`. Đổi hợp đồng = PR `iface/...` (§9).
- **stub**: bản rỗng foundation để sẵn trong thư mục của mỗi task. Task thay ruột stub, giữ nguyên chữ ký hàm.
- **vết**: dòng in ra UART khi bật `APP_TRACE` (preset `debug`, `test`). Scenario CI bắt theo vết.

---

## §1. Quy trình tích hợp (agent làm theo thứ tự)

1. **Xác định phạm vi.** Đọc phần Task của mình ở §6: thư mục sở hữu, hàm phải cài, hằng số, vết.
   Xong khi: liệt kê được đúng các file được phép sửa và các hàm công khai phải giữ chữ ký.
2. **Kiểm kê code có sẵn.** Đọc toàn bộ code tự tìm hiểu trước đây. Ghi một bảng: mỗi hàm, biến toàn cục,
   ngắt/callback, timer, task/thread, hàng đợi, lệnh chờ (`delay`, `HAL_Delay`, `vTaskDelay`…), lệnh in, chân GPIO,
   ngoại vi đã dùng.
   Xong khi: mọi dòng trong code cũ thuộc về một mục của bảng.
3. **Ánh xạ.** Với mỗi mục trong bảng, ghi cách làm tương ứng theo §3 và kiểm tra theo §4.
   Xong khi: không còn mục nào "chưa rõ". Mục nào cần đổi hợp đồng (thêm loại sự kiện, thêm chân, thêm callback,
   đổi hằng số chung) → dừng, mô tả cho dev để mở PR `iface/...` (§9) trước.
4. **Port.** Viết code vào `app/<module>/` (thêm file `.c/.h` tùy ý, CMake tự nhận). Giữ chữ ký hàm trong header.
   Thông số thời gian/chân lấy từ `app_config.h` / `board_pins.h`.
   Xong khi: `cmake --build --preset debug` và `--preset test` không có warning mới, không đụng file ngoài phạm vi.
5. **Kiểm thử.** Chạy đủ các mục "Thử riêng" và "Nghiệm thu" của Task (§6): unit test host (bắt buộc với Task 2),
   demo Wokwi, và viết scenario thật thay cho `# TODO` trong `tests/scenarios/<task>.test.yaml`.
   Xong khi: mọi tiêu chí nghiệm thu của Task được kiểm và ghi kết quả.
6. **PR.** Commit theo §9, điền `.github/pull_request_template.md`.
   Xong khi: CI (`host-tests`, `firmware`, `wokwi`) xanh và PR chỉ chạm file trong phạm vi.

---

## §2. Kiến trúc và hợp đồng

### 2.1 Super-loop

`Core/Src/main.c` (CubeMX sinh) gọi `App_Init()` một lần rồi `App_Loop()` liên tục.
`app/common/app.c` gọi theo thứ tự:

```c
App_Init:  queues_init → UartPort_Init → Input_Init → Security_Init → Actuator_Init
           → UI_Init → Logger_Init → in "SMARTLOCK READY 72MHz"
App_Loop:  Actuator_Process → Input_Process → Security_Process → UI_Process
           → Logger_Process → UartPort_Process → (TestHook_Process nếu APP_TEST_HOOKS)
           → nhịp tim LED PC13 (500 ms)
```

Mỗi `*_Process()` là một "task" của bản RTOS cũ: làm một phần việc ngắn rồi trả về.
Mọi chờ đợi biểu diễn bằng **mốc thời gian** (`soft_timer`, `app_elapsed_ms`), không bằng vòng chờ.

### 2.2 Luồng dữ liệu

```text
Task 1 Input ──xKeypadQueue (InputEvt_t)──▶ Task 2 Security ──xActuatorQueue (ActCmd_t)──▶ Task 3 Actuator
   │  ▲                                        │──xUIQueue (UIMsg_t)───▶ Task 4 UI (LCD)
   │  └──── EVT_RELOCKED (Task 3 gửi) ─────────│──xLogQueue (LogEvt_t)──▶ Task 4 Logger (UART)
   ├── g_emergencyReq (ISR → Task 3, không qua hàng đợi)
   └── g_doorOpen     (Task 1 ghi → Task 3 đọc)
```

Task 2 là nơi duy nhất quyết định trạng thái; Task 3 là nơi duy nhất điều khiển servo/LED xanh/buzzer;
Task 4 là nơi duy nhất dùng I2C1 và USART1.

### 2.3 API của hợp đồng (chi tiết trong comment của từng header)

| Nhu cầu | Dùng | Ghi chú |
| --- | --- | --- |
| Gửi sự kiện cho Task 2 | `Q_SendInput(InputEvt_t)` | Gọi được trong ngắt |
| Nhận sự kiện (Task 2) | `Q_RecvInput(&evt)` | In vết `KEY 5`, `DOOR OPEN`… |
| Lệnh chấp hành | `Q_SendAct(ActCmd_t)` / `Q_RecvAct(&cmd)` | In vết `ACT:UNLOCK`… |
| Màn hình | `Q_SendUI(UIScreen_t, value)` / `Q_RecvUI(&msg)` | `value` = pinLen (ENTER_PIN, ADMIN) hoặc số giây (LOCKED) |
| Nhật ký | `Q_SendLog(LogEvt_t)` / `Q_RecvLog(&evt)` | In vết `LOG:OK` / `LOG:FAIL` |
| Cờ khẩn cấp | `g_emergencyReq` | Task 1 ghi 1 trong ngắt, Task 3 đọc rồi ghi 0 |
| Trạng thái cửa | `g_doorOpen` | Task 1 ghi (đã chống rung, 1 = mở), Task 3 chỉ đọc |
| Thời gian | `app_now_ms()`, `app_elapsed_ms(since)` | Đúng cả khi tick tràn |
| Hẹn giờ one-shot | `SoftTimer_t` + `soft_timer_start/stop/expired/remaining` | `expired` trả true đúng 1 lần |
| In vết | `TRACE("fmt", ...)` | Tự thêm `\r\n`; biến mất ở preset `release` |
| In bắt buộc | `uart_write("...\r\n")` | Dùng cho dòng phải có cả ở release (log) |
| Đọc ký tự UART | `UartPort_GetChar(&c)` | Chỉ test hook dùng |
| Chân | macro `*_Pin`/`*_GPIO_Port` (`main.h`), `BOARD_*` (`board_pins.h`) | |
| Ngoại vi | `htim3` (servo), `htim2` (âm buzzer), `hi2c1` (LCD), `huart1` (UART) | Từ `tim.h`, `i2c.h`, `usart.h` |

Callback HAL (`HAL_GPIO_EXTI_Callback`, `HAL_TIM_PeriodElapsedCallback`, `HAL_UART_*Callback`) chỉ được định nghĩa
**một lần** trong `app/common/hal_callbacks.c`; file đó chuyển tiếp sang:
`Input_OnEmergencyIrq()` (PB11), `Actuator_OnToneTick()` (TIM2 4 kHz), `UartPort_OnTxCplt/OnRxByte/OnError()` (USART1).

### 2.4 Bản đồ thư mục và quyền sở hữu

| Đường dẫn | Chủ | Ghi chú |
| --- | --- | --- |
| `app/input/` | Task 1 | `input.c/.h` + file tùy thêm |
| `app/security/` | Task 2 | Logic thuần, không include HAL |
| `app/actuator/` | Task 3 | |
| `app/ui/` | Task 4 | `ui`, `logger`, `uart_port`, (+ `sw_rtc`, driver LCD) |
| `tests/host/test_input.c` | Task 1 | |
| `tests/host/test_security.c` | Task 2 | |
| `tests/scenarios/<task>.test.yaml` | Task tương ứng | |
| `app/common/`, `Core/`, `Drivers/`, `cmake/`, `CMakeLists.txt`, `CMakePresets.json`, `smart_lock.ioc`, `diagram.json`, `.github/` | foundation | Chỉ sửa qua PR `iface/...` |

---

## §3. Bảng chuyển đổi code có sẵn → foundation

| Code cũ thường gặp | Viết lại thành |
| --- | --- |
| `xTaskCreate(fn…)`, vòng `for(;;)` trong task | Ruột vòng lặp chuyển vào `Xxx_Process()`; phần khởi tạo vào `Xxx_Init()`. Biến cục bộ của task → `static` trong file |
| `void setup()` / `void loop()` (Arduino) | `setup` → `Xxx_Init()`, `loop` → `Xxx_Process()` |
| `vTaskDelay`, `vTaskDelayUntil(&t, 20)` | `if (app_elapsed_ms(t0) >= KEYPAD_SCAN_MS) { t0 += KEYPAD_SCAN_MS; ... }` (quét keypad dùng `t0 = app_now_ms()` để không quét dồn khi vòng lặp trễ; `t0 += ...` vẫn đúng cho việc cần chu kỳ cố định) |
| `HAL_Delay(ms)`, `delay(ms)` trong logic | `SoftTimer_t` + máy trạng thái nhỏ (ghi bước hiện tại vào biến `static`) |
| `HAL_Delay` trong chuỗi khởi tạo thiết bị (LCD) | Giữ nguyên, chỉ trong `Xxx_Init()` (chạy trước vòng lặp) |
| `delayMicroseconds`, chờ vài µs | Vòng `__NOP()` ngắn, chỉ cho chờ < 50 µs |
| `xQueueSend/Receive(xKeypadQueue…)` | `Q_SendInput` / `Q_RecvInput` (tương tự `Q_*` cho 3 hàng đợi kia) |
| `xQueueSendFromISR` | `Q_SendInput` (an toàn trong ngắt); các `Q_Send*` khác chỉ gọi ngoài ngắt |
| `xTimerCreate` / `xTimerStart` / callback timer | `SoftTimer_t`; trong `Process()` kiểm `soft_timer_expired()` rồi làm việc của callback |
| `vTaskNotifyGiveFromISR` / `ulTaskNotifyTake` | Cờ `volatile` của hợp đồng (`g_emergencyReq`) |
| `xSemaphore`/mutex bảo vệ biến | Không cần (một luồng). Biến chung với ngắt: `volatile` + đọc/ghi một lần |
| Tự định nghĩa `HAL_GPIO_EXTI_Callback` / `HAL_TIM_PeriodElapsedCallback` / `HAL_UART_*Callback` | Viết ruột vào hàm `*_On*()` của module (§2.3); callback đã có trong `hal_callbacks.c` |
| `printf`, `Serial.print`, `HAL_UART_Transmit` | `TRACE(...)` cho vết debug; `uart_write()` cho dòng bắt buộc. Cấm gọi trong ngắt |
| `HAL_UART_Receive_IT` để nhận lệnh thử | Test hook đã có sẵn (§5.3) |
| `HAL_RTC_GetTime/GetDate`, bật RTC/LSE | RTC phần mềm `sw_rtc` của Task 4 (§6 Task 4). Wokwi không mô phỏng RTC |
| DMA (UART/I2C) | Truyền bằng ngắt (`*_IT`) hoặc chặn ngắn có timeout. Wokwi không mô phỏng DMA |
| `analogWrite`, `Servo.write`, `tone()` | `__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, ccr)`; âm buzzer qua `Actuator_OnToneTick` |
| `LiquidCrystal_I2C` | Driver PCF8574 tự viết trên `HAL_I2C_Master_Transmit(&hi2c1, LCD_I2C_ADDR << 1, ...)` |
| `digitalRead/Write(pin)` | `HAL_GPIO_ReadPin/WritePin(X_GPIO_Port, X_Pin)` với tên từ `main.h` |
| Số "ma thuật" (60000, 15000, 1500…) | Hằng số trong `app_config.h`; thiếu hằng số → PR `iface/...` |
| Tự cấu hình chân/clock/timer (`MX_*`, `HAL_GPIO_Init`) | Đã cấu hình trong `.ioc`. Chỉ được đổi thông số runtime trong `Init()` (ví dụ `__HAL_TIM_SET_PRESCALER`); đổi chân → PR `iface/...` |
| Thêm loại sự kiện / lệnh / màn hình mới | PR `iface/...` sửa `app_types.h` + `queues.c` (tên + vết) trước |

---

## §4. Quy tắc (kiểm từng dòng code port)

1. Sửa file trong thư mục của Task mình; thêm file mới thoải mái (CMake GLOB tự nhận).
2. `Process()` trả về nhanh: chờ bằng mốc thời gian; vòng lặp có giới hạn số lần (ví dụ xử lý tối đa N phần tử).
3. Module trao đổi chỉ qua `Q_*` và 2 cờ của hợp đồng; gọi hàm công khai của module khác là sai.
4. Code trong ngắt: chỉ ghi cờ / `Q_SendInput` / đảo chân; in ấn và logic để cho `Process()`.
5. Biến dùng chung với ngắt khai báo `volatile`.
6. Hằng số lấy từ `app_config.h`, chân từ `main.h`/`board_pins.h`.
7. Task 2 chỉ include `app_time.h`, `queues.h`, `app_config.h`, `trace.h` (để chạy được unit test trên PC).
8. Chuỗi vết và định dạng log giữ đúng §5.4 – scenario và người khác dựa vào chúng.
9. Giữ cảnh báo build bằng 0 (CMake bật `-Wall -Wextra -Wshadow` cho `app/`).
10. Guardrail: không bật RTC/DMA/IWDG, không sửa `Core/` ngoài khối `USER CODE` (và chỉ qua PR `iface/...`).

---

## §5. Build, chạy, kiểm thử

### 5.1 Lệnh

```bash
cmake --preset debug && cmake --build --preset debug      # build/debug/smart_lock.elf  (Wokwi hằng ngày)
cmake --preset test  && cmake --build --preset test       # build/test/...  (test hook + vết, timeout ngắn)
cmake --preset release && cmake --build --preset release  # board thật (BUZZER_ACTIVE=1, không vết)

cmake -S tests/host -B build/host -G Ninja && cmake --build build/host
ctest --test-dir build/host --output-on-failure           # unit test trên PC (cần gcc + ninja của máy)
```

| Preset | Định nghĩa | `LOCKOUT_MS` / `DOOR_AJAR_MS` |
| --- | --- | --- |
| `debug` | `APP_TRACE` | 60000 / 15000 |
| `test` | `APP_TRACE`, `APP_TEST_HOOKS` | 6000 / 3000 |
| `release` | `BUZZER_ACTIVE=1` | 60000 / 15000 |

### 5.2 Wokwi trong VS Code

`F1` → **Wokwi: Start Simulator**: nạp `build/debug/smart_lock.elf` (xem `wokwi.toml`), Wokwi tự nạp lại sau mỗi
lần build. Debug: **Wokwi: Start Simulator and Wait for Debugger** rồi `F5` (cấu hình "Wokwi GDB").
Chạy bản `test` trong VS Code: tạm đổi 2 dòng đường dẫn trong `wokwi.toml` sang `build/test/smart_lock.elf`,
không commit thay đổi đó. Khởi động đúng: Serial in `SMARTLOCK READY 72MHz`, LED PC13 nhấp nháy.

### 5.3 Test hook (chỉ preset `test`)

Gõ trên Serial Monitor; chữ IN HOA là phím, chữ thường là lệnh:

| Ký tự | Tác dụng |
| --- | --- |
| `0-9 A-D * #` | `Q_SendInput({EVT_KEY, ký tự})` – thay keypad |
| `o` / `c` | `g_doorOpen = 1/0` rồi `EVT_DOOR_OPEN` / `EVT_DOOR_CLOSED` |
| `e` | `g_emergencyReq = 1` rồi `EVT_EMERGENCY` – thay nút khẩn cấp |
| `u` `l` `t` `a` `s` | `Q_SendAct`: UNLOCK, LOCK, BEEP_TICK, ALARM_ON, ALARM_OFF |

### 5.4 Vết và log (một phần của hợp đồng)

| Nguồn | Dòng in |
| --- | --- |
| Khởi động (`app.c`) | `SMARTLOCK READY 72MHz`, (preset test) `TEST HOOKS ON` |
| `Q_RecvInput` | `KEY 5`, `DOOR OPEN`, `DOOR CLOSED`, `EMERGENCY`, `RELOCKED` |
| `Q_SendAct` | `ACT:UNLOCK`, `ACT:LOCK`, `ACT:BEEP_TICK`, `ACT:ALARM_ON`, `ACT:ALARM_OFF` |
| `Q_SendUI` | `UI:IDLE`, `UI:ENTER_PIN 3`, `UI:GRANTED`, `UI:LOCKED 57`, `UI:DOOR_AJAR`, `UI:ADMIN 2` |
| `Q_SendLog` | `LOG:OK`, `LOG:FAIL` |
| Task 2 (`TRACE` khi đổi trạng thái) | `STATE:IDLE`, `STATE:ENTERING_PIN`, `STATE:UNLOCKED`, `STATE:LOCKED_OUT`, `STATE:ADMIN_RESET` |
| Task 4 logger (mọi preset) | `[YYYY-MM-DD HH:MM:SS] UNLOCK_OK` / `UNLOCK_FAIL` (stub hiện in `[--] UNLOCK_OK`) |

### 5.5 Scenario Wokwi CI

File `tests/scenarios/*.test.yaml`, chạy trên bản `test`. Các bước dùng được: `wait-serial` (viết đúng nguyên
dòng như §5.4), `write-serial`, `delay` (có đơn vị, ví dụ `500ms`), `set-control` (`btn_emergency`, `name: pressed`),
`expect-pin` (ví dụ `led_green`, `name: A`), `take-screenshot` (`lcd`). Tài liệu:
<https://docs.wokwi.com/wokwi-ci/automation-scenarios>. Chạy cục bộ (cần token `WOKWI_CLI_TOKEN`):

```bash
wokwi-cli . --scenario tests/scenarios/<task>.test.yaml --timeout 30000   # wokwi.toml trỏ build/test
```

CI (`.github/workflows/ci.yml`): `host-tests` → `firmware` (build 3 preset) → `wokwi` (matrix 5 scenario).

---

## §6. Đặc tả từng Task

### Task 1 – Input (`app/input/`) · UC1, UC8 (đọc), UC9 (ngắt)

Hàm phải cài (`input.h`): `Input_Init()`, `Input_Process()`, `Input_OnEmergencyIrq()` (chạy trong ngắt).

Task 1 tách làm hai: `input.c` là lớp HAL mỏng (quét keypad, đọc PB10, chia nhịp `KEYPAD_SCAN_MS`), còn
`input_core.c/.h` là logic thuần không include HAL (chống rung, gửi sự kiện, cờ `g_doorOpen`/`g_emergencyReq`).
Nhờ vậy toàn bộ logic chạy được trong unit test host `tests/host/test_input.c`; keypad và công tắc cửa không
điều khiển được bằng `set-control` của Wokwi nên chỉ kiểm bằng unit test này, scenario chỉ kiểm nút khẩn cấp.

Hành vi:

- Keymap: `BOARD_KEYMAP` (hàng R1..R4 = `{'1','2','3','A'}` … `{'*','0','#','D'}`). Hàng PA0–PA3 open-drain
  (thả = HIGH), cột PA4–PA7 pull-up.
- Mỗi `KEYPAD_SCAN_MS` (10 ms): kéo lần lượt từng hàng xuống LOW, chờ vài µs (`__NOP`), đọc 4 cột; cột LOW = phím
  đang nhấn. Trả hàng về HIGH sau khi đọc.
- Chống rung: giống nhau `KEY_DEBOUNCE_SAMPLES` lần liên tiếp mới chấp nhận; chỉ gửi ở **cạnh nhấn**
  (0 → phím): `Q_SendInput({EVT_KEY, k})`. Giữ phím không lặp; đổi thẳng từ phím này sang phím khác (không qua
  trạng thái nhả ổn định) thì không gửi sự kiện.
- Cửa PB10 (`DOOR_SW`): LOW = đóng (`BOARD_DOOR_CLOSED_LEVEL`), chống rung `DOOR_DEBOUNCE_SAMPLES` lần cùng nhịp
  10 ms. Khi trạng thái ổn định đổi: ghi `g_doorOpen` **trước**, rồi gửi `EVT_DOOR_OPEN`/`EVT_DOOR_CLOSED`.
- `Input_Init()`: đọc trạng thái cửa ban đầu, ghi `g_doorOpen`, gửi một sự kiện cửa.
- `Input_OnEmergencyIrq()`: bỏ qua nếu cách lần trước < `EMERG_DEBOUNCE_MS` (dùng `HAL_GetTick`/`app_now_ms`);
  "lần trước" là lần được **nhận** gần nhất, ngắt bị bỏ không dời mốc; lần đầu luôn được nhận. Nút chỉ được nhận
  lại sau khi nhả ổn định (`DOOR_DEBOUNCE_SAMPLES` nhịp quét liên tiếp ở mức nhả, `Input_Process` đọc PB11 rồi gọi
  `InputCore_EmergencyLevel()`) và qua `EMERG_DEBOUNCE_MS`, nên giữ nút lâu rồi nhả không sinh lần thứ hai;
  `g_emergencyReq = 1`; `Q_SendInput({EVT_EMERGENCY})`.

Thử riêng (preset `debug`, stub Task 2/3 của foundation): vết `KEY x`, `DOOR OPEN/CLOSED`, `EMERGENCY` do
`Q_RecvInput` in; stub Task 3 bật LED xanh khi có `g_emergencyReq`.

Nghiệm thu: 16/16 phím đúng ký tự; mỗi lần nhấn đúng 1 sự kiện; cửa chỉ báo khi đổi; nhấn nút khẩn cấp →
`EMERGENCY` + LED xanh; không chờ chặn trong `Input_Process`.
Scenario đích `input_emergency.test.yaml`: `wait-serial: 'SMARTLOCK READY 72MHz'` → `wait-serial: 'DOOR CLOSED'`
(trạng thái cửa ban đầu) → `delay 300ms` → `set-control btn_emergency pressed 1` → `wait-serial: 'EMERGENCY'` →
`expect-pin led_green A 1` → `delay 100ms` → `pressed 0`. Chờ serial và kiểm chân nằm **trước** bước nhả nút;
nút giữ ngắn (≪ `EMERG_DEBOUNCE_MS`) nên các cạnh rung khi nhả bị bỏ, chỉ in đúng 1 dòng `EMERGENCY`.

### Task 2 – Security (`app/security/`) · UC4, UC6, UC7, UC8 (hẹn giờ)

Hàm phải cài (`security.h`): `Security_Init()`, `Security_Process()`, `Security_GetState()`; enum `SysState_t` đã có.
Chỉ include `app_time.h`, `queues.h`, `app_config.h`, `trace.h`.

`Security_Process()`: lấy hết sự kiện bằng `Q_RecvInput`, mỗi sự kiện gọi một hàm xử lý `switch(state)`; sau đó
kiểm các mốc thời gian (lockout, cửa); cuối cùng gọi `alarm_update()`. Mỗi lần đổi trạng thái: `TRACE("STATE:%s")`.

Phím: `0-9` thêm số (tối đa `PIN_MAX_LEN`, số thừa bỏ qua) · `#` xác nhận · `*` xóa, về IDLE · `A` vào admin ·
`C` thoát admin. Mỗi phím hợp lệ (ngoài LOCKED_OUT) gửi `Q_SendAct(ACT_BEEP_TICK)` và cập nhật màn hình.
PIN chủ khởi tạo `PIN_MASTER_DEFAULT`, lưu RAM. So khớp: độ dài bằng nhau và `memcmp == 0`; xóa bộ đệm sau mỗi lần so.

| Trạng thái | Sự kiện | Hành động → trạng thái |
| --- | --- | --- |
| IDLE | số | thêm số, `UI ENTER_PIN 1` → ENTERING_PIN |
| IDLE | `A` | `adminLogin = 1`, `UI ENTER_PIN 0` → ENTERING_PIN |
| ENTERING_PIN | số | thêm số, `UI ENTER_PIN len` |
| ENTERING_PIN | `*` | xóa, `adminLogin = 0`, `UI IDLE` → IDLE |
| ENTERING_PIN | `C` khi `adminLogin` | xóa, `adminLogin = 0`, `UI IDLE` → IDLE |
| ENTERING_PIN | `#` đúng, không admin | `failCount = 0`, `ACT UNLOCK`, `UI GRANTED`, `LOG OK` → UNLOCKED |
| ENTERING_PIN | `#` đúng, `adminLogin` | `failCount = 0`, `UI ADMIN 0` → ADMIN_RESET |
| ENTERING_PIN | `#` sai (cả khi admin) | `failCount++`, `LOG FAIL`; nếu `failCount == MAX_FAIL_COUNT` → ghi mốc, `UI LOCKED 60` → LOCKED_OUT; ngược lại `UI IDLE` → IDLE |
| ADMIN_RESET | số / `*` | thêm số mới, `UI ADMIN len` / xóa số mới |
| ADMIN_RESET | `#` với `PIN_MIN_LEN..PIN_MAX_LEN` số | lưu PIN chủ mới, `UI IDLE` → IDLE (ít hơn thì bỏ qua) |
| ADMIN_RESET | `C` | không đổi, `UI IDLE` → IDLE |
| LOCKED_OUT | mọi phím (kể cả `A`) | bỏ qua, không beep |
| LOCKED_OUT | mỗi 1 s | `UI LOCKED <giây còn lại>`; đủ `LOCKOUT_MS` → `failCount = 0`, `UI IDLE` → IDLE |
| UNLOCKED | `EVT_RELOCKED` | `UI IDLE` → IDLE |
| bất kỳ | `EVT_EMERGENCY` | `LOG OK`, `UI GRANTED`; nếu đang LOCKED_OUT thì hủy phạt (`failCount = 0`) → UNLOCKED |
| bất kỳ | `EVT_DOOR_OPEN` | ghi mốc cửa; mở quá `DOOR_AJAR_MS` → `doorAlarm = 1`, `UI DOOR_AJAR` |
| bất kỳ | `EVT_DOOR_CLOSED` | hủy mốc, `doorAlarm = 0`, vẽ lại màn của trạng thái hiện tại |

Báo động – một nơi quyết định duy nhất:

```c
static void alarm_update(void) {
    bool want = (state == STATE_LOCKED_OUT) || doorAlarm;
    if (want != alarmOn) { alarmOn = want; Q_SendAct(want ? ACT_ALARM_ON : ACT_ALARM_OFF); }
}
```

Thử riêng: preset `test`, gõ phím trên Serial; vết `ACT:`, `UI:`, `LOG:`, `STATE:` cho thấy kết quả.
Unit test: bật 2 test có sẵn trong `tests/host/test_security.c` và thêm test phủ **mọi dòng** của bảng trên
(dùng `fake_time_advance`).
Nghiệm thu: đủ 5 trạng thái đúng bảng; PIN đúng/sai/khóa 3 lần/đổi PIN/cửa mở lâu/khẩn cấp khi đang khóa đều có test.
Scenario đích `security_lockout.test.yaml`: `0000#`,`1111#`,`2222#` → `STATE:LOCKED_OUT`, `ACT:ALARM_ON` →
(6 s) `ACT:ALARM_OFF`, `STATE:IDLE` → `1234#` → `ACT:UNLOCK`.

### Task 3 – Actuator (`app/actuator/`) · UC2, UC4 (chấp hành), UC5, còi UC6/UC8, UC9

Hàm phải cài (`actuator.h`): `Actuator_Init()`, `Actuator_Process()`, `Actuator_OnToneTick()` (ngắt TIM2 4 kHz).
Stub đã có: start PWM TIM3 CH3 ở `SERVO_LOCK_CCR`, start ngắt TIM2, cờ `s_toneOn` và `Actuator_OnToneTick`
(đảo PB1 khi `s_toneOn`; `BUZZER_ACTIVE=1` thì giữ mức).

Hành vi của `Actuator_Process()` theo thứ tự:

1. `g_emergencyReq == 1` → ghi 0, mở khóa (servo `SERVO_UNLOCK_CCR`, LED xanh sáng), bắt đầu đếm tự khóa.
2. Lấy hết lệnh `Q_RecvAct`: `UNLOCK` → mở khóa + đếm lại; `LOCK` → khóa ngay + `Q_SendInput({EVT_RELOCKED})`;
   `BEEP_TICK` → kêu `BEEP_TICK_MS` (bỏ qua khi đang báo động); `ALARM_ON/OFF` → còi `ALARM_ON_MS`/`ALARM_OFF_MS`
   lặp / tắt.
3. Cập nhật `s_toneOn` theo mốc thời gian.
4. Tự khóa: bộ đếm `RELOCK_MS` chỉ chạy khi `g_doorOpen == 0`; cửa mở → dừng; cửa đóng lại → đếm lại từ đầu.
   Hết hạn → servo `SERVO_LOCK_CCR`, tắt LED xanh, `Q_SendInput({EVT_RELOCKED})`.

Servo: TIM3 PSC 71, ARR 19999 (1 µs/tick, 20 ms); `__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, ccr)`.

Thử riêng: preset `test`, lệnh `u l t a s e o c` (§5.3); logic analyzer `la` đo xung PB0.
Nghiệm thu: PWM 20 ms, xung 1.0/1.5 ms; mở → servo 90° + LED; tự khóa sau 5 s khi cửa đóng, không khóa khi cửa
mở; `EVT_RELOCKED` gửi đúng; tích ~50 ms; còi đúng `ALARM_ON/OFF`; khẩn cấp mở ngay.
Scenario đích `actuator_relock.test.yaml`: `u` → `expect-pin led_green A 1` → `delay 5500ms` →
`wait-serial: 'RELOCKED'`; `u`,`o` → `delay 6000ms` → LED vẫn sáng → `c` → `wait-serial: 'RELOCKED'`.

### Task 4 – UI & Log (`app/ui/`) · UC3, UC10

Hàm phải cài: `UI_Init()`, `UI_Process()` (`ui.h`); `Logger_Init()`, `Logger_Process()` (`logger.h`);
hoàn thiện `uart_port.c` (`uart_port.h`); thêm `sw_rtc.c/.h`.

LCD (PCF8574, `LCD_I2C_ADDR` 7-bit → HAL dùng `<< 1`, chế độ 4 bit, giữ bit backlight):

- `UI_Init()` khởi tạo LCD (được `HAL_Delay`) và vẽ màn IDLE.
- Giữ `shown[2][16]` và `want[2][16]`; tin mới chỉ sửa `want`. Mỗi lần `UI_Process()` gửi tối đa 2 ký tự khác
  nhau (mỗi ký tự ~0.4 ms trên I2C 100 kHz); không xóa cả màn hình.
- I2C chặn với timeout nhỏ (vài ms); lỗi I2C thì bỏ qua lượt, không treo vòng lặp.

| Màn | Dòng 1 | Dòng 2 |
| --- | --- | --- |
| `UI_IDLE` | `SMART DOOR LOCK` | `ENTER PIN` |
| `UI_ENTER_PIN` | `ENTER PIN:` | `*` × `pinLen`, còn lại khoảng trắng |
| `UI_GRANTED` | `ACCESS GRANTED` | trống |
| `UI_LOCKED` | `LOCKED <seconds>s` | trống |
| `UI_DOOR_AJAR` | `DOOR AJAR` | trống |
| `UI_ADMIN` | `ADMIN MODE` | `*` × `pinLen` |

RTC phần mềm `sw_rtc`: giờ gốc từ `__DATE__`/`__TIME__` lúc build; `now = gốc + app_now_ms()/1000`, đổi ra
năm-tháng-ngày giờ:phút:giây (tính cả năm nhuận).
Logger: mỗi `LogEvt_t` → `snprintf` thành `[YYYY-MM-DD HH:MM:SS] UNLOCK_OK` (hoặc `UNLOCK_FAIL`) + `\r\n` →
`uart_write`. Thay dòng stub `[--] ...`.
`uart_port.c`: giữ phần RX đang có. Thay TX chặn bằng ring buffer `UART_TX_BUF_SIZE`: `uart_write` chép cả dòng nếu
đủ chỗ (đầy thì bỏ cả dòng mới); `UartPort_Process` thấy UART rảnh và còn dữ liệu → `HAL_UART_Transmit_IT` đoạn liền
mạch từ `tail`; `UartPort_OnTxCplt` dời `tail` và gửi tiếp. `uart_write` được gọi từ vòng lặp; gửi đi bằng ngắt.

Thử riêng: tạo `app/ui/ui_demo.c` bọc trong `#ifdef UI_DEMO`, mỗi giây gửi `Q_SendUI`/`Q_SendLog` theo thứ tự
ENTER_PIN 1→4, GRANTED, UNLOCK_OK, UNLOCK_FAIL, LOCKED đếm ngược; gọi nó từ `UI_Process` cũng trong `#ifdef UI_DEMO`.
Khi thử, thêm `#define UI_DEMO` tạm ở đầu `ui_demo.c`/`ui.c`; xóa trước khi commit.
Nghiệm thu: đủ 6 màn; chỉ `*` cho PIN; không nhấp nháy; dấu thời gian đúng định dạng; mỗi lần mở khóa đúng 1 dòng log;
TX bằng ngắt; vết của các module khác vẫn in đầy đủ.
Scenario đích `ui_log.test.yaml`: `1234#` → `wait-serial` dòng log `UNLOCK_OK`; `take-screenshot` part `lcd`.

---

## §7. Phần cứng và mạch Wokwi

| Chân | Cấu hình (.ioc) | Label (`main.h`) | Task | Part Wokwi (`id`) |
| --- | --- | --- | --- | --- |
| PA0–PA3 | Output open-drain, mặc định HIGH | `KP_R1..KP_R4` | 1 | `keypad` R1–R4 |
| PA4–PA7 | Input pull-up | `KP_C1..KP_C4` | 1 | `keypad` C1–C4 |
| PB10 | Input pull-up (LOW = cửa đóng) | `DOOR_SW` | 1 | `door` (slide switch) |
| PB11 | EXTI falling, pull-up | `BTN_EMERG` | 1 | `btn_emergency` |
| PA8 | Output push-pull | `LED_GREEN` | 3 | `led_green` qua `r_led` 220 Ω |
| PB0 | TIM3_CH3 PWM 50 Hz | – | 3 | `servo` PWM, `la` D0 |
| PB1 | Output push-pull (âm từ TIM2 4 kHz) | `BUZZER` | 3 | `buzzer` |
| PB6 / PB7 | I2C1 SCL / SDA 100 kHz | – | 4 | `lcd` (`pins: i2c`) |
| PA9 / PA10 | USART1 115200 8N1 | – | 4 | Serial Monitor |
| PC13 | Output (LED onboard, sáng khi LOW) | `LED_BOARD` | foundation | onboard |

Clock: HSE 8 MHz × PLL 9 = 72 MHz; APB1 36 MHz (timer 72 MHz). NVIC: SysTick 0, EXTI15_10 1, TIM2 2, USART1 3.
Tên chân Blue Pill trong `diagram.json` không có chữ P: `bp:A9`, `bp:B6`, `bp:GND.1`, `bp:5V.1`, `bp:3V3.2`.
`id` của part là một phần hợp đồng (scenario tham chiếu theo `id`).
Trên Wokwi servo lấy nguồn `3V3.2` (mô phỏng không xét điện áp); board thật cấp 5V riêng cho servo.

Board thật: keypad màng 4x4, LCD1602 + PCF8574 (địa chỉ 0x27 hoặc 0x3F – quét I2C rồi sửa `LCD_I2C_ADDR`),
SG90 loại 180°, còi chủ động (preset `release` đặt `BUZZER_ACTIVE=1`), công tắc từ MC-38 (NC), nút nhấn, LED + 220 Ω,
ST-Link V2 (SWD PA13/PA14), USB-UART (TX→PA10, RX→PA9, chung GND), nguồn 5V ≥ 1 A cho servo.

---

## §8. Quyết định đã chốt (đừng làm khác nếu chưa có PR `iface/...`)

| Vấn đề | Quyết định | Lý do |
| --- | --- | --- |
| RTOS | Bare-metal super-loop | Phạm vi dự án; tên hàng đợi giữ như RTOS để chuyển sau |
| RTC | RTC phần mềm từ `HAL_GetTick` | Wokwi không mô phỏng RTC; bật LSE làm `SystemClock_Config` treo/lỗi |
| UART | Ngắt + ring buffer, không DMA | Wokwi không mô phỏng DMA |
| Callback HAL | Chỉ trong `hal_callbacks.c` | Hai branch cùng định nghĩa → lỗi link khi merge |
| FIFO | Tắt ngắt ngắn trong push/pop | `xKeypadQueue` có nhiều bên ghi (vòng lặp + ISR) |
| Báo động | Một hàm `alarm_update()` trong Task 2 | Đóng cửa không được tắt nhầm còi phạt |
| Sai PIN ở bước admin | Tính vào `failCount`; LOCKED_OUT bỏ qua cả `A` | Như khóa thương mại (dormakaba E-Plex) |
| Khẩn cấp khi LOCKED_OUT | Mở được và hủy phạt | Như núm vặn trong nhà / chìa cơ (igloohome) |
| Tự khóa | Chỉ khi cửa đóng (`g_doorOpen`) | Như Nuki / August DoorSense; chốt bật khi cửa mở sẽ kẹt |
| Buzzer | TIM2 đảo PB1 (Wokwi), giữ mức (`BUZZER_ACTIVE`) | PB1 là TIM3_CH4, chung timer với servo 50 Hz |
| Thứ tự chân keypad | A0..A3 = R1..R4, A4..A7 = C1..C4 | Dây keypad trên Wokwi cắt chéo là chấp nhận được |

---

## §9. Git, PR và thay đổi hợp đồng

- Branch: `feat/t<số>-<tính-năng>`, `fix/t<số>-<mô-tả>`, `iface/<mô-tả>`. Commit: `feat(input): ...`,
  `fix(security): ...`, `test(actuator): ...`. PR nhỏ theo use case; squash merge vào `main`.
- Trước khi mở PR: `git fetch && git rebase origin/main`, build 2 preset, chạy host test.
- **PR `iface/...`** (đổi hợp đồng): nhỏ, một mục đích, mô tả ai bị ảnh hưởng; cả nhóm review; merge trước, các
  branch khác rebase. Ví dụ: thêm `EVT_*` mới = sửa `app_types.h` + nhánh vết trong `queues.c`.
- Đổi chân/ngoại vi: chỉ foundation owner mở CubeMX, sửa `smart_lock.ioc` + Generate + `board_pins.h` + `diagram.json`
  trong cùng PR. Xung đột `.ioc` khi rebase: lấy bản của `main`, làm lại thay đổi trong CubeMX, Generate lại;
  không sửa tay `.ioc`.
- Code tự viết trong `Core/` chỉ nằm trong khối `/* USER CODE BEGIN */ … /* USER CODE END */`.

---

## §10. Lỗi thường gặp

| Triệu chứng | Nguyên nhân / cách xử lý |
| --- | --- |
| `multiple definition of HAL_..._Callback` | Module tự định nghĩa callback → chuyển ruột vào hàm `*_On*()` (§2.3) |
| Vòng lặp "đơ", keypad trễ, LCD giật | Có `HAL_Delay`/vòng chờ trong `Process()` hoặc ghi LCD cả dòng một lượt |
| Serial im lặng sau khi gọi in trong ngắt | `uart_write`/`TRACE` gọi trong ngắt → chuyển sang `Process()` |
| Scenario chờ mãi | Chuỗi `wait-serial` khác vết (§5.4), hoặc `wokwi.toml` chưa trỏ `build/test` |
| `Error_Handler` ngay khi khởi động | Ai đó bật RTC/LSE/DMA trong `.ioc` → hoàn tác |
| Unit test host không build trên Windows | Cần MinGW-w64 gcc + Ninja trong PATH; hoặc để CI chạy |
| Tên chân không nối trong Wokwi | Dùng `bp:A9`, không phải `bp:PA9` |
| LCD trắng trên board thật | Sai địa chỉ (0x27/0x3F) hoặc chưa chỉnh biến trở tương phản |
