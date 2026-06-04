#include "tower.h"
#include "bsp_servo_pwm.h"
#include "Emm_V5.h"
#include "zdt_motor.h"
#include "cmsis_os2.h"
#include "Emm_V5.h"
#include "visual_identity.h"
#include "bsp_buzzer_pwm.h"
#include "chassis_task.h"
#include "position_task.h"


const int16_t target_x = 8;  // 目标误差X
const int16_t target_y = 7;  // 目标误差Y


/*
 * @brief 识别姿态
 * @param void
 * @retval void
 */
void identify_posture(void)
{
    // 夹爪打开
    bsp_gripper_state_set(JIAZHUA_DA_OPEN);
}

/*
 * @brief 复位姿态
 * @param void
 * @retval void
 */
void reset_posture(void)
{
    // 夹爪打开
    bsp_gripper_state_set(JIAZHUA_CLOSE);
    osDelay(200);
    gimbal_turn(3, 2000);
}

/*
 * @brief 抓取物料盘物料
 * @param void
 * @retval void
 */
void grab_plate_material(void)
{
    Motor_35_Move(Motor_35_DOWN, WULIAOPAN);
    osDelay(300);
    bsp_gripper_state_set(JIAZHUA_CLOSE);
    osDelay(200);
    Motor_35_Move(Motor_35_UP, WULIAOPAN);
    osDelay(700);
}

/*
 * @brief 放置物料到车上
 * @param void
 * @retval void
 */
void place_material(void)
{
    Motor_28_Move(QIAN, NAWULIAO_28); // 夹爪伸出
    osDelay(200);
    Motor_35_Move(Motor_35_DOWN, YUNHUO);
    osDelay(700);
    bsp_gripper_state_set(JIAZHUA_XIAO_OPEN);
    osDelay(200);
    Motor_28_Move(HOU, NAWULIAO_28);
    osDelay(200);
    Motor_35_Move(Motor_35_UP, YUNHUO);
    osDelay(800);
    
}

/*
 * @brief 抓取地板物料
 * @param void
 * @retval void
 */
void grab_materials_ground(uint8_t color, uint8_t first)
{
    gimbal_turn(color, 2000); // 云台转到拿物料位置，7对应拿物料位置，放下夹爪
    switch (color)
    {
    case 50:
        Motor_28_Move(QIAN, GROUND_GREEN); // 夹爪伸出
        break;
    case 49:
        Motor_28_Move(QIAN, GROUND_RED); // 夹爪伸出
        break;
    case 51:
        Motor_28_Move(QIAN, GROUND_BLUE); // 夹爪伸出
        break;
    }
    bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪张开打一点
    osDelay(100);
    Motor_35_Move(Motor_35_DOWN, DIMIAN); // 降下夹爪抓物料
    osDelay(1500);
    // 夹爪闭合
    bsp_gripper_state_set(JIAZHUA_CLOSE);
    osDelay(100);
    Motor_35_Move(Motor_35_UP, DIMIAN); // 抬起
    osDelay(1700);
    switch (color)
    {
    case 50:
        Motor_28_Move(HOU, GROUND_GREEN); // 夹爪收回
        break;
    case 49:
        Motor_28_Move(HOU, GROUND_RED); // 夹爪收回
        break;
    case 51:
        Motor_28_Move(HOU, GROUND_BLUE); // 夹爪收回
        break;
    }
    // Motor_28_Move(HOU, NAWULIAO_28); // 夹爪收回来,准备转
    osDelay(100);
}

/*
 * @brief 物料静态识别抓取
 * @param 1/2/3 红绿蓝
 * @retval void
*/
void grab_turntable_A(uint8_t color, uint8_t num)
{
    osDelay(100);
    gimbal_turn(7, 2000); // 云台转到拿物料位置，7对应拿物料位置，放下夹爪
    bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪完全张开
    osDelay(100);

    identify_materials(color); // 发送开始信息，告诉maixcam开始识别

    while (visual_data_ready == 0)
    {
        osDelay(20);
    }
    
    // 等物料到达指定位置
    while (is_error_little(30 ,30, 3) == 0)
    {
        osDelay(20);
    }
    
    visual_idle(); // 发送停止信息，告诉maixcam停止识别
    buzzer_rings(2000, 20, 100); // 物料移动到对应位置，蜂鸣器响一声
    
    grab_plate_material(); // 抓物料：降下夹爪抓物料
    gimbal_turn(num, 2000); // 转到对应位置，num对应放置位置
    place_material(); // 放置物料：1松开夹爪2恢复位置

    visual_idle(); // 再次发送停止信息，告诉maixcam停止识别
    osDelay(100);
}

