#ifndef ZDT_MOTOR_H
#define ZDT_MOTOR_H

#define Motor_35_ADDR 1  // 丝杆电机地址
#define Motor_28_ADDR 2  // 伸缩电机地址

#define Motor_35_UP 1
#define Motor_35_DOWN 0

#define YUNHUO 9500 // 大约0.7ms
#define WULIAOPAN 21200 // 大约1.0s
#define DIMIAN 52700 // 大约1.5s

#define QIAN 1
#define HOU 0

extern void zdt_motor_test(void);

#endif // ZDT_MOTOR_H