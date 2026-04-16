/**
 * @file position_task.c
 * @brief 底盘位置环外环控制模块
 * ----------------------------------------------------------------------------
 * @version 1.0.0.0
 * @author Copilot
 * @date 2026-03-28
 * @remark 按 chassis_task 注释风格补充模块注释与函数注释
 */

/*-----------------------------------头文件引用-----------------------------------*/

#include "position_task.h"
#include "chassis_task.h"
#include "CAN_receive.h"
#include "cmsis_os2.h"
#include <math.h>

/*-----------------------------------宏定义-----------------------------------*/

#define POSITION_ODOM_OFFLINE_MISS_THRESHOLD 10U

/*-----------------------------------变量声明-----------------------------------*/

uint8_t position_mode = 1;

static chassis_odometry_t chassis_odometry;

static volatile fp32 g_position_target_x = 0.0f;
static volatile fp32 g_position_target_y = 0.0f;
static volatile fp32 g_position_target_yaw = 0.0f;
static volatile uint8_t g_position_enable = 0;
static volatile uint8_t g_position_odom_fresh = 1; // 位置里程计数据是否新鲜（0=离线/过期, 1=在线/新鲜）
static volatile uint8_t g_position_odom_miss_cnt = 0; // 位置里程计数据丢失计数器，连续丢失达到一定次数则认为离线
static volatile uint8_t g_position_arrive_stable_cnt = 0; // 连续到位计数器

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief  对输入值按绝对值进行限幅
 * @param[in] value 输入值
 * @param[in] abs_limit 绝对值上限
 * @retval 限幅后的值
 */
static fp32 position_clamp_abs(fp32 value, fp32 abs_limit)
{
    if (value > abs_limit)
    {
        return abs_limit;
    }
    if (value < -abs_limit)
    {
        return -abs_limit;
    }
    return value;
}

/**
 * @brief  位置环初始化，包括反馈指针、PID和初始状态
 * @param[out] odom_init 位置里程计结构体指针
 * @retval none
 */
static void position_init(chassis_odometry_t *odom_init);

/**
 * @brief  位置反馈更新，读取最新里程计并判定离线状态
 * @param[out] odom_update 位置里程计结构体指针
 * @retval none
 */
static void position_feedback_update(chassis_odometry_t *odom_update);

/**
 * @brief  位置控制计算，输出世界系速度指令
 * @param[out] odom_control 位置里程计结构体指针
 * @retval none
 */
static void position_control_calc(chassis_odometry_t *odom_control);

/**
 * @brief  位置控制输出，将计算结果下发到底盘控制接口
 * @param[out] odom_output 位置里程计结构体指针
 * @retval none
 */
static void position_output(chassis_odometry_t *odom_output);


/**
 * @brief  位置任务主循环
 * @param[in] argument 任务参数（未使用）
 * @retval none
 */
void position_task(void *argument)
{
    osDelay(CHASSIS_POSITION_TASK_INIT_TIME);
    position_init(&chassis_odometry);

    uint32_t previous_wake_time = osKernelGetTickCount();
    for (;;)
    {
        // 1、数据更新：位置反馈更新，获取最新的位置信息
        position_feedback_update(&chassis_odometry);
        // 2、位置控制计算
        position_control_calc(&chassis_odometry);
        // 3、位置控制输出：根据计算结果设置底盘控制目标
        position_output(&chassis_odometry);

        previous_wake_time += POSITION_CONTROL_TIME_MS;
        osDelayUntil(previous_wake_time);
    }
    
}

/*-----------------------------------对外接口实现-----------------------------------*/

/**
 * @brief  设置位置目标点（不改变航向目标）
 * @param[in] x_m 目标 x 坐标（m）
 * @param[in] y_m 目标 y 坐标（m）
 * @retval none
 */
void position_set_position(fp32 x_m, fp32 y_m)
{
    g_position_target_x = position_clamp_abs(x_m, CHASSIS_POSITION_X_LIMIT_M);
    g_position_target_y = position_clamp_abs(y_m, CHASSIS_POSITION_Y_LIMIT_M);
    // New target invalidates previous arrival status.
    chassis_odometry.arrive_flag = 0; // 设置新目标时重置到达状态
    g_position_arrive_stable_cnt = 0;
}