/*
 * @brief 颜色环校准
 * @param void
 * @retval void
 */
void color_ring_calibration(uint8_t color)
{
    const fp32 error_to_speed = 0.00160f;
    const fp32 max_speed = 0.0150f;
    const uint32_t control_period_ms = 50U;
    fp32 x_cmd = 0.0f;
    fp32 y_cmd = 0.0f;
    fp32 yaw_hold = 0.0f;
    const chassis_move_t *chassis = get_chassis_move_data();
    static uint8_t timeout = 0;

    if (chassis != NULL)
    {
        yaw_hold = chassis->chassis_yaw;
    }
    gimbal_turn(50, 2000);
    bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪完全张开，避免挡住摄像头视野
    osDelay(100);
    visual_data_ready = 0;
    identify_color_rings(color);

    while (visual_data_ready == 0)
    {
        osDelay(control_period_ms);
    }

    position_disable(); // 关闭位置环，进入底盘速度控制模式
    chassis_cmd_disable_world_frame(); // 关闭世界系，进入车体系控制

    while (is_calibration_error_little() == 0)
    {
        int16_t error_x = g_maxicam_info.error_x - target_x;
        int16_t error_y = g_maxicam_info.error_y - target_y;

        // error_x>0: target on right -> move right (x negative). error_y>0: target down -> move back (y negative).
        x_cmd = error_to_speed * (fp32)error_y; 
        y_cmd = error_to_speed * (fp32)error_x;

        if (x_cmd > max_speed)
        {
            x_cmd = max_speed;
        }
        else if (x_cmd < -max_speed)
        {
            x_cmd = -max_speed;
        }

        if (y_cmd > max_speed)
        {
            y_cmd = max_speed;
        }
        else if (y_cmd < -max_speed)
        {
            y_cmd = -max_speed;
        }
        
        chassis_set_control_target(x_cmd, y_cmd, yaw_hold);
        osDelay(50); 
        chassis_set_control_target(0.0f, 0.0f, yaw_hold);
        osDelay(control_period_ms);
        timeout++;
        if (timeout > 120) // 大约6秒钟还没对准？算了，放弃微调，继续往下走正常抓取流程
        {
            break;
        }
    }

    chassis_set_control_target(0.0f, 0.0f, yaw_hold);
    chassis_cmd_disable_yaw_hold();
    osDelay(100);
    visual_idle();
    osDelay(100);
    timeout = 0; // 重置timeout，避免影响后续流程
}

/*
 * @brief 车上抓取物料
 * @param 
 * @param order      1/2/3
 * @retval void
 */
void grab_materials_car(uint8_t order, uint8_t first)     //车上抓取物料
{
    gimbal_turn(order, 2000); // 云台转到拿物料位置
    Motor_28_Move(QIAN, NAWULIAO_28); // 夹爪伸出
    bsp_gripper_state_set(JIAZHUA_XIAO_OPEN); // 夹爪张开一点
    osDelay(100);
    Motor_35_Move(Motor_35_DOWN, YUNHUO);
    osDelay(1200);
    bsp_gripper_state_set(JIAZHUA_CLOSE); // 夹爪闭合
    osDelay(100);
    Motor_35_Move(Motor_35_UP, TAIQI); // 抬起物料
    osDelay(800);
    Motor_28_Move(HOU, NAWULIAO_28); // 夹爪收回来,准备转
    osDelay(300);

}

