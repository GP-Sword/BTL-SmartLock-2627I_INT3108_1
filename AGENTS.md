# Hướng dẫn cho agent – Smart Lock (STM32F103C8, bare-metal)

Firmware khóa cửa trên Blue Pill: super-loop, không RTOS, CMake + `arm-none-eabi-gcc`, mô phỏng Wokwi.
Repo đã có **foundation** (hợp đồng chung + stub). Việc của dev: thay ruột stub trong thư mục task của mình,
thường bằng cách tích hợp code họ đã tự viết trước đó (có thể là FreeRTOS, Arduino hoặc HAL mẫu).

## Bước đầu tiên mỗi phiên: xác định task

Task suy ra từ tên branch (`feat/t1-…` → Task 1) hoặc từ thư mục dev nhắc tới; chưa rõ thì hỏi dev.

| Task | Thư mục được sửa | Đặc tả |
| --- | --- | --- |
| 1 Input (keypad, cửa, khẩn cấp) | `app/input/`, `tests/host/test_input.c`, `tests/scenarios/input_emergency.test.yaml` | `docs/GUIDE.md` §6 Task 1 |
| 2 Security (máy trạng thái) | `app/security/`, `tests/host/test_security.c`, `tests/scenarios/security_lockout.test.yaml` | §6 Task 2 |
| 3 Actuator (servo, LED, buzzer) | `app/actuator/`, `tests/scenarios/actuator_relock.test.yaml` | §6 Task 3 |
| 4 UI & Log (LCD, RTC mềm, UART) | `app/ui/`, `tests/scenarios/ui_log.test.yaml` | §6 Task 4 |
| foundation / `iface/…` | `app/common/`, `Core/`, `CMakeLists.txt`, `diagram.json`, `smart_lock.ioc`… | §9 |

## Đọc `docs/GUIDE.md` trước khi

- viết, port hoặc tích hợp code vào `app/`: §1 (quy trình), §2 (hợp đồng), §3 (bảng chuyển code có sẵn), §4 (quy tắc), rồi §6 phần task của mình;
- viết test hoặc scenario: §5;
- đụng chân, ngoại vi, `.ioc`, `diagram.json` hoặc phần dùng chung: §7, §9;
- gặp lỗi build/mô phỏng: §10.

## Quy tắc luôn áp dụng

- Sửa file trong thư mục của task; thêm file `.c/.h` mới vào đó (CMake tự nhận, không cần sửa `CMakeLists.txt`).
- Giữ nguyên chữ ký hàm trong header của module và mọi thứ trong `app/common/`. Cần đổi hợp đồng → dừng, đề xuất PR `iface/…` cho dev.
- `*_Process()` trả về nhanh: chờ bằng `soft_timer` / `app_elapsed_ms()`; `HAL_Delay` chỉ trong `*_Init()`.
- Module trao đổi qua `Q_Send*/Q_Recv*` và cờ `g_emergencyReq`, `g_doorOpen`. Callback HAL chỉ nằm trong `app/common/hal_callbacks.c`; viết ruột vào hàm `*_On*()` của module.
- In bằng `TRACE()` / `uart_write()`, ngoài ngắt. Giữ đúng chuỗi vết ở GUIDE §5.4.
- Hằng số từ `app_config.h`, chân từ `main.h` / `board_pins.h`. RTC, DMA, IWDG giữ tắt (Wokwi không mô phỏng).

## Kiểm tra trước khi báo xong

```bash
cmake --preset debug && cmake --build --preset debug     # không có warning mới
cmake --preset test  && cmake --build --preset test
cmake -S tests/host -B build/host -G Ninja && cmake --build build/host && ctest --test-dir build/host --output-on-failure
```

Xong khi: cả ba lệnh sạch, mọi tiêu chí "Nghiệm thu" của task trong GUIDE §6 đã kiểm (ghi rõ mục nào cần dev
chạy Wokwi để xác nhận), và diff chỉ chạm file trong phạm vi task.
