/**
 * @file chassis_task.c
 * @brief 底盘任务
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

/*-----------------------------------头文件引用-----------------------------------*/

#include "chassis_task.h"
#include "CAN_receive.h"
#include "dc_motor.h"
#include "X_V2.h"
#include <math.h>
#include "arm_math.h"
#include "cmsis_os2.h"


/*-----------------------------------宏定义-----------------------------------*/



/*-----------------------------------变量声明-----------------------------------*/

// 底盘运动数据
chassis_move_t chassis_move;

float test_angle_set;

fp32 vx_set_tem, vy_set_tem, offset_angle;

/*------------------------ 底盘指令（由外部任务/主循环写入） -------------------------*/

static volatile fp32 g_chassis_cmd_vx = 0.0f;
static volatile fp32 g_chassis_cmd_vy = 0.0f;
static volatile fp32 g_chassis_cmd_wz = 0.0f;
static volatile fp32 g_chassis_cmd_yaw = 0.0f;
static volatile uint8_t g_chassis_yaw_hold_enable = 0;

// 世界系速度控制相关
static volatile fp32 g_chassis_cmd_vx_world = 0.0f;
static volatile fp32 g_chassis_cmd_vy_world = 0.0f;
static volatile uint8_t g_chassis_world_frame_enable = 0;  // 0=车体系, 1=世界系

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief  初始化chassis_move变量，包括pid初始化、遥控器指针初始化、3508底盘电机指针初始化、云台电机初始化、陀螺仪角度指针初始化
 * @param[out] chassis_move_init chassis_move变量指针
 * @retval none
 */
static void chassis_init(chassis_move_t *chassis_move_init);

/**
 * @brief  底盘测量数据更新，包括电机速度、欧拉角度、机器人速度
 * @param[out] chassis_move_update chassis_move变量指针
 * @retval none
 */
static void chassis_feedback_update(chassis_move_t *chassis_move_update);

/**
 * @brief  设置底盘控制设定点，三个运动控制值由chassis_behaviour_control_set设置
 * @param[out] chassis_move_control chassis_move变量指针
 * @retval none
 */
static void chassis_set_control(chassis_move_t *chassis_move_control);

/**
 * @brief  通过三个参数计算四个麦轮速度
 * @param[in] vx_set 纵向速度
 * @param[in] vy_set 横移速度
 * @param[in] wz_set 旋转角速度
 * @param[out] wheel_speed 四个麦轮速度的数组
 * @retval none
 */
static void chassis_vector_to_mecanum_wheel_speed(const fp32 vx_set, const fp32 vy_set, const fp32 wz_set, fp32 wheel_speed[4]);

/**
 * @brief  控制循环，根据控制设定值，计算电机电流值，进行控制
 * @param[out] chassis_move_control_loop chassis_move变量指针
 * @retval none
 */
static void chassis_control_loop(chassis_move_t *chassis_move_control_loop);

/**
 * @brief 将四个轮子的目标线速度(m/s)转换为电机转速(RPM)，并发送到CAN总线
 * @param w1_speed_ms 1号轮目标速度 (m/s)
 * @param w2_speed_ms 2号轮目标速度 (m/s)
 * @param w3_speed_ms 3号轮目标速度 (m/s)
 * @param w4_speed_ms 4号轮目标速度 (m/s)
 */
static void chassis_execute_wheel_speeds(fp32 w1_speed_ms, fp32 w2_speed_ms, fp32 w3_speed_ms, fp32 w4_speed_ms);

// ------------------------- 对外接口实现 -------------------------
void chassis_cmd_set_speed(fp32 vx, fp32 vy)
{
  g_chassis_cmd_vx = vx;
  g_chassis_cmd_vy = vy;
}

void chassis_cmd_set_wz(fp32 wz_rad_s)
{
  g_chassis_cmd_wz = wz_rad_s;
}

void chassis_cmd_set_yaw_abs(fp32 yaw_rad)
{
  g_chassis_cmd_yaw = rad_format(yaw_rad);
  g_chassis_yaw_hold_enable = 1;
}

void chassis_cmd_set_yaw_rel(fp32 yaw_delta_rad)
{
  // 基于当前底盘角度锁存一次目标角，避免外部周期性调用导致目标不断累加
  g_chassis_cmd_yaw = rad_format(chassis_move.chassis_yaw + yaw_delta_rad);
  g_chassis_yaw_hold_enable = 1;
}