/*
 * @brief 地面放物料
 * @param color 物料颜色 1/2/3 红绿蓝
 * @param none
 * @retval void
*/
void put_materials_ground(uint8_t color, uint16_t none)     //地面放物料
{
    gimbal_turn(color, 2300); // 转到放置的位置

    switch (color)
    {
    case 4:
    case 51:
        Motor_28_Move(QIAN, GROUND_RED); // 夹爪伸出
        osDelay(200);
        Motor_35_Move(Motor_35_DOWN, DIMIAN); // 降下夹爪
        osDelay(1500);
        bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪张开一点,放置物料
        osDelay(200);
        Motor_35_Move(Motor_35_UP, DIMIAN); // 升高夹爪，完成放置动作
        osDelay(1900);
        Motor_28_Move(HOU, GROUND_RED); // 夹爪收回来
        osDelay(300);
        
        break;
    case 5:
    case 50:
        Motor_28_Move(QIAN, GROUND_GREEN);   
        osDelay(200);
        Motor_35_Move(Motor_35_DOWN, DIMIAN);
        osDelay(1500);
        bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪张开一点,放置物料
        osDelay(200);
        Motor_35_Move(Motor_35_UP, DIMIAN); // 升高夹爪，完成放置动作
        osDelay(1900);
        Motor_28_Move(HOU, GROUND_GREEN); // 夹爪收回来
        osDelay(300);
        

        break;
    case 6:
    case 49:
        Motor_28_Move(QIAN, GROUND_BLUE); // 夹爪伸出
        osDelay(200);
        Motor_35_Move(Motor_35_DOWN, DIMIAN);
        osDelay(1500);
        bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪张开一点,放置物料
        osDelay(200);
        Motor_35_Move(Motor_35_UP, DIMIAN); // 升高夹爪，完成放置动作
        osDelay(1900);
        Motor_28_Move(HOU, GROUND_BLUE); // 夹爪收回来
        osDelay(300);
        

        break;
    
    default:
        break;
    }
    osDelay(200);

}

/*
 * @brief 车上放物料
 * @param order      1/2/3
 * @retval void
 */
void put_materials_car(uint8_t order)
{
    gimbal_turn(order, 2000); // 云台转到放物料位置
    place_material(); // 放置物料
}

/*
 * @brief 物料校准
 * @param mode 1 校准转盘物料 mode 2 校准码垛物料
 * @retval void
 */
void color_materials_calibration(uint8_t color, uint8_t mode)
{
    const fp32 error_to_speed = 0.0014f;
    const fp32 max_speed = 0.09f;
    const uint32_t control_period_ms = 50U;
    fp32 x_cmd = 0.0f;
    fp32 y_cmd = 0.0f;
    fp32 yaw_hold = 0.0f;
    const chassis_move_t *chassis = get_chassis_move_data();

    uint8_t arrow_error_x = 0;
    uint8_t arrow_error_y = 0;

    if (mode == 1) // 码垛物料校准，误差要求较小一些
    {
        arrow_error_x = 2;
        arrow_error_y = 2;
    } else {
            arrow_error_x = 5;
            arrow_error_y = 5;
    }
    
    if (chassis != NULL)
    {
        yaw_hold = chassis->chassis_yaw;
    }
    gimbal_turn(50, 1200);
    bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪完全张开，避免挡住摄像头视野
    osDelay(100);
    visual_data_ready = 0;
    identify_materials(color);

    while (visual_data_ready == 0)
    {
        osDelay(control_period_ms);
    }

    position_disable(); // 关闭位置环，进入底盘速度控制模式
    chassis_cmd_disable_world_frame(); // 关闭世界系，进入车体系控制

    // 【新增】进入视觉微调循环前，调用一次函数并传入 reset_state = 1，清空上一轮可能残留的历史垃圾数据
    //apply_y_axis_speed_lock(0, 0, 0.0f, 1);

    while (is_error_little(arrow_error_x, arrow_error_y, 5) == 0)
    {
        int16_t error_x = g_maxicam_info.error_x;
        int16_t error_y = g_maxicam_info.error_y;

        // error_x>0: target on right -> move right (x negative). error_y>0: target down -> move back (y negative).
        x_cmd = error_to_speed * (fp32)error_y; 
        y_cmd = error_to_speed * (fp32)error_x;

        if (x_cmd > max_speed)
        {
            x_cmd = max_speed;
        }
        else if (x_cmd < -max_speed)
        {
            x_cmd = -max_speed;
        }

        if (y_cmd > max_speed)
        {
            y_cmd = max_speed;
        }
        else if (y_cmd < -max_speed)
        {
            y_cmd = -max_speed;
        }

        if (mode == 0)
        {
            if ((ABS(error_x) > 50) || (ABS(error_y) > 160)) // 如果误差较大，不启用校准
            {
                x_cmd = 0;
                y_cmd = 0;
            }
        }
        chassis_set_control_target(x_cmd, y_cmd, yaw_hold);
        osDelay(control_period_ms);
    }

    chassis_set_control_target(0.0f, 0.0f, yaw_hold);
    chassis_cmd_disable_yaw_hold();
    osDelay(100);
    visual_idle();
    osDelay(100);
}

