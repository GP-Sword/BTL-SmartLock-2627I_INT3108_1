cmake_minimum_required(VERSION 3.22)

#
# CubeMX chỉ sinh file này MỘT lần, không ghi đè khi Generate lại.
# OWNER: foundation. Branch tính năng KHÔNG cần sửa file này:
# thêm file .c/.h vào thư mục app/<module>/ của mình là CMake tự nhận (GLOB).
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name -> build/<preset>/smart_lock.elf
set(CMAKE_PROJECT_NAME smart_lock)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources (Core/, Drivers/, startup)
add_subdirectory(cmake/stm32cubemx)

# ---------------------------------------------------------------------------
# Tùy chọn build (preset chọn sẵn, xem CMakePresets.json)
# ---------------------------------------------------------------------------
option(APP_TRACE      "In dòng vết ACT:/UI:/LOG:/KEY ra UART"                  OFF)
option(APP_TEST_HOOKS "Nhận lệnh từ UART RX, rút ngắn timeout (scenario CI)"   OFF)
option(BUZZER_ACTIVE  "Board thật dùng còi chủ động (Wokwi: OFF)"              OFF)

# ---------------------------------------------------------------------------
# Mã ứng dụng: app/common + 1 thư mục cho mỗi task
# ---------------------------------------------------------------------------
set(APP_DIRS
    ${CMAKE_SOURCE_DIR}/app/common
    ${CMAKE_SOURCE_DIR}/app/input
    ${CMAKE_SOURCE_DIR}/app/security
    ${CMAKE_SOURCE_DIR}/app/actuator
    ${CMAKE_SOURCE_DIR}/app/ui
)

set(APP_SOURCES "")
foreach(dir ${APP_DIRS})
    file(GLOB dir_sources CONFIGURE_DEPENDS ${dir}/*.c)
    list(APPEND APP_SOURCES ${dir_sources})
endforeach()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${APP_SOURCES})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${APP_DIRS})

# Cảnh báo chặt chỉ áp cho mã của nhóm, không áp cho mã CubeMX sinh.
set_source_files_properties(${APP_SOURCES} PROPERTIES COMPILE_OPTIONS
    "-Wall;-Wextra;-Wshadow;-Wdouble-promotion;-Werror=implicit-function-declaration;-Werror=return-type"
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<BOOL:${APP_TRACE}>:APP_TRACE>
    $<$<BOOL:${APP_TEST_HOOKS}>:APP_TEST_HOOKS>
    BUZZER_ACTIVE=$<IF:$<BOOL:${BUZZER_ACTIVE}>,1,0>
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
)

# ---------------------------------------------------------------------------
# Sau mỗi lần build: tạo .hex/.bin và in dung lượng
# ---------------------------------------------------------------------------
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Tao ${CMAKE_PROJECT_NAME}.hex / .bin"
)