void chassis_cmd_disable_yaw_hold(void)
{
  g_chassis_yaw_hold_enable = 0;
}

void chassis_set_control_target(fp32 vx, fp32 vy, fp32 yaw_rad)
{
  chassis_cmd_set_speed(vx, vy);
  chassis_cmd_set_yaw_abs(yaw_rad);
}

// ------------------------- 世界系速度控制接口 -------------------------
void chassis_cmd_set_speed_world(fp32 vx_world, fp32 vy_world)
{
  g_chassis_cmd_vx_world = vx_world;
  g_chassis_cmd_vy_world = vy_world;
  g_chassis_world_frame_enable = 1;
}

void chassis_cmd_set_speed_body(fp32 vx, fp32 vy)
{
  g_chassis_cmd_vx = vx;
  g_chassis_cmd_vy = vy;
  g_chassis_world_frame_enable = 0;
}

void chassis_cmd_disable_world_frame(void)
{
  g_chassis_world_frame_enable = 0;
}

uint8_t chassis_cmd_is_world_frame(void)
{
  return g_chassis_world_frame_enable;
}

void chassis_set_world_target(fp32 vx_world, fp32 vy_world, fp32 wz_rad_s)
{
  chassis_cmd_set_speed_world(vx_world, vy_world);
  chassis_cmd_disable_yaw_hold();  // 世界系+旋转模式下用wz直接控制
  chassis_cmd_set_wz(wz_rad_s);
}

void chassis_set_world_target_yaw(fp32 vx_world, fp32 vy_world, fp32 yaw_rad)
{
  chassis_cmd_set_speed_world(vx_world, vy_world);
  chassis_cmd_set_yaw_abs(yaw_rad);  // 世界系+航向锁定模式
}

/*-----------------------------------函数实现-----------------------------------*/

void chassis_task(void *argument)
{
  osDelay(CHASSIS_TASK_INIT_TIME); // 等待系统稳定
  // 底盘初始化,包括pid初始化、jgb37底盘电机指针初始化、陀螺仪角度指针初始化
  chassis_init(&chassis_move);
  
  // 获取单片机从开机到当前时刻的系统滴答数
  uint32_t PreviousWakeTime = osKernelGetTickCount();

  for (;;)
  {
    // 1. 底盘数据更新（从电机反馈中获取当前状态）
    chassis_feedback_update(&chassis_move);
    
    // 2. 底盘控制量设置（整合遥控器、上位机的速度指令）
    chassis_set_control(&chassis_move);

    // 3. 底盘运动学解算（计算出四个轮子需要的线速度 m/s）
    chassis_control_loop(&chassis_move);
    
    // 4. 执行轮速：将解算好的四个轮子目标速度(m/s)直接塞进我们的新函数里！
    chassis_execute_wheel_speeds(chassis_move.motor_chassis[0].speed_set,
                                 chassis_move.motor_chassis[1].speed_set,
                                 chassis_move.motor_chassis[2].speed_set,
                                 chassis_move.motor_chassis[3].speed_set);
  
    PreviousWakeTime += CHASSIS_CONTROL_TIME_MS; 
    osDelayUntil(PreviousWakeTime);

    //osDelay(CHASSIS_CONTROL_TIME_MS); // 40ms周期
  }
}