/**
 * @brief  设置位置与航向目标
 * @param[in] x_m 目标 x 坐标（m）
 * @param[in] y_m 目标 y 坐标（m）
 * @param[in] yaw_rad 目标航向角（rad）
 * @retval none
 */
void position_set_position_yaw(fp32 x_m, fp32 y_m, fp32 yaw_rad)
{
    position_set_position(x_m, y_m);
    g_position_target_yaw = yaw_rad;
}

/**
 * @brief  使能位置控制
 * @param[in] latch_current_target 1=锁存当前位置为目标，0=保持现有目标
 * @retval none
 */
void position_enable(uint8_t latch_current_target)
{
    g_position_enable = 1;
    chassis_odometry.mode = POSITION_MODE_POSITION;
    chassis_odometry.arrive_flag = 0; // 使能时重置到达状态
    g_position_arrive_stable_cnt = 0;
    if (latch_current_target)
    {
        g_position_target_x = chassis_odometry.position_x;
        g_position_target_y = chassis_odometry.position_y;
    }
}

/**
 * @brief  关闭位置控制并清空控制输出
 * @retval none
 */
void position_disable(void)
{
    g_position_enable = 0;
    chassis_odometry.mode = POSITION_MODE_DISABLE;
    chassis_odometry.arrive_flag = 0;
    g_position_arrive_stable_cnt = 0;
    PID_clear(&chassis_odometry.position_x_pid);
    PID_clear(&chassis_odometry.position_y_pid);

    chassis_cmd_disable_yaw_hold();
    chassis_cmd_disable_world_frame();
    chassis_set_control_target(0.0f, 0.0f, 0.0f);
}

/**
 * @brief  查询是否到达目标点
 * @retval 1=已到达, 0=未到达
 */
uint8_t position_is_arrived(void)
{
    return chassis_odometry.arrive_flag;
}

/**
 * @brief  获取位置控制状态数据
 * @retval 位置里程计结构体常量指针
 */
const chassis_odometry_t *get_position_data(void)
{
    return &chassis_odometry;
}


/**
 * @brief  底盘目标定位（阻塞等待到位）
 * @param[in] x_m 目标 x 坐标（m）
 * @param[in] y_m 目标 y 坐标（m）
 * @param[in] yaw_rad 目标航向角（rad）
 * @param[in] delay 到位后额外延时（ms）
 * @retval none
 */
void Chassis_Go_Pos(fp32 x_m, fp32 y_m, fp32 yaw_rad, uint16_t delay)
{
    position_enable(0);
    position_set_position_yaw(x_m, y_m, yaw_rad);

    while (position_is_arrived() == 0)
    {
        osDelay(10);
    }

    if (delay != 0U)
    {
        osDelay(delay);
    }
}

/*
 * @brief  视觉定位（阻塞等待到位）
 * @param[in] target 目标标识
 * @param目标编号 1物料 2色环 3码垛色环
 * @param[in] x x 像素
 * @param[in] y y 像素
 * @param[in] z z 像素差
 * @param[in] delay 到位后延时（ms）
 * @retval none
 */
void Chassis_Visual_Pos(uint8_t target, int x, int y, int z, uint16_t delay)
{

}

/*-----------------------------------内部函数实现-----------------------------------*/

static void position_init(chassis_odometry_t *odom_init)
{
    if (odom_init == NULL)
    {
        return;
    }

    extern volatile float can_distence_x_m;
    extern volatile float can_distence_y_m;
    odom_init->position_x_ptr = &can_distence_x_m;
    odom_init->position_y_ptr = &can_distence_y_m;

    const static fp32 position_pid_x[3] = {
        CHASSIS_POSITION_PID_KP,
        CHASSIS_POSITION_PID_KI,
        CHASSIS_POSITION_PID_KD
    };
    const static fp32 position_pid_y[3] = {
        CHASSIS_POSITION_PID_KP,
        CHASSIS_POSITION_PID_KI,
        CHASSIS_POSITION_PID_KD
    };

    PID_init(&odom_init->position_x_pid, PID_POSITION, position_pid_x,
             CHASSIS_POSITION_PID_MAX_OUT, CHASSIS_POSITION_PID_MAX_IOUT);
    PID_init(&odom_init->position_y_pid, PID_POSITION, position_pid_y,
             CHASSIS_POSITION_PID_MAX_OUT, CHASSIS_POSITION_PID_MAX_IOUT);

    odom_init->mode = POSITION_MODE_DISABLE;
    odom_init->arrive_flag = 0;
    g_position_arrive_stable_cnt = 0;

    odom_init->position_x = *(odom_init->position_x_ptr);
    odom_init->position_y = *(odom_init->position_y_ptr);

    odom_init->position_x_set = odom_init->position_x;
    odom_init->position_y_set = odom_init->position_y;
    odom_init->yaw_set = 0.0f;

    odom_init->vx_out = 0.0f;
    odom_init->vy_out = 0.0f;

    // 测试：开启位置环
    //position_enable(1);
}

