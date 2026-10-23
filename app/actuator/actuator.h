/**
 * @file    actuator.h
 * @brief   Task 3 - Servo, LED xanh, buzzer (UC2, UC4 chấp hành, UC5, còi UC6/UC8, UC9).
 *          OWNER: Task 3. Là khối DUY NHẤT điều khiển servo, LED xanh và buzzer.
 */
#ifndef ACTUATOR_H
#define ACTUATOR_H

void Actuator_Init(void);
void Actuator_Process(void);   /* xử lý g_emergencyReq, xActuatorQueue, buzzer, tự khóa */
void Actuator_OnToneTick(void); /* gọi từ ngắt TIM2 4 kHz (hal_callbacks.c)            */

#endif /* ACTUATOR_H */
