/**
 * @file    app.h
 * @brief   Điểm vào ứng dụng. Core/Src/main.c chỉ gọi 2 hàm này
 *          (trong khối USER CODE 2 và USER CODE 3).
 */
#ifndef APP_H
#define APP_H

void App_Init(void);  /* gọi 1 lần sau MX_*_Init()          */
void App_Loop(void);  /* gọi liên tục trong while(1)        */

#endif /* APP_H */
