/**
 * @file    logger.h
 * @brief   Task 4 - Nhật ký truy cập có dấu thời gian (UC10). OWNER: Task 4.
 *          Định dạng bắt buộc: "[YYYY-MM-DD HH:MM:SS] UNLOCK_OK" / "UNLOCK_FAIL".
 */
#ifndef LOGGER_H
#define LOGGER_H

void Logger_Init(void);
void Logger_Process(void);

#endif /* LOGGER_H */
