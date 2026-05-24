#include "main_task.h"
#include "position_task.h"
#include "cmsis_os2.h"
#include "gm65.h"
#include "bsp_buzzer_pwm.h"
#include "CAN_receive.h"
#include "bsp_servo_pwm.h"
#include "bsp_usart.h"
#include "bsp_oled.h"
#include "bsp_buzzer_pwm.h"
#include "visual_identity.h"
#include "Emm_V5.h"
#include "zdt_motor.h"
#include "tower.h"
#include <string.h>

/*-----------------------------------变量定义---------------------------------------*/

uint8_t GET_TASK_TIME_OUT = 0; // 获取任务代码超时标志

char task_code_str[10] = "123"; 

/*-----------------------------------内部函数声明-----------------------------------*/

/*
 * @brief 获取任务代码
 * @param void
 * @retval void
 */
static void get_task_code(void);

/*
 * @brief 显示任务代码
 * @param void
 * @retval void
 */
static void show_task_code(void);

void grab_turntable_test(void);

/*-----------------------------------函数实现-----------------------------------*/

void main_task(void *argument)
{
    can_chassis_init(); // 使能底盘can步进电机驱动板
    bsp_gimbal_angle_set(GIMBAL_INIT_ANGLE); // 云台复位
    //bsp_gimbal_angle_set(GIMBAL_GREEN_ANGLE); // 云台转到绿色位置，准备放下夹爪，测试用
    bsp_gripper_state_set(JIAZHUA_INIT); // 夹爪复位
    //bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪完全张开，测试用
    uart_init_it(&huart5);
    visual_idle();
    osDelay(3000); // 任务启动延时，等待外设、速度环、位置环完成初始化
    // 蜂鸣器响一下，提示任务开始
    // buzzer_rings(2000, 10, 500); // 2kHz频率，10音量，响500ms

    color_ring_calibration(50);
    while(1){osDelay(100);}

    //uint8_t tmp[7] = {50,49,51,0,51,50,49};
    //memcpy(color_task, tmp, sizeof(tmp));

    for (;;)
    {
        /*
        Chassis_Go_Pos(-0.15, 0.15, QIANMIAN, 200); // 出库
        
        Chassis_Go_Pos(-0.226, 0.77, QIANMIAN, 200); // 扫码位置
        get_task_code();
        show_task_code();

        Chassis_Go_Pos(-0.127, 1.475, QIANMIAN, 200); // 拿物料位置

        grab_turntable_A(color_task[0], 1);
        grab_turntable_A(color_task[1], 2);
        grab_turntable_A(color_task[2], 3);


        Chassis_Go_Pos(-0.217, 1.076, ZUOBIAN, 200); // 跑图：二维码和物料盘之间的路口

        Chassis_Go_Pos(-1.974, 1.069, ZUOBIAN, 200); // 跑图：加工区
        */
        Chassis_Go_Pos(-1.974, 1.069, HOUMIAN, 200); // 转向朝下,准备加工物料

        // 这里是加工物料的代码
        color_ring_calibration(50); // 校准色环
        /*
        grab_materials_car(1, 0);
        put_materials_ground(color_task[0],0);
        grab_materials_car(2, 0);
		put_materials_ground(color_task[1],0);
		grab_materials_car(3, 0);
		put_materials_ground(color_task[2],0);
		
        //while(1){osDelay(100);}

        grab_materials_ground(color_task[0],1);
		put_materials_car(1);
		grab_materials_ground(color_task[1],0);
		put_materials_car(2);
		grab_materials_ground(color_task[2],0); 
		put_materials_car(3);

        reset_posture();
*/
        while(1){osDelay(100);}

        Chassis_Go_Pos(-1.897, 1.897, HOUMIAN, 200); // 跑图：地图左上角
        Chassis_Go_Pos(-1.897, 1.897, ZUOBIAN, 200); // 转向朝左

        Chassis_Go_Pos(-1.055, 1.985, ZUOBIAN, 200); // 放置区

        // 这里是放物料的代码

        Chassis_Go_Pos(-0.163, 1.857, ZUOBIAN, 200); // 地图右上角
        Chassis_Go_Pos(-0.163, 1.857, QIANMIAN, 200); // 转向朝前

        //Chassis_Go_Pos(-0.15, 0.15, QIANMIAN, 200); // 准备入库
        //Chassis_Go_Pos(0.0, 0.0, QIANMIAN, 200); // 入库

        while(1){osDelay(100);}
        // 开始第二圈
        Chassis_Go_Pos(-0.122, 1.475, QIANMIAN, 200); // 拿物料位置

        grab_turntable_A(color_task[4], 1);
        grab_turntable_A(color_task[5], 2);
        grab_turntable_A(color_task[6], 3);

        Chassis_Go_Pos(-0.217, 1.076, ZUOBIAN, 200); // 跑图：二维码和物料盘之间的路口

        Chassis_Go_Pos(-1.974, 1.069, ZUOBIAN, 200); // 跑图：加工区
        Chassis_Go_Pos(-1.974, 1.069, HOUMIAN, 200); // 转向朝下，准备加工物料

        // 这里是加工物料的代码

        Chassis_Go_Pos(-1.897, 1.897, HOUMIAN, 200); // 跑图：地图左上角
        Chassis_Go_Pos(-1.897, 1.897, ZUOBIAN, 200); // 转向朝左

        Chassis_Go_Pos(-1.055, 1.985, ZUOBIAN, 200); // 放置区

        // 这里是放物料的代码

        Chassis_Go_Pos(-0.163, 1.857, ZUOBIAN, 200); // 地图右上角
        Chassis_Go_Pos(-0.163, 1.857, QIANMIAN, 200); // 转向朝前

        Chassis_Go_Pos(-0.15, 0.15, QIANMIAN, 200); // 准备入库
        Chassis_Go_Pos(0.0, 0.0, QIANMIAN, 200); // 入库
        

        while(1){osDelay(100);}
    }
}

