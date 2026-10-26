# Unit test chạy trên PC (gcc/clang của máy, không cần toolchain ARM).
#
#   cmake -S tests/host -B build/host -G Ninja
#   cmake --build build/host
#   ctest --test-dir build/host --output-on-failure
#
# Chỉ build mã logic thuần (không include HAL). UNIT_TEST tắt đoạn tới hạn của fifo
# và tắt TRACE. Muốn test thêm module: thêm 1 add_executable + add_test bên dưới.
cmake_minimum_required(VERSION 3.22)
project(smart_lock_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(APP ${CMAKE_CURRENT_SOURCE_DIR}/../../app)

add_library(app_common STATIC
    ${APP}/common/fifo.c
    ${APP}/common/queues.c
    fake_time.c
)
target_include_directories(app_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${APP}/common
    ${APP}/security
)
target_compile_definitions(app_common PUBLIC UNIT_TEST)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(app_common PUBLIC -Wall -Wextra -Wno-unused-function)
endif()

enable_testing()

add_executable(test_common test_common.c)
target_link_libraries(test_common PRIVATE app_common)
add_test(NAME common COMMAND test_common)

add_executable(test_security test_security.c ${APP}/security/security.c)
target_link_libraries(test_security PRIVATE app_common)
add_test(NAME security COMMAND test_security)

add_executable(test_input test_input.c ${APP}/input/input_core.c)
target_include_directories(test_input PRIVATE ${APP}/input)
# --wrap: test chặn Q_SendInput để thấy giá trị cờ ngay lúc gửi sự kiện (thứ tự ghi cờ / gửi).
target_link_options(test_input PRIVATE -Wl,--wrap=Q_SendInput)
target_link_libraries(test_input PRIVATE app_common)
add_test(NAME input COMMAND test_input)