/*
 * @brief 物料动态识别抓取
 * @param 1/2/3 红绿蓝
 * @retval void
*/
void grab_turntable_B(uint8_t color, uint8_t num)
{
    osDelay(100);
    //gimbal_turn(7, 2000); // 云台转到拿物料位置，7对应拿物料位置，放下夹爪
    bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪完全张开
    //Motor_28_Move(QIAN, WULIAOPAN_MATERIAL); // 夹爪伸出
    osDelay(100);

    color_materials_calibration(color, 0);
    
    visual_idle(); // 发送停止信息，告诉maixcam停止识别
    buzzer_rings(2000, 20, 100); // 物料移动到对应位置，蜂鸣器响一声
    
    grab_plate_material(); // 抓物料：降下夹爪抓物料
    //Motor_28_Move(HOU, WULIAOPAN_MATERIAL); // 夹爪收回,准备转
    gimbal_turn(num, 2000); // 转到对应位置，num对应放置位置
    place_material(); // 放置物料：1松开夹爪2恢复位置

    visual_idle(); // 再次发送停止信息，告诉maixcam停止识别
    osDelay(100);
}

/*
 * @brief 放置物料到物料上
 * @param color  物料颜色
 * @param mode   校准模式
 * @retval void
 */
void put_materials_materials(uint8_t color, uint16_t none)
{
    gimbal_turn(color, 2300); // 转到放置的位置
    switch (color)
    {
    case 4:
    case 51:
        Motor_28_Move(QIAN, GROUND_RED); // 夹爪伸出
        osDelay(200);
        Motor_35_Move(Motor_35_DOWN, MADUO); // 降下夹爪
        osDelay(1100);
        bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪张开一点,放置物料
        osDelay(200);
        Motor_35_Move(Motor_35_UP, MADUO); // 升高夹爪，完成放置动作
        osDelay(1200);
        Motor_28_Move(HOU, GROUND_RED); // 夹爪收回来
        osDelay(300);
        
        break;
    case 5:
    case 50:
        Motor_28_Move(QIAN, GROUND_GREEN);   
        osDelay(200);
        Motor_35_Move(Motor_35_DOWN, MADUO);
        osDelay(1100);
        bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪张开一点,放置物料
        osDelay(200);
        Motor_35_Move(Motor_35_UP, MADUO); // 升高夹爪，完成放置动作
        osDelay(1200);
        Motor_28_Move(HOU, GROUND_GREEN); // 夹爪收回来
        osDelay(300);
        

        break;
    case 6:
    case 49:
        Motor_28_Move(QIAN, GROUND_BLUE); // 夹爪伸出
        osDelay(200);
        Motor_35_Move(Motor_35_DOWN, MADUO);
        osDelay(1100);
        bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪张开一点,放置物料
        osDelay(200);
        Motor_35_Move(Motor_35_UP, MADUO); // 升高夹爪，完成放置动作
        osDelay(1200);
        Motor_28_Move(HOU, GROUND_BLUE); // 夹爪收回来
        osDelay(300);
        

        break;
    
    default:
        break;
    }
    osDelay(200);
}