static void chassis_init(chassis_move_t *chassis_move_init)
{
  if (chassis_move_init == NULL)
  {
    return;
  }

  // 底盘速度环pid值
  const static fp32 motor_speed_pid[3] = {JGB37_MOTOR_SPEED_PID_KP, JGB37_MOTOR_SPEED_PID_KI, JGB37_MOTOR_SPEED_PID_KD};

  // 底盘角度pid值
  const static fp32 chassis_yaw_pid[3] = {CHASSIS_FOLLOW_GIMBAL_PID_KP, CHASSIS_FOLLOW_GIMBAL_PID_KI, CHASSIS_FOLLOW_GIMBAL_PID_KD};

  const static fp32 chassis_x_order_filter[1] = {CHASSIS_ACCEL_X_NUM}; // 
  const static fp32 chassis_y_order_filter[1] = {CHASSIS_ACCEL_Y_NUM};
  uint8_t i;

  // 底盘开机状态为原始模式
  //chassis_move_init->chassis_mode = CHASSIS_VECTOR_RAW;
  chassis_move_init->chassis_mode = CHASSIS_VECTOR_NO_FOLLOW_YAW; // 默认航向保持模式
  
  // 获取陀螺仪姿态角指针, 使用来自 CAN 的外部陀螺仪数据
  chassis_move_init->chassis_INS_angle = &can_gyro_yaw_rad;

  // 获取底盘电机数据指针，并初始化pid
  for (i = 0; i < 4; i++)
  {
    //chassis_move_init->motor_chassis[i].chassis_motor_measure = get_chassis_motor_measure_point(i);
    PID_init(&chassis_move_init->motor_speed_pid[i], PID_POSITION, motor_speed_pid, JGB37_MOTOR_SPEED_PID_MAX_OUT, JGB37_MOTOR_SPEED_PID_MAX_IOUT);
  }
  // 初始化角度pid
  PID_init(&chassis_move_init->chassis_angle_pid, PID_POSITION, chassis_yaw_pid, CHASSIS_FOLLOW_GIMBAL_PID_MAX_OUT, CHASSIS_FOLLOW_GIMBAL_PID_MAX_IOUT);

  // 用一阶滤波代替斜波函数生成
  first_order_filter_init(&chassis_move_init->chassis_cmd_slow_set_vx, CHASSIS_CONTROL_TIME, chassis_x_order_filter);
  first_order_filter_init(&chassis_move_init->chassis_cmd_slow_set_vy, CHASSIS_CONTROL_TIME, chassis_y_order_filter);

  // 最大、最小速度
  chassis_move_init->vx_max_speed = NORMAL_MAX_CHASSIS_SPEED_X * 2.0f / 3.0f;
  chassis_move_init->vx_min_speed = -NORMAL_MAX_CHASSIS_SPEED_X * 2.0f / 3.0f;

  chassis_move_init->vy_max_speed = NORMAL_MAX_CHASSIS_SPEED_Y * 2.0f / 3.0f;
  chassis_move_init->vy_min_speed = -NORMAL_MAX_CHASSIS_SPEED_Y * 2.0f / 3.0f;

  // 更新数据
  chassis_feedback_update(chassis_move_init);

  // 默认锁存当前航向角作为目标，避免上电后角度环突变
  g_chassis_cmd_yaw = chassis_move_init->chassis_yaw;
  chassis_move_init->chassis_yaw_set = chassis_move_init->chassis_yaw;
}

static void chassis_feedback_update(chassis_move_t *chassis_move_update) // 1
{
  if (chassis_move_update == NULL)
  {
    return;
  }
  
  uint8_t i = 0;
  for (i = 0; i < 4; i++)
  {
    // 发送读取实时转速指令，并使用CAN回传的电机速度反馈
    X_V2_Read_Sys_Params((uint8_t)(i + 1U), S_VEL);
    osDelay(1); // 让CAN给can电机的接收留时间
    // 更新电机速度，加速度是速度的PID微分
    chassis_move_update->motor_chassis[i].accel = chassis_move_update->motor_speed_pid[i].Dbuf[0] * CHASSIS_CONTROL_FREQUENCE;
    chassis_move_update->motor_chassis[i].speed = can_y42_motor_measure[i].speed_rpm * RPM_TO_SPEED_MS;
  }

  // 更新底盘纵向速度x、平移速度y、旋转角速度wz，坐标系为右手系（向前为x正，向左为y正，向上为z正）
  chassis_move_update->vx = (chassis_move_update->motor_chassis[0].speed - chassis_move_update->motor_chassis[1].speed + chassis_move_update->motor_chassis[2].speed - chassis_move_update->motor_chassis[3].speed) * MOTOR_SPEED_TO_CHASSIS_SPEED_VX;
  chassis_move_update->vy = (-chassis_move_update->motor_chassis[0].speed - chassis_move_update->motor_chassis[1].speed + chassis_move_update->motor_chassis[2].speed + chassis_move_update->motor_chassis[3].speed) * MOTOR_SPEED_TO_CHASSIS_SPEED_VY;
  chassis_move_update->wz = (-chassis_move_update->motor_chassis[0].speed - chassis_move_update->motor_chassis[1].speed - chassis_move_update->motor_chassis[2].speed - chassis_move_update->motor_chassis[3].speed) * MOTOR_SPEED_TO_CHASSIS_SPEED_WZ / MOTOR_DISTANCE_TO_CENTER;

  // 计算底盘姿态角度
  chassis_move_update->chassis_yaw = rad_format(*(chassis_move_update->chassis_INS_angle));
}

