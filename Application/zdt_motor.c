#include "zdt_motor.h"
#include "Emm_V5.h"
#include "struct_typedef.h"

/*-----------------------------------宏定义-----------------------------------*/

#define Ballscrew_Motor_ADDR 0x01  // 丝杆电机地址
#define Telescopic_Motor_ADDR 0x02  // 伸缩电机地址
#define Collision_Return_To_Zero 0x01  // 碰撞回零模式

// 每毫米对应的脉冲数（pulses/mm）
#define Ballscrew_Pulses_Per_Mm 1000.0f
#define Telescopic_Pulses_Per_Mm 1.0f

// Emm_V5 方向定义：0为CW，其余值为CCW，cw为向下，ccw为向上（根据实际安装调整）
#define EMM_DIR_CW 0u
#define EMM_DIR_CCW 1u

// 机构方向映射（如发现运动方向相反，只需要对调 CW/CCW 即可）
#define Telescopic_Forward EMM_DIR_CW
#define Telescopic_Backward EMM_DIR_CCW
#define Ballscrew_Up EMM_DIR_CW
#define Ballscrew_Down EMM_DIR_CCW

// 速度和加速度
#define Telescopic_Move_Speed 100
#define Telescopic_Move_Acc 100
#define Ballscrew_Move_Speed 2000
#define Ballscrew_Move_Acc 255

/*-----------------------------------预设位置表（枚举→距离映射）-----------------------------------*/

// 丝杆预设位置表（单位：mm，相对于零点的绝对位置）
static const fp32 ballscrew_position_table[BALLSCREW_POS_COUNT] = {
    [BALLSCREW_POS_ZERO]           = 0.0f,
    [BALLSCREW_POS_OBSERVE]        = 0.0f,    // 观察高度
    [BALLSCREW_POS_BSP_MATERIAL]   = 200.0f,  // 车顶物料
    [BALLSCREW_POS_FLOOR_MATERIAL] = 200.0f,  // 地面物料
};

// 伸缩臂预设位置表（单位：mm，相对于零点的绝对位置）
static const fp32 telescopic_position_table[TELESCOPIC_POS_COUNT] = {
    [TELESCOPIC_POS_ZERO]                 = 0.0f,
    [TELESCOPIC_POS_OBSERVE_RING]         = 200.0f,
    [TELESCOPIC_POS_OBSERVE_MATERIAL]     = 200.0f,
    [TELESCOPIC_POS_BSP_MATERIAL]         = 300.0f,
    [TELESCOPIC_POS_FLOOR_MIDDLE_MATERIAL]= 300.0f,
    [TELESCOPIC_POS_FLOOR_SIDE_MATERIAL]  = 500.0f,
};

/*-----------------------------------电机对象实例-----------------------------------*/

static zdt_motor_t ballscrew_motor = {
    .state = MOTOR_STATE_IDLE,
    .current_pos_mm = 0.0f,
    .target_pos_mm = 0.0f,
};

static zdt_motor_t telescopic_motor = {
    .state = MOTOR_STATE_IDLE,
    .current_pos_mm = 0.0f,
    .target_pos_mm = 0.0f,
};

/*-----------------------------------内部函数-----------------------------------*/

static void Ballscrew_Reset_CurPos_To_Zero(void)
{
    Emm_V5_Origin_Trigger_Return(Ballscrew_Motor_ADDR, Collision_Return_To_Zero, false);
}

static void Telescopic_Reset_CurPos_To_Zero(void)
{
    Emm_V5_Origin_Trigger_Return(Telescopic_Motor_ADDR, Collision_Return_To_Zero, false);
}

/**
 * @brief 内部：驱动丝杆移动指定脉冲（带方向自动判断）
 */
static void Ballscrew_DriveByPulse(int32_t pulse)
{
    if (pulse == 0) return;
    uint8_t dir = (pulse > 0) ? Ballscrew_Down : Ballscrew_Up;
    uint32_t abs_pulse = (pulse > 0) ? (uint32_t)pulse : (uint32_t)(-pulse);
    Emm_V5_Pos_Control(Ballscrew_Motor_ADDR, dir, Ballscrew_Move_Speed,
                       Ballscrew_Move_Acc, abs_pulse, false, false);
}

/**
 * @brief 内部：驱动伸缩臂移动指定脉冲（带方向自动判断）
 */
static void Telescopic_DriveByPulse(int32_t pulse)
{
    if (pulse == 0) return;
    uint8_t dir = (pulse > 0) ? Telescopic_Forward : Telescopic_Backward;
    uint32_t abs_pulse = (pulse > 0) ? (uint32_t)pulse : (uint32_t)(-pulse);
    Emm_V5_Pos_Control(Telescopic_Motor_ADDR, dir, Telescopic_Move_Speed,
                       Telescopic_Move_Acc, abs_pulse, false, false);
}

