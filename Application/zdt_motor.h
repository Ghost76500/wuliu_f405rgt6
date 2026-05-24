#ifndef ZDT_MOTOR_H
#define ZDT_MOTOR_H

#include "struct_typedef.h"

#define Motor_35_ADDR 1  // 丝杆电机地址
#define Motor_28_ADDR 2  // 伸缩电机地址

#define Motor_35_UP 1
#define Motor_35_DOWN 0

#define MECHANICAL_ERROR_35 5000 // 机械误差补偿脉冲数，根据实际情况调整

#define YUNHUO 9500 + MECHANICAL_ERROR_35 // 大约0.7ms
#define WULIAOPAN 21200 + MECHANICAL_ERROR_35 // 大约1.0s
#define DIMIAN 54700 + MECHANICAL_ERROR_35 // 大约1.5s

#define QIAN 1
#define HOU 0

#define GROUND_GREEN 0
#define GROUND_RED 1950
#define GROUND_BLUE 1700

extern void zdt_motor_test(void);
extern void Motor_35_Move(uint8_t dir, fp32 clk);
extern void Motor_28_Move(uint8_t dir, fp32 clk);


#endif // ZDT_MOTOR_H