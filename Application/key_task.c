#include "key_task.h"
#include "bsp_key.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "bsp_led.h"
#include "bsp_servo_pwm.h"
#include "CAN_receive.h"
#include "X_V2.h"

extern osMessageQueueId_t KeyQueueHandle;

void key_task(void *argument) {

    for (;;) {
        if (isKeyClicked() == 1) 
        {
            bsp_led_toggle(CORE_ONE); // 按键1被点击时点亮LED1
            bsp_test_angle_set(500); // 测试用：设置夹爪舵机到闭合位置
            bsp_gimbal_angle_set(GIMBAL_INIT_ANGLE); // 设置云台角度为初始角度
            //X_V2_Read_Sys_Params(1, S_VEL);
            //X_V2_Vel_Control(1, 0, 255, 100.0f, false); // 测试用：让电机1以1000速度运行，限流500mA
            //can_send_chassis_speed(50, 100, 100, 100); // 测试用：发送底盘速度指令
            //X_V2_Read_Sys_Params(1, S_VEL);
            //chassis_set_control_target(0.0f, 0.0f, 0.1f);
            X_V2_En_Control(1, false, false); // 关闭电机
            osDelay(1); // 给 CAN 总线一点时间处理发送，避免过快调用导致的拥堵
            X_V2_En_Control(2, false, false); // 关闭电机
            osDelay(1); // 给 CAN 总线一点时间处理发送，避免过快调用导致的拥堵
            X_V2_En_Control(3, false, false); // 关闭电机
            osDelay(1); // 给 CAN 总线一点时间处理发送，避免过快调用导致的拥堵
            X_V2_En_Control(4, false, false); // 关闭电机
        
        }
        osDelay(10); // 延时10毫秒
  }
}