/*-----------------------------------外部接口实现-----------------------------------*/

void ZDT_Motor_Init(void)
{
    Ballscrew_Reset_CurPos_To_Zero();
    Telescopic_Reset_CurPos_To_Zero();
    
    ballscrew_motor.state = MOTOR_STATE_IDLE;
    ballscrew_motor.current_pos_mm = 0.0f;
    ballscrew_motor.target_pos_mm = 0.0f;
    
    telescopic_motor.state = MOTOR_STATE_IDLE;
    telescopic_motor.current_pos_mm = 0.0f;
    telescopic_motor.target_pos_mm = 0.0f;
}

/**
 * @brief 丝杆移动到预设位置
 * @param target 目标位置枚举
 */
void Ballscrew_MoveToTarget(ballscrew_target_pos_e target)
{
    if (target >= BALLSCREW_POS_COUNT) return;
    
    fp32 target_mm = ballscrew_position_table[target];
    fp32 delta_mm = target_mm - ballscrew_motor.current_pos_mm;
    int32_t pulse = (int32_t)(delta_mm * Ballscrew_Pulses_Per_Mm);
    
    ballscrew_motor.target_pos_mm = target_mm;
    ballscrew_motor.state = MOTOR_STATE_MOVING;
    
    Ballscrew_DriveByPulse(pulse);
    
    // 注意：这里假设运动会成功完成，实际应在回调/轮询中更新
    ballscrew_motor.current_pos_mm = target_mm;
    ballscrew_motor.state = MOTOR_STATE_IDLE;
}

/**
 * @brief 伸缩臂移动到预设位置
 * @param target 目标位置枚举
 */
void Telescopic_MoveToTarget(telescopic_target_pos_e target)
{
    if (target >= TELESCOPIC_POS_COUNT) return;
    
    fp32 target_mm = telescopic_position_table[target];
    fp32 delta_mm = target_mm - telescopic_motor.current_pos_mm;
    int32_t pulse = (int32_t)(delta_mm * Telescopic_Pulses_Per_Mm);
    
    telescopic_motor.target_pos_mm = target_mm;
    telescopic_motor.state = MOTOR_STATE_MOVING;
    
    Telescopic_DriveByPulse(pulse);
    
    telescopic_motor.current_pos_mm = target_mm;
    telescopic_motor.state = MOTOR_STATE_IDLE;
}

/**
 * @brief 丝杆按距离移动（正值向下，负值向上）
 * @param distance_mm 移动距离（mm）
 */
void Ballscrew_MoveByDistance(fp32 distance_mm)
{
    int32_t pulse = (int32_t)(distance_mm * Ballscrew_Pulses_Per_Mm);
    
    ballscrew_motor.target_pos_mm = ballscrew_motor.current_pos_mm + distance_mm;
    ballscrew_motor.state = MOTOR_STATE_MOVING;
    
    Ballscrew_DriveByPulse(pulse);
    
    ballscrew_motor.current_pos_mm += distance_mm;
    ballscrew_motor.state = MOTOR_STATE_IDLE;
}

/**
 * @brief 伸缩臂按距离移动（正值向前，负值向后）
 * @param distance_mm 移动距离（mm）
 */
void Telescopic_MoveByDistance(fp32 distance_mm)
{
    int32_t pulse = (int32_t)(distance_mm * Telescopic_Pulses_Per_Mm);
    
    telescopic_motor.target_pos_mm = telescopic_motor.current_pos_mm + distance_mm;
    telescopic_motor.state = MOTOR_STATE_MOVING;
    
    Telescopic_DriveByPulse(pulse);
    
    telescopic_motor.current_pos_mm += distance_mm;
    telescopic_motor.state = MOTOR_STATE_IDLE;
}

void ZDT_Motor_Stop_All(void)
{
    Emm_V5_Stop_Now(Ballscrew_Motor_ADDR, false);
    Emm_V5_Stop_Now(Telescopic_Motor_ADDR, false);
    
    ballscrew_motor.state = MOTOR_STATE_IDLE;
    telescopic_motor.state = MOTOR_STATE_IDLE;
}

const zdt_motor_t* Ballscrew_GetMotor(void)
{
    return &ballscrew_motor;
}

const zdt_motor_t* Telescopic_GetMotor(void)
{
    return &telescopic_motor;
}


// 外部调用实例
// 初始化
//ZDT_Motor_Init();

// 丝杆移动到"车顶物料"预设位置
//Ballscrew_MoveToTarget(BALLSCREW_POS_BSP_MATERIAL);

// 伸缩臂向前伸出 100mm
//Telescopic_MoveByDistance(100.0f);

// 查询当前位置
//const zdt_motor_t* motor = Ballscrew_GetMotor();
//fp32 pos = motor->current_pos_mm;


