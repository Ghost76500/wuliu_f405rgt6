/**
 * @file chassis_task.h
 * @brief 底盘任务的头文件
 * ----------------------------------------------------------------------------
 * @version 1.0.0.0
 * @author RM
 * @date 2018-12-26
 * @remark 官步初始代码
 * ----------------------------------------------------------------------------
 * @version 1.0.0.1
 * @author 周明杨
 * @date 2024-12-30
 * @remark 优化整体架构
 */

/*-----------------------------------预处理-----------------------------------*/

#ifndef CHASSIS_TASK_H
#define CHASSIS_TASK_H

#include "struct_typedef.h"
#include "pid.h"
#include <stddef.h>
#include "user_lib.h"

/*-----------------------------------宏定义-----------------------------------*/
// 任务开始空闲一段时间
#define CHASSIS_TASK_INIT_TIME 357

// 控制周期  秒 / 毫秒 / 频率
#define CHASSIS_CONTROL_TIME 0.01f
#define CHASSIS_CONTROL_TIME_MS 10
#define CHASSIS_CONTROL_FREQUENCE 100.0f

// 底盘电机速度转换为底盘速度的系数
#define MOTOR_SPEED_TO_CHASSIS_SPEED_VX 0.25f
#define MOTOR_SPEED_TO_CHASSIS_SPEED_VY 0.25f
#define MOTOR_SPEED_TO_CHASSIS_SPEED_WZ 0.25f

// 编码器脉冲数（40ms周期）转换为轮端物理速度 (m/s) 的系数
// 计算公式: (2 * PI * 轮子半径) / (编码器单圈总脉冲 * 减速比 * 0.04s)
#define ENCODER_DELTA_TO_VECTOR_SEN 0.004462f // 75mm
// #define ENCODER_DELTA_TO_VECTOR_SEN 0.004760f // 80mm

#define RPM_TO_SPEED_MS (2.0f * 3.1415926f * 0.0375f / 60.0f) // 轮子直径77mm

// 电机到底盘中心的距离，单位米
#define MOTOR_DISTANCE_TO_CENTER 0.13862f // 138.62697mm
// 单个底盘电机最大速度
#define MAX_WHEEL_SPEED 1.2f
// 底盘运动过程最大前进速度
#define NORMAL_MAX_CHASSIS_SPEED_X 1.2f
// 底盘运动过程最大平移速度
#define NORMAL_MAX_CHASSIS_SPEED_Y 1.2f

// 底盘前后运动加速度
#define CHASSIS_ACCEL_X_NUM 0.15f
// 底盘左右运动加速度
#define CHASSIS_ACCEL_Y_NUM 0.15f

#define CHASSIS_WZ_SET_SCALE 1.0f // 底盘旋转速度缩放系数

// ==================== 停机/死区抑制 ====================
// 速度单位: m/s；角速度单位: rad/s；PWM单位: 与 give_pwm 一致
// 用于抑制“停机时因为积分残留/编码器零漂导致的微小输出”。
#define CHASSIS_STOP_EPS_V (0.002f)           // 认为“已经停”的线速度阈值
#define CHASSIS_STOP_EPS_WZ (0.05f)          // 认为“已经停”的角速度阈值
#define CHASSIS_SPEED_SET_DEADBAND (0.005f)   // 轮速设定死区（m/s）
#define CHASSIS_PWM_DEADBAND (8)             // PWM 输出死区（|pwm|<=该值则置0）
#define CHASSIS_PWM_MIN_OUTPUT (10)          // PWM 最小有效输出值（用于克服电机静摩擦，提高位置环精度）

// ==================== 方向/极性修正 ====================
// 速度指令坐标约定：x 正方向=向左，y 正方向=向前
// 若“给正x车却向右”，将 CHASSIS_VX_SIGN 设为 -1.0f
// 若“给正y车却后退”，将 CHASSIS_VY_SIGN 设为 -1.0f
#define CHASSIS_VX_SIGN (-1.0f)
#define CHASSIS_VY_SIGN (1.0f)