static void chassis_set_control(chassis_move_t *chassis_move_control) // 2
{

  if (chassis_move_control == NULL)
  {
    return;
  }
  // 外部输入：采用(x, y)机体系速度指令（单位 m/s）
  // 约定：x 正方向=向左，y 正方向=向前
  // 内部麦轮解算仍使用(vx, vy)：vx=向前，vy=向左
  fp32 vx_set, vy_set;
  fp32 wz_set = g_chassis_cmd_wz;

  // 世界系/车体系速度选择
  if (g_chassis_world_frame_enable)
  {
    // 世界系(x,y) -> 车体系(vx,vy)：先把(x,y)转为(vx,vy)，再做旋转变换
    // (vx,vy) = (y,x)
    // vx_body =  vx_world * cos(yaw) + vy_world * sin(yaw)
    // vy_body = -vx_world * sin(yaw) + vy_world * cos(yaw)
    fp32 yaw = chassis_move_control->chassis_yaw;
    fp32 cos_yaw = cosf(yaw);
    fp32 sin_yaw = sinf(yaw);
    fp32 x_world = g_chassis_cmd_vx_world;
    fp32 y_world = g_chassis_cmd_vy_world;

    // 方向修正：保证 x 正为“向左”，y 正为“向前”
    x_world *= CHASSIS_VX_SIGN;
    y_world *= CHASSIS_VY_SIGN;

    // 转成(vx,vy)：vx=向前，vy=向左
    fp32 vx_world = y_world;
    fp32 vy_world = x_world;

    vx_set =  vx_world * cos_yaw + vy_world * sin_yaw;
    vy_set = -vx_world * sin_yaw + vy_world * cos_yaw;
  }
  else
  {
    // 车体系(x,y)速度，先做方向修正，再转成(vx,vy)
    fp32 x_body = g_chassis_cmd_vx;
    fp32 y_body = g_chassis_cmd_vy;

    // 方向修正：保证 x 正为“向左”，y 正为“向前”
    x_body *= CHASSIS_VX_SIGN;
    y_body *= CHASSIS_VY_SIGN;

    // (vx,vy) = (y,x)
    vx_set = y_body;
    vy_set = x_body;
  }

  // vx/vy 进行一阶低通，减缓设定突变
  first_order_filter_cali(&chassis_move_control->chassis_cmd_slow_set_vx, vx_set);
  first_order_filter_cali(&chassis_move_control->chassis_cmd_slow_set_vy, vy_set);
  vx_set = chassis_move_control->chassis_cmd_slow_set_vx.out;
  vy_set = chassis_move_control->chassis_cmd_slow_set_vy.out;

  chassis_move_control->vx_set = fp32_constrain(vx_set, chassis_move_control->vx_min_speed, chassis_move_control->vx_max_speed);
  chassis_move_control->vy_set = fp32_constrain(vy_set, chassis_move_control->vy_min_speed, chassis_move_control->vy_max_speed);

  // 航向角闭环：角度PID输出 wz_set (rad/s)
  if (g_chassis_yaw_hold_enable)
  {
    fp32 yaw_target = rad_format(g_chassis_cmd_yaw);
    chassis_move_control->chassis_yaw_set = yaw_target;

    // 用弧度环的“最短角误差”，避免跨 ±PI 时跳变
    fp32 yaw_err = rad_format(yaw_target - chassis_move_control->chassis_yaw);

    // PID_calc 内部直接做 set-ref；这里将 ref 固定为0，set为误差，实现对误差的PID
    chassis_move_control->wz_set = PID_calc(&chassis_move_control->chassis_angle_pid, 0.0f, yaw_err);
  }
  else
  {
    // 旋转速度控制（开环/速度环输入），可按需要缩放
    chassis_move_control->wz_set = wz_set * CHASSIS_WZ_SET_SCALE;
  }

  test_angle_set = chassis_move_control->chassis_yaw_set;
}


static void chassis_vector_to_mecanum_wheel_speed(const fp32 vx_set, const fp32 vy_set, const fp32 wz_set, fp32 wheel_speed[4]) // 3.5
{
  // 数组中四个轮子按顺序分别为左前、右前、左后、右后
  vx_set_tem = vx_set;
  vy_set_tem = vy_set;
  //   FL = vx - vy - L*wz
  //   FR = vx + vy - L*wz
  //   RL = vx + vy + L*wz
  //   RR = vx - vy + L*wz
  wheel_speed[0] =  vx_set_tem - vy_set_tem - MOTOR_DISTANCE_TO_CENTER * wz_set;   // FL
  wheel_speed[1] =  vx_set_tem + vy_set_tem + MOTOR_DISTANCE_TO_CENTER * wz_set;   // FR
  wheel_speed[2] = -(vx_set_tem + vy_set_tem - MOTOR_DISTANCE_TO_CENTER * wz_set); // RL (pre-flip)
  wheel_speed[3] = -(vx_set_tem - vy_set_tem + MOTOR_DISTANCE_TO_CENTER * wz_set); // RR (pre-flip)
}

