#ifndef ZDT_MOTOR_H
#define ZDT_MOTOR_H

#include "struct_typedef.h"

#define Motor_35_ADDR 1  // 丝杆电机地址
#define Motor_28_ADDR 2  // 伸缩电机地址

#define Motor_35_UP 1
#define Motor_35_DOWN 0


extern void zdt_motor_test(void);
extern void Motor_35_Move(uint8_t dir, fp32 clk);
extern void Motor_28_Move(uint8_t dir, fp32 clk);


#endif // ZDT_MOTOR_H