// 轮速映射到各电机的方向修正（只影响 speed_set，不会把闭环变成正反馈）
// 若某个轮“目标为正但实际反向”，将该轮的符号取反。
#define CHASSIS_WHEEL1_DIR (1.0f)
#define CHASSIS_WHEEL2_DIR (1.0f)
#define CHASSIS_WHEEL3_DIR (-1.0f)
#define CHASSIS_WHEEL4_DIR (-1.0f)

// 底盘电机速度环PID
// 注意：底盘速度环使用的速度单位是 m/s（见 motor_chassis[i].speed / speed_set）。
// 若沿用“编码器增量/周期”下的PID参数，会因为误差量级变小(0~1m/s)而导致输出过小、起步很慢。
// 这里按 1 m/s ≈ (1 / ENCODER_DELTA_TO_VECTOR_SEN) 个编码器增量/40ms 做等效缩放。
#define JGB37_MOTOR_SPEED_PID_KP (0.5f / ENCODER_DELTA_TO_VECTOR_SEN)
#define JGB37_MOTOR_SPEED_PID_KI (0.06f / ENCODER_DELTA_TO_VECTOR_SEN)
#define JGB37_MOTOR_SPEED_PID_KD (0.0f)
#define JGB37_MOTOR_SPEED_PID_MAX_OUT 100.0f // 
#define JGB37_MOTOR_SPEED_PID_MAX_IOUT 120.0f // 
// 底盘旋转跟随PID
#define CHASSIS_FOLLOW_GIMBAL_PID_KP 4.0f
#define CHASSIS_FOLLOW_GIMBAL_PID_KI 0.0f
#define CHASSIS_FOLLOW_GIMBAL_PID_KD 0.18f
#define CHASSIS_FOLLOW_GIMBAL_PID_MAX_OUT 1.8f
#define CHASSIS_FOLLOW_GIMBAL_PID_MAX_IOUT 0.2f

/*-----------------------------------数据结构定义-----------------------------------*/

// 底盘控制模式
typedef enum
{
  CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW,  // 底盘会跟随云台相对角度
  CHASSIS_VECTOR_FOLLOW_CHASSIS_YAW, // 底盘有底盘角度控制闭环
  CHASSIS_VECTOR_NO_FOLLOW_YAW,      // 底盘有旋转速度控制
  CHASSIS_VECTOR_RAW,                // 直接向CAN发送电流控制
  CHASSIS_VECTOR_DEFINED             // 底盘云台独立运动
  //CHASSIS_RUN_MAP,                // 底盘有全局路径跟随闭环（未实现）
  //CHASSIS_VISUAL_CALIBRATION,        // 视觉标定模式（未实现）
} chassis_mode_e;

typedef struct
{
  // const motor_measure_t *chassis_motor_measure;
  fp32 accel;
  fp32 speed;
  fp32 speed_set;
  // int16_t give_current;
  int16_t give_pwm;
} chassis_motor_t;

