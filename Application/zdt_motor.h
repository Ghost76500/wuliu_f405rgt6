#ifndef ZDT_MOTOR_H
#define ZDT_MOTOR_H

#include "struct_typedef.h"
#include <stdint.h>
#include <stdbool.h>

/*-------------------------- 枚举定义 --------------------------*/

// 电机运行状态（用于状态机/外部查询）
typedef enum {
    MOTOR_STATE_IDLE = 0,   // 空闲
    MOTOR_STATE_MOVING,     // 运动中
    MOTOR_STATE_ERROR,      // 错误
} motor_state_e;

// 伸缩臂目标位置枚举（预设位置点）
typedef enum {
    TELESCOPIC_POS_ZERO = 0,                // 零点/收回
    TELESCOPIC_POS_OBSERVE_RING,            // 观察环位置
    TELESCOPIC_POS_OBSERVE_MATERIAL,        // 观察物料位置
    TELESCOPIC_POS_BSP_MATERIAL,            // 车顶物料位置
    TELESCOPIC_POS_FLOOR_MIDDLE_MATERIAL,   // 地面中间物料位置
    TELESCOPIC_POS_FLOOR_SIDE_MATERIAL,     // 地面两侧物料位置
    TELESCOPIC_POS_COUNT,                   // 枚举计数（用于数组边界）
} telescopic_target_pos_e;

// 丝杆目标位置枚举（预设位置点）
typedef enum {
    BALLSCREW_POS_ZERO = 0,         // 零点/最高
    BALLSCREW_POS_OBSERVE,          // 观察高度
    BALLSCREW_POS_BSP_MATERIAL,     // 车顶物料高度
    BALLSCREW_POS_FLOOR_MATERIAL,   // 地面物料高度
    BALLSCREW_POS_COUNT,            // 枚举计数
} ballscrew_target_pos_e;

/*-------------------------- 结构体定义 --------------------------*/

// 电机控制结构体（面向对象：把状态和数据封装在一起）
typedef struct {
    motor_state_e   state;              // 当前状态
    fp32            current_pos_mm;     // 当前位置(mm)
    fp32            target_pos_mm;      // 目标位置(mm)
} zdt_motor_t;

/*-------------------------- 外部接口 --------------------------*/

// 初始化
void ZDT_Motor_Init(void);

// 丝杆：移动到预设位置（传入枚举）
void Ballscrew_MoveToTarget(ballscrew_target_pos_e target);

// 伸缩臂：移动到预设位置（传入枚举）
void Telescopic_MoveToTarget(telescopic_target_pos_e target);

// 丝杆：按距离移动（正值向下，负值向上）
void Ballscrew_MoveByDistance(fp32 distance_mm);

// 伸缩臂：按距离移动（正值向前，负值向后）
void Telescopic_MoveByDistance(fp32 distance_mm);

// 停止所有电机
void ZDT_Motor_Stop_All(void);

// 获取当前状态（可选：供外部查询）
const zdt_motor_t* Ballscrew_GetMotor(void);
const zdt_motor_t* Telescopic_GetMotor(void);

#endif // ZDT_MOTOR_H