static void chassis_control_loop(chassis_move_t *chassis_move_control_loop)
{
  fp32 max_vector = 0.0f, vector_rate = 0.0f;
  fp32 temp = 0.0f;
  fp32 wheel_speed[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  uint8_t i = 0;

  // 1. 停机判定：当目标速度趋近于 0 时，触发停机标志
  uint8_t stop_cmd = 0;
  if (fabsf(chassis_move_control_loop->vx_set) < CHASSIS_STOP_EPS_V &&
      fabsf(chassis_move_control_loop->vy_set) < CHASSIS_STOP_EPS_V &&
      fabsf(chassis_move_control_loop->wz_set) < CHASSIS_STOP_EPS_WZ)
  {
    stop_cmd = 1;
  }

  // 2. 麦轮运动学分解 (根据 vx, vy, wz 算出四个轮子的理论速度 m/s)
  chassis_vector_to_mecanum_wheel_speed(chassis_move_control_loop->vx_set, chassis_move_control_loop->vy_set, chassis_move_control_loop->wz_set, wheel_speed);

  // 3. 轮端方向修正 (这里乘以 CHASSIS_WHEELx_DIR 修正轮子安装的极性差异)
  wheel_speed[0] *= CHASSIS_WHEEL1_DIR;
  wheel_speed[1] *= CHASSIS_WHEEL2_DIR;
  wheel_speed[2] *= CHASSIS_WHEEL3_DIR;
  wheel_speed[3] *= CHASSIS_WHEEL4_DIR;

  // 4. 滤除速度设定死区，并将临时速度保存进结构体
  for (i = 0; i < 4; i++)
  {
    if (fabsf(wheel_speed[i]) < CHASSIS_SPEED_SET_DEADBAND)
    {
      wheel_speed[i] = 0.0f;
    }
    chassis_move_control_loop->motor_chassis[i].speed_set = wheel_speed[i];
    
    // 寻找四个轮子中绝对速度最大的那一个
    temp = fabs(chassis_move_control_loop->motor_chassis[i].speed_set);
    if (max_vector < temp)
    {
      max_vector = temp;
    }
  }

  // 5. 如果是停机状态，把目标速度全部清零，并直接返回
  if (stop_cmd)
  {
    for (i = 0; i < 4; i++)
    {
      chassis_move_control_loop->motor_chassis[i].speed_set = 0.0f;
    }
    return; // <--- 注意：这里直接 return 退出了
  }

  // 6. 如果最大轮速超过了设定上限，对四个轮子进行等比例缩小，防止因为超速导致底盘打转
  if (max_vector > MAX_WHEEL_SPEED)
  {
    vector_rate = MAX_WHEEL_SPEED / max_vector;
    for (i = 0; i < 4; i++)
    {
      chassis_move_control_loop->motor_chassis[i].speed_set *= vector_rate;
    }
  }
}

static void chassis_execute_wheel_speeds(fp32 w1_speed_ms, fp32 w2_speed_ms, fp32 w3_speed_ms, fp32 w4_speed_ms)
{
    int16_t motor_target_rpm[4] = {0};

    // 物理量转换：速度(m/s) / 转换系数 = 电机转速(RPM)
    motor_target_rpm[0] = (int16_t)(w1_speed_ms / RPM_TO_SPEED_MS);
    motor_target_rpm[1] = (int16_t)(w2_speed_ms / RPM_TO_SPEED_MS);
    motor_target_rpm[2] = (int16_t)(w3_speed_ms / RPM_TO_SPEED_MS);
    motor_target_rpm[3] = (int16_t)(w4_speed_ms / RPM_TO_SPEED_MS);

    // 调用底层 CAN 接口发送数据
    can_send_chassis_speed(motor_target_rpm[0], 
                           motor_target_rpm[1], 
                           motor_target_rpm[2], 
                           motor_target_rpm[3]);
}

const chassis_move_t *get_chassis_move_data(void)
{
  return &chassis_move;
}