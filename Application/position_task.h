/**
 * @file position_task.h
 * @brief Chassis position outer-loop control module.
 */

#ifndef POSITION_TASK_H
#define POSITION_TASK_H

#include "struct_typedef.h"
#include "pid.h"

#define CHASSIS_POSITION_TASK_INIT_TIME 357

#define POSITION_CONTROL_TIME      0.1f
#define POSITION_CONTROL_TIME_MS   100

#define CHASSIS_POSITION_PID_KP         0.9f
#define CHASSIS_POSITION_PID_KI         0.0f
#define CHASSIS_POSITION_PID_KD         0.05f
#define CHASSIS_POSITION_PID_MAX_OUT    0.60f
#define CHASSIS_POSITION_PID_MAX_IOUT   0.1f

#define CHASSIS_POSITION_ROOT_KP        0.6f
#define CHASSIS_POSITION_ROOT_KD        0.2f

#define CHASSIS_POSITION_DEADBAND          0.005f
#define CHASSIS_POSITION_ARRIVE_THRESHOLD  0.010f
#define CHASSIS_POSITION_ARRIVE_STABLE_CYCLES 3U
#define CHASSIS_POSITION_X_LIMIT_M         2.3f
#define CHASSIS_POSITION_Y_LIMIT_M         2.3f

#define YOUBIAN -1.5707963f
#define ZUOBIAN 1.570963f
#define QIANMIAN 0.0f
#define HOUMIAN 3.1415926f

typedef enum {
    CHASSIS_MODE_MANUAL = 0,
    CHASSIS_MODE_AUTO_NAV,
    CHASSIS_MODE_VISUAL_ALIGN,
} chassis_position_mode_e;

typedef enum
{
    POSITION_MODE_DISABLE = 0,
    POSITION_MODE_POSITION,
} position_mode_e;

typedef struct
{
    position_mode_e mode;

    const volatile fp32 *position_x_ptr;
    const volatile fp32 *position_y_ptr;

    pid_type_def position_x_pid;
    pid_type_def position_y_pid;

    fp32 position_x;
    fp32 position_y;

    fp32 position_x_set;
    fp32 position_y_set;

    fp32 vx_out;
    fp32 vy_out;

    fp32 yaw_set;

    uint8_t arrive_flag;
} chassis_odometry_t;

extern void global_position_init(void);
extern void position_task_cycle(void);

extern void position_set_position(fp32 x_m, fp32 y_m);
extern void position_set_position_yaw(fp32 x_m, fp32 y_m, fp32 yaw_rad);
extern void position_enable(uint8_t latch_current_target);
extern void position_disable(void);
extern uint8_t position_is_arrived(void);
extern const chassis_odometry_t *get_position_data(void);
extern void Chassis_Go_Pos(fp32 x_m, fp32 y_m, fp32 yaw_rad, uint16_t delay_ms);

#endif /* POSITION_TASK_H */