typedef struct
{
  chassis_mode_e chassis_mode;              // 底盘控制模式
  const volatile fp32 *chassis_INS_angle;    // 获取陀螺仪解算出的欧拉角指针(可能由中断更新)
  chassis_motor_t motor_chassis[4];          // 底盘电机数据
  pid_type_def motor_speed_pid[4];           // 底盘电机速度pid
  pid_type_def chassis_angle_pid;            // 底盘跟随角度pid

  first_order_filter_type_t chassis_cmd_slow_set_vx; // 使用一阶低通滤波减缓设定值
  first_order_filter_type_t chassis_cmd_slow_set_vy; // 使用一阶低通滤波减缓设定值

  // 内部速度状态量仍采用(vx, vy)：vx=向前，vy=向左（便于复用麦轮解算公式）
  fp32 vx;                         // 底盘速度：向前为正，单位 m/s
  fp32 vy;                         // 底盘速度：向左为正，单位 m/s
  fp32 wz;                         // 底盘旋转角速度，逆时针为正 单位 rad/s
  fp32 vx_set;                     // 底盘设定速度：向前为正，单位 m/s
  fp32 vy_set;                     // 底盘设定速度：向左为正，单位 m/s
  fp32 wz_set;                     // 底盘设定旋转角速度 逆时针为正 单位 rad/s
  fp32 chassis_yaw_set;

  fp32 vx_max_speed;  // 前进方向最大速度，单位 m/s
  fp32 vx_min_speed;  // 后退方向最大速度，单位 m/s
  fp32 vy_max_speed;  // 左方向最大速度，单位 m/s
  fp32 vy_min_speed;  // 右方向最大速度，单位 m/s
  fp32 chassis_yaw;   // 陀螺仪和云台电机叠加的yaw角度
  fp32 chassis_pitch; // 陀螺仪和云台电机叠加的pitch角度
  fp32 chassis_roll;  // 陀螺仪和云台电机叠加的roll角度

} chassis_move_t;

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 底盘任务，间隔 CHASSIS_CONTROL_TIME_MS 2ms
 * @param[in] pvParameters 空
 * @retval none
 */
// extern void chassis_task(void const *pvParameters);

/**
 * @brief 根据遥控器通道值，计算纵向和横移速度
 * @param[out] vx_set 纵向速度指针
 * @param[out] vy_set 横移速度指针
 * @param[out] chassis_move_rc_to_vector chassis_move变量指针
 * @retval none
 */
// extern void chassis_rc_to_control_vector(fp32 *vx_set, fp32 *vy_set, chassis_move_t *chassis_move_rc_to_vector);

/**
 * @brief 获取底盘电机电流
 * @param[in] none
 * @retval 电流值
 */
//uint16_t get_chassis_current(void);

/**
 * @brief 便捷接口：设置(x, y, 目标航向角)（车体坐标系）
 * @note 坐标约定：x 正方向=向左，y 正方向=向前
 * @param[in] vx x 方向速度（向左为正）
 * @param[in] vy y 方向速度（向前为正）
 * @param[in] yaw_rad 目标航向角(rad)
 */
void chassis_set_control_target(fp32 vx, fp32 vy, fp32 yaw_rad);

/**
 * @brief 关闭航向角闭环（恢复为外部直接给wz的方式，默认wz为0）
 */
void chassis_cmd_disable_yaw_hold(void);

// ==================== 世界系速度控制接口 ====================

/**
 * @brief 关闭世界系速度控制，恢复为车体系
 */
void chassis_cmd_disable_world_frame(void);

/**
 * @brief 查询当前是否为世界系速度控制模式
 * @return 1=世界系, 0=车体系
 */
uint8_t chassis_cmd_is_world_frame(void);

/**
 * @brief 便捷接口：世界系速度 + 旋转角速度控制
 * @param[in] vx_world 世界系X方向速度 m/s
 * @param[in] vy_world 世界系Y方向速度 m/s
 * @param[in] wz_rad_s 旋转角速度 rad/s（逆时针为正）
 * @note 典型用法：小车边旋转边沿世界系某方向平移
 */
void chassis_set_world_target(fp32 vx_world, fp32 vy_world, fp32 wz_rad_s);

/**
 * @brief 便捷接口：世界系速度 + 航向角锁定
 * @param[in] vx_world 世界系X方向速度 m/s
 * @param[in] vy_world 世界系Y方向速度 m/s
 * @param[in] yaw_rad 目标航向角 rad
 * @note 小车保持固定航向，同时沿世界系某方向平移
 */
void chassis_set_world_target_yaw(fp32 vx_world, fp32 vy_world, fp32 yaw_rad);

/**
 * @brief 获取底盘运行数据指针（调试/上位机发送）
 * @return 底盘运行数据只读指针
 */
const chassis_move_t *get_chassis_move_data(void);

#endif // CHASSIS_TASK_H