#include "key_task.h"
#include "bsp_key.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "bsp_led.h"
#include "bsp_servo_pwm.h"
#include "CAN_receive.h"
#include "X_V2.h"
#include "zdt_motor.h"
#include "tower.h"

extern osMessageQueueId_t KeyQueueHandle;

void key_task(void *argument) {

    for (;;) {
        if (isKeyClicked() == 1) 
        {
            bsp_led_toggle(CORE_ONE); // 按键1被点击时点亮LED1
            //bsp_test_angle_set(500); // 测试用：设置夹爪舵机到闭合位置
            //bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 设置夹爪舵机状态
            //bsp_gripper_state_set(JIAZHUA_CLOSE); // 设置夹爪舵机状态为闭合
            osDelay(200); // 延时200ms，等待夹爪动作完成
            //Motor_35_Move(Motor_35_DOWN, 52700);
            //bsp_gimbal_angle_set(GIMBAL_INIT_ANGLE); // 设置云台角度为初始角度
            //X_V2_Read_Sys_Params(1, S_VEL);
            //X_V2_Vel_Control(1, 0, 255, 100.0f, false); // 测试用：让电机1以1000速度运行，限流500mA
            //can_send_chassis_speed(50, 100, 100, 100); // 测试用：发送底盘速度指令
            //X_V2_Read_Sys_Params(1, S_VEL);
            //chassis_set_control_target(0.0f, 0.0f, 0.1f);
            /*
            X_V2_En_Control(1, false, false); // 关闭电机
            osDelay(1); // 给 CAN 总线一点时间处理发送，避免过快调用导致的拥堵
            X_V2_En_Control(2, false, false); // 关闭电机
            osDelay(1); // 给 CAN 总线一点时间处理发送，避免过快调用导致的拥堵
            X_V2_En_Control(3, false, false); // 关闭电机
            osDelay(1); // 给 CAN 总线一点时间处理发送，避免过快调用导致的拥堵
            X_V2_En_Control(4, false, false); // 关闭电机
*/
            //zdt_motor_test(); // 测试用：控制ZDT电机运行
            // Update_OPS(0.3, 0.5, 1.0);
            //place_material(); // 测试用：执行放置物料动作
            //Motor_35_Move(Motor_35_DOWN, DIMIAN);
            //bsp_gimbal_angle_set(angle); // 夹爪张开一点,放置物料
            Motor_28_Move(QIAN, 3100);
            //osDelay(100);
            //bsp_gimbal_angle_set(GIMBAL_RED_ANGLE);
            
            
        }
        osDelay(10); // 延时10毫秒
  }
}