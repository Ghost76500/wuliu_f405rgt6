#ifndef TOWER_H
#define TOWER_H

#include "struct_typedef.h"


/*云台指定角度宏定义*/
#define GIMBAL_MIN_ANGLE 500   // 最小占空比，对应0度
#define GIMBAL_MID_ANGLE 1500  // 中间占空比，对应180度
#define GIMBAL_MAX_ANGLE 2500  // 最大占空比，对应360度

#define GIMBAL_INIT_ANGLE 1960 // 初始位置占空比(未定义,随便填一个数字)

#define GIMBAL_ONE_ANGLE 2305 // 云台转动一个角度对应的占空比增量 中间差170
#define GIMBAL_TWO_ANGLE 2135 // 测试用占空比
#define GIMBAL_THREE_ANGLE 1960 // 测试用占空比

#define GIMBAL_BLUE_ANGLE 1280 // 蓝色位置占空比
#define GIMBAL_GREEN_ANGLE 955
#define GIMBAL_RED_ANGLE 650

#define GIMBAL_NAWULIAO_ANGLE 955


/*夹爪指定状态宏定义*/
#define JIAZHUA_ERROR 65
#define JIAZHUA_INIT 660 - JIAZHUA_ERROR   // 夹爪初始状态占空比(未定义,随便填一个数字)
#define JIAZHUA_CLOSE 660 - JIAZHUA_ERROR   // 夹爪闭合占空比(未定义,随便填一个数字)
#define JIAZHUA_XIAO_OPEN 930 - JIAZHUA_ERROR   // 夹爪张开一点点占空比(未定义,随便填一个数字)
#define JIAZHUA_DA_OPEN 1100 - JIAZHUA_ERROR   // 夹爪完全张开占空比

/*35步进上下移动丝杆宏定义*/
#define MECHANICAL_ERROR_35 5000 // 机械误差补偿脉冲数，根据实际情况调整

#define YUNHUO 6500 + MECHANICAL_ERROR_35 // 大约0.7ms
#define WULIAOPAN 21200 + MECHANICAL_ERROR_35 // 大约1.0s
#define DIMIAN 52000 + MECHANICAL_ERROR_35 // 大约1.5s
#define TAIQI 6500 + MECHANICAL_ERROR_35 // 把物料从物料盘里拿起来
#define MADUO 23000 + MECHANICAL_ERROR_35 // 物料叠叠乐
#define SHIBIE 17000 + MECHANICAL_ERROR_35 // 识别物料高度

#define QIAN 1
#define HOU 0

/*28步进伸缩夹爪脉冲宏定义*/
#define GROUND_GREEN 0
#define GROUND_RED 2150
#define GROUND_BLUE 1690
#define WULIAOPAN_MATERIAL 3300
#define WAITING 1300

#define NAWULIAO_28 1550 // 拿车上物料的脉冲数

extern const int16_t target_x; // 给视觉用
extern const int16_t target_y;
extern const int16_t materials_target_x;
extern const int16_t materials_target_y;

extern void grab_turntable_A(uint8_t color, uint8_t num); // 物料静态识别抓取，color为物料颜色，num为放置位置
extern void grab_turntable_B(uint8_t color, uint8_t num); // 物料动态识别抓取，color为物料颜色，num为放置位置
extern void grab_turntable_C(uint8_t color, uint8_t num); // 物料抓取转盘C，color为物料颜色，num为放置位置
extern void color_ring_calibration(uint8_t color); // 颜色环校准，color为色环颜色
extern void color_materials_calibration(uint8_t color, uint8_t mode); // 物料校准，color为物料颜色,mode为校准模式，0校准转盘物料，1校准码垛物料

extern void identify_posture(void); // 识别姿态，打开夹爪
extern void reset_posture(void); // 重置姿态，夹爪闭合，云台转回初始位置

extern void place_material(void); // 抓取物料盘物料
extern void grab_plate_material(void); // 抓取物料盘物料
extern void grab_materials_ground(uint8_t color, uint8_t first); // 抓取地板物料
extern void put_materials_car(uint8_t order); // 车上放置物料
extern void grab_materials_car(uint8_t order, uint8_t first);     // 抓取车上的物料
extern void put_materials_ground(uint8_t color, uint16_t first); // 放置物料到地面，color为物料颜色
extern void put_materials_materials(uint8_t color, uint16_t none); // 放置物料到物料上，color为物料颜色

#endif // TOWER_H