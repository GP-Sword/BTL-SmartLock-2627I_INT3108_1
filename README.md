# Smart Lock – STM32F103C8 (Blue Pill)

Khóa cửa thông minh dùng bàn phím 4x4, servo, LCD 16x2 I2C và nhật ký UART, chạy bare-metal (super-loop,
không RTOS) trên Blue Pill. Build bằng CMake + `arm-none-eabi-gcc`, mô phỏng bằng Wokwi trong VS Code.

Hệ thống chia 4 khối, mỗi người phụ trách một khối:

| Task | Khối | Thư mục |
| --- | --- | --- |
| 1 | Nhập liệu & cảm biến: keypad, công tắc cửa, nút khẩn cấp | `app/input/` |
| 2 | Bảo mật & máy trạng thái: PIN, khóa khi sai, đổi PIN, cửa mở lâu | `app/security/` |
| 3 | Chấp hành: servo, LED, buzzer, tự khóa | `app/actuator/` |
| 4 | Giao diện & nhật ký: LCD, RTC phần mềm, UART | `app/ui/` |

Tài liệu kỹ thuật đầy đủ (kiến trúc, đặc tả từng task, quy tắc tích hợp): [`docs/GUIDE.md`](docs/GUIDE.md).

## Yêu cầu

- VS Code + extension: Wokwi Simulator, CMake Tools, C/C++ (VS Code tự gợi ý từ `.vscode/extensions.json`)
- [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) 13.3 (`arm-none-eabi-gcc` trong PATH)
- CMake ≥ 3.22, Ninja, Git
- License Wokwi: `F1` → **Wokwi: Request a new License**
- Tùy chọn: gcc + Ninja của máy (Windows: MinGW-w64) để chạy unit test trên PC

## Bắt đầu nhanh

```bash
git clone <repo-url> smart_lock && cd smart_lock
cmake --preset debug
cmake --build --preset debug
```

Trong VS Code: `F1` → **Wokwi: Start Simulator**. Serial Monitor in `SMARTLOCK READY 72MHz` và LED PC13 nhấp nháy
là đã chạy đúng.

## Build preset

| Preset | Dùng khi |
| --- | --- |
| `debug` | Phát triển hằng ngày trên Wokwi, có in vết sự kiện |
| `test` | Chạy kịch bản test; nhận lệnh gõ từ Serial Monitor, thời gian chờ rút ngắn |
| `release` | Nạp board thật bằng ST-Link |

## Kiểm thử

```bash
# Unit test trên PC
cmake -S tests/host -B build/host -G Ninja
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

- Kịch bản Wokwi: `tests/scenarios/*.test.yaml`, CI chạy tự động trên bản `test`.
- Lệnh gõ trên Serial ở bản `test`: phím `0-9 A-D * #`, cửa `o`/`c`, khẩn cấp `e`, cơ cấu chấp hành `u l t a s`.

## Cấu trúc thư mục

```text
app/common/    Phần dùng chung: vòng lặp chính, hàng đợi, kiểu dữ liệu, cấu hình
app/input/     Task 1          app/security/  Task 2
app/actuator/  Task 3          app/ui/        Task 4
Core/ Drivers/ cmake/ smart_lock.ioc    Mã STM32CubeMX sinh
tests/host/    Unit test PC    tests/scenarios/  Kịch bản Wokwi
docs/GUIDE.md  Tài liệu kỹ thuật
diagram.json, wokwi.toml        Mạch và cấu hình Wokwi
```

## Quy trình làm việc

1. Tạo branch từ `main`: `feat/t<số>-<tính-năng>`, ví dụ `feat/t2-lockout`.
2. Chỉ sửa trong thư mục của task mình. Cần đổi phần dùng chung → mở PR riêng `iface/<mô-tả>`.
3. Build `debug` và `test` không có warning, unit test qua, thử trên Wokwi.
4. Mở PR vào `main`, điền checklist; CI (`host-tests`, `firmware`, `wokwi`) phải xanh; squash merge.

Chi tiết quy tắc và tiêu chí nghiệm thu từng task: [`docs/GUIDE.md`](docs/GUIDE.md).

## Phần cứng (board thật)

Blue Pill STM32F103C8, keypad màng 4x4, LCD1602 + module I2C PCF8574, servo SG90, còi chủ động 5V,
công tắc từ cửa MC-38, nút nhấn, LED + điện trở 220 Ω, mạch nạp ST-Link V2, mạch USB-UART.
Sơ đồ chân: [`docs/GUIDE.md`](docs/GUIDE.md) §7.
