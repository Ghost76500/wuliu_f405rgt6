#include "tower.h"
#include "bsp_servo_pwm.h"
#include "Emm_V5.h"
#include "zdt_motor.h"
#include "cmsis_os2.h"
#include "Emm_V5.h"
#include "visual_identity.h"
#include "bsp_buzzer_pwm.h"
#include "chassis_task.h"

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
    Motor_35_Move(Motor_35_DOWN, 21200);
    osDelay(1200);
    bsp_gripper_state_set(JIAZHUA_CLOSE);
    Motor_35_Move(Motor_35_UP, 21200);
    osDelay(1200);
}

/*
 * @brief 放置物料到车上
 * @param void
 * @retval void
 */
void place_material(void)
{
    Motor_28_Move(QIAN, 1700);
    osDelay(200);
    Motor_35_Move(Motor_35_DOWN, 9500);
    osDelay(800);
    bsp_gripper_state_set(JIAZHUA_XIAO_OPEN);
    osDelay(200);
    Motor_28_Move(HOU, 1700);
    osDelay(200);
    Motor_35_Move(Motor_35_UP, 9500);
    osDelay(800);
    
}

/*
 * @brief 抓取地板物料
 * @param void
 * @retval void
 */
void grab_floor_material(void)
{
    // 先张开夹爪并伸出到地面抓取位置
    bsp_gripper_state_set(JIAZHUA_DA_OPEN);
    osDelay(100);
    Motor_28_Move(QIAN, 1700);
    osDelay(200);

    // 下探抓取地面物料
    Motor_35_Move(Motor_35_DOWN, DIMIAN);
    osDelay(1500);

    // 闭合夹爪夹住物料
    bsp_gripper_state_set(JIAZHUA_CLOSE);
    osDelay(100);

    // 抬起并收回，避免拖地
    Motor_35_Move(Motor_35_UP, DIMIAN);
    osDelay(1700);
    Motor_28_Move(HOU, 1700);
    osDelay(200);
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
    while (is_error_little() == 0)
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
    const fp32 error_to_speed = 0.00125f;
    const fp32 max_speed = 0.0125f;
    const uint32_t control_period_ms = 100U;
    fp32 x_cmd = 0.0f;
    fp32 y_cmd = 0.0f;
    fp32 yaw_hold = 0.0f;
    const chassis_move_t *chassis = get_chassis_move_data();

    if (chassis != NULL)
    {
        yaw_hold = chassis->chassis_yaw;
    }

    visual_data_ready = 0;
    identify_color_rings(color);

    while (visual_data_ready == 0)
    {
        osDelay(control_period_ms);
    }

    chassis_cmd_disable_world_frame(); // 关闭世界系，进入车体系控制

    while (is_calibration_error_little() == 0)
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
        
        chassis_set_control_target(x_cmd, y_cmd, yaw_hold);
        osDelay(32); 
        chassis_set_control_target(0.0f, 0.0f, yaw_hold);
        osDelay(control_period_ms);
    }

    chassis_set_control_target(0.0f, 0.0f, yaw_hold);
    chassis_cmd_disable_yaw_hold();
    osDelay(100);
    visual_idle();
    osDelay(100);
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
    Motor_28_Move(QIAN, 1700); // 夹爪伸出
    bsp_gripper_state_set(JIAZHUA_XIAO_OPEN); // 夹爪张开一点
    osDelay(100);
    Motor_35_Move(Motor_35_DOWN, YUNHUO);
    osDelay(1200);
    bsp_gripper_state_set(JIAZHUA_CLOSE); // 夹爪闭合
    osDelay(100);
    Motor_35_Move(Motor_35_UP, 1500); // 抬起物料
    osDelay(400);
    Motor_28_Move(HOU, 1700); // 夹爪收回来,准备转
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
    switch (color)
    {
        case '1': // 红色
            color = 4;
            break;
        case '2': // 绿色
            color = 5;
            break;
        case '3': // 蓝色
            color = 6;
            break;
    default:
        break;
    }
    gimbal_turn(color, 2300); // 转到放置的位置

    switch (color)
    {
    case 4:
        Motor_28_Move(QIAN, GROUND_RED); // 夹爪伸出
        osDelay(200);
        Motor_35_Move(Motor_35_DOWN, 44700); // 降下夹爪
        osDelay(1500);
        bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪张开一点,放置物料
        osDelay(200);
        Motor_28_Move(HOU, GROUND_RED); // 夹爪收回来
        osDelay(300);
        Motor_35_Move(Motor_35_UP, 52700); // 升高夹爪，完成放置动作
        osDelay(1700);

        break;
    case 5:
        Motor_28_Move(QIAN, GROUND_GREEN);   
        osDelay(200);
        Motor_35_Move(Motor_35_DOWN, 44700);
        osDelay(1500);
        bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪张开一点,放置物料
        osDelay(200);
        Motor_28_Move(HOU, GROUND_GREEN); // 夹爪收回来
        osDelay(300);
        Motor_35_Move(Motor_35_UP, 52700); // 升高夹爪，完成放置动作
        osDelay(1700);

        break;
    case 6:
        Motor_28_Move(QIAN, GROUND_BLUE); // 夹爪伸出
        osDelay(200);
        Motor_35_Move(Motor_35_DOWN, 44700);
        osDelay(1500);
        bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪张开一点,放置物料
        osDelay(200);
        Motor_28_Move(HOU, GROUND_BLUE); // 夹爪收回来
        osDelay(300);
        Motor_35_Move(Motor_35_UP, 52700); // 升高夹爪，完成放置动作
        osDelay(1700);

        break;
    
    default:
        break;
    }
    osDelay(200);

}