static void position_feedback_update(chassis_odometry_t *odom_update)
{
    if (odom_update == NULL)
    {
        return;
    }

    if (can_odom_new_data_flag == 0)
    {
        if (g_position_odom_miss_cnt < POSITION_ODOM_OFFLINE_MISS_THRESHOLD)
        {
            g_position_odom_miss_cnt++;
        }
        g_position_odom_fresh = (g_position_odom_miss_cnt < POSITION_ODOM_OFFLINE_MISS_THRESHOLD) ? 1 : 0;
        return;
    }

    can_odom_new_data_flag = 0;
    g_position_odom_miss_cnt = 0;
    g_position_odom_fresh = 1;

    odom_update->position_x = *(odom_update->position_x_ptr);
    odom_update->position_y = *(odom_update->position_y_ptr);
}

static void position_control_calc(chassis_odometry_t *odom_control)
{
    if (odom_control == NULL)
    {
        return;
    }

    if (odom_control->mode == POSITION_MODE_DISABLE || g_position_enable == 0)
    {
        odom_control->vx_out = 0.0f;
        odom_control->vy_out = 0.0f;
        odom_control->arrive_flag = 0;
        g_position_arrive_stable_cnt = 0;
        return;
    }

    if (g_position_odom_fresh == 0)
    {
        odom_control->vx_out = 0.0f;
        odom_control->vy_out = 0.0f;
        odom_control->arrive_flag = 0;
        g_position_arrive_stable_cnt = 0;
        return;
    }

    odom_control->position_x_set = g_position_target_x;
    odom_control->position_y_set = g_position_target_y;
    odom_control->yaw_set = g_position_target_yaw;

    fp32 err_x = odom_control->position_x_set - odom_control->position_x;
    fp32 err_y = odom_control->position_y_set - odom_control->position_y;

    // 到位判定仅使用原始位置误差，不受控制死区影响。
    fp32 dist = sqrtf(err_x * err_x + err_y * err_y);
    if (dist < CHASSIS_POSITION_ARRIVE_THRESHOLD)
    {
        if (g_position_arrive_stable_cnt < CHASSIS_POSITION_ARRIVE_STABLE_CYCLES)
        {
            g_position_arrive_stable_cnt++;
        }
    }
    else
    {
        g_position_arrive_stable_cnt = 0;
    }
    odom_control->arrive_flag = (g_position_arrive_stable_cnt >= CHASSIS_POSITION_ARRIVE_STABLE_CYCLES) ? 1 : 0;

    fp32 err_x_ctrl = err_x;
    fp32 err_y_ctrl = err_y;

    // Radial deadband avoids square dead-zone that can stall before arrival.
    fp32 control_deadband = CHASSIS_POSITION_DEADBAND;
    if (control_deadband > CHASSIS_POSITION_ARRIVE_THRESHOLD)
    {
        control_deadband = CHASSIS_POSITION_ARRIVE_THRESHOLD;
    }
    if (dist < control_deadband)
    {
        err_x_ctrl = 0.0f;
        err_y_ctrl = 0.0f;
    }

    odom_control->vx_out = PID_calc(&odom_control->position_x_pid, 0.0f, -err_x_ctrl);
    odom_control->vy_out = PID_calc(&odom_control->position_y_pid, 0.0f, -err_y_ctrl);
}

static void position_output(chassis_odometry_t *odom_output)
{
    if (odom_output == NULL)
    {
        return;
    }

    if (odom_output->mode == POSITION_MODE_DISABLE || g_position_enable == 0)
    {
        return;
    }

    chassis_set_world_target_yaw(odom_output->vx_out, odom_output->vy_out, odom_output->yaw_set);
}
