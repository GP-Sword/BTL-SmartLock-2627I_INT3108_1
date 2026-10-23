## Task / Use case
<!-- Ví dụ: Task 2 – UC6 khóa sau 3 lần sai -->

## Thay đổi
- 

## Đã kiểm tra
- [ ] `cmake --build --preset debug` và `--preset test` không có warning mới
- [ ] Unit test host qua (`ctest --test-dir build/host`)
- [ ] Chạy thử trên Wokwi VS Code: <!-- mô tả thao tác và kết quả -->
- [ ] Scenario mới/đã sửa trong `tests/scenarios/`
- [ ] Không sửa file ngoài thư mục của mình (trừ PR `iface/...`)
- [ ] Không dùng `HAL_Delay()` trong `*_Process()`, không tự định nghĩa `HAL_*Callback`