static void get_task_code(void)
{
    while(color_task[0] + color_task[5] == 0)  
    {
		osDelay(50); 
	    GET_TASK_TIME_OUT++;
		if (GET_TASK_TIME_OUT >= 60)  // 超过3秒还没有接收到任务代码，前后动一下
		{
            Chassis_Go_Pos(-0.223, 0.47, QIANMIAN, 200);
            Chassis_Go_Pos(-0.223, 0.77, QIANMIAN, 200);
			GET_TASK_TIME_OUT = 0;
		}
	}
}

static void show_task_code(void)
{
    oled_show(color_task);
    osDelay(200);
}


void grab_turntable_test(void)
{
    osDelay(100);
    gimbal_turn(7, 2000); // 云台转到拿物料位置，7对应拿物料位置，放下夹爪
    bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪完全张开
    osDelay(100);

    // 降下夹爪抓物料
    Emm_V5_Pos_Control(1, Motor_35_DOWN, 3000, 245, 21200, false, false);
    osDelay(1200);
    bsp_gripper_state_set(JIAZHUA_CLOSE);
    Emm_V5_Pos_Control(1, Motor_35_UP, 3000, 245, 21200, false, false);
    osDelay(1200);
    gimbal_turn(1, 2000); // 放到一号位
    Emm_V5_Pos_Control(1, Motor_35_DOWN, 3000, 245, 9500, false, false);
    osDelay(800);
    bsp_gripper_state_set(JIAZHUA_XIAO_OPEN);
    Emm_V5_Pos_Control(1, Motor_35_UP, 3000, 245, 9500, false, false);
    osDelay(800);
    bsp_gripper_state_set(JIAZHUA_CLOSE);
    // 1松开夹爪2恢复位置
        
    
    visual_idle(); // 发送停止信息，告诉maixcam停止识别
    

    osDelay(1500);
}