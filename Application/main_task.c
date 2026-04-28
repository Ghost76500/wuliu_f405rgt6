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

/*-----------------------------------变量定义---------------------------------------*/

uint8_t GET_TASK_TIME_OUT = 0; // 获取任务代码超时标志

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

/*
 * @brief 颜色环校准
 * @param void
 * @retval void
*/
static void color_ring_calibration(void);

/*
 * @brief 物料静态识别抓取
 * @param 1/2/3 红绿蓝
 * @retval void
*/
static void grab_turntable_A(uint8_t color, uint8_t num);

void grab_turntable_test(void);

/*-----------------------------------函数实现-----------------------------------*/

void main_task(void *argument)
{
    can_chassis_init(); // 使能底盘can步进电机驱动板
    bsp_gimbal_angle_set(GIMBAL_INIT_ANGLE); // 云台复位
    bsp_gripper_state_set(JIAZHUA_INIT); // 夹爪复位
    uart_init_it(&huart5);
    osDelay(3000); // 任务启动延时，等待外设、速度环、位置环完成初始化
     // 蜂鸣器响一下，提示任务开始
    buzzer_rings(2000, 10, 500); // 2kHz频率，20音量，响500ms
    for (;;)
    {

        Chassis_Go_Pos(-0.15, 0.15, QIANMIAN, 200); // 出库
        
        Chassis_Go_Pos(-0.223, 0.77, QIANMIAN, 200); // 扫码位置
        get_task_code();
        show_task_code();

        Chassis_Go_Pos(-0.128, 1.467, QIANMIAN, 200); // 拿物料位置
        grab_turntable_test();
        while(1){osDelay(100);}
        gimbal_turn(7, 2000); // 转到物料位置，放下夹爪
        gimbal_turn(1, 2000);
        gimbal_turn(7, 2000);
        gimbal_turn(2, 2000);
        gimbal_turn(7, 2000);
        gimbal_turn(3, 2000);

        Chassis_Go_Pos(-0.217, 1.076, ZUOBIAN, 200); // 跑图：二维码和物料盘之间的路口

        Chassis_Go_Pos(-1.962, 1.076, ZUOBIAN, 200); // 跑图：加工区
        Chassis_Go_Pos(-1.962, 1.076, HOUMIAN, 200); // 转向朝下

        Chassis_Go_Pos(-1.897, 1.897, HOUMIAN, 200); // 跑图：地图左上角
        Chassis_Go_Pos(-1.897, 1.897, ZUOBIAN, 200); // 转向朝左

        Chassis_Go_Pos(-1.070, 1.959, ZUOBIAN, 200); // 放置区

        Chassis_Go_Pos(-0.163, 1.857, ZUOBIAN, 200); // 地图右上角
        Chassis_Go_Pos(-0.163, 1.857, QIANMIAN, 200); // 转向朝前

        //Chassis_Go_Pos(-0.15, 0.15, QIANMIAN, 200); // 准备入库
        //Chassis_Go_Pos(0.0, 0.0, QIANMIAN, 200); // 入库

        // 开始第二圈
        Chassis_Go_Pos(-0.128, 1.467, QIANMIAN, 200); // 拿物料位置

        gimbal_turn(GIMBAL_GREEN_ANGLE, 2000); // 转到蓝色位置，放下夹爪
        gimbal_turn(GIMBAL_ONE_ANGLE, 2000);
        gimbal_turn(GIMBAL_GREEN_ANGLE, 2000);
        gimbal_turn(GIMBAL_TWO_ANGLE, 2000);
        gimbal_turn(GIMBAL_GREEN_ANGLE, 2000);
        gimbal_turn(GIMBAL_THREE_ANGLE, 2000);

        Chassis_Go_Pos(-0.217, 1.076, ZUOBIAN, 200); // 跑图：二维码和物料盘之间的路口

        Chassis_Go_Pos(-1.962, 1.076, ZUOBIAN, 200); // 跑图：加工区
        Chassis_Go_Pos(-1.962, 1.076, HOUMIAN, 200); // 转向朝下

        Chassis_Go_Pos(-1.897, 1.897, HOUMIAN, 200); // 跑图：地图左上角
        Chassis_Go_Pos(-1.897, 1.897, ZUOBIAN, 200); // 转向朝左

        Chassis_Go_Pos(-1.070, 1.959, ZUOBIAN, 200); // 放置区

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
		if (GET_TASK_TIME_OUT >= 60)  //��ʱ����
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

static void color_ring_calibration(void)
{
    
}

static void grab_turntable_A(uint8_t color, uint8_t num)
{
    osDelay(100);
    gimbal_turn(7, 2000); // 云台转到拿物料位置，7对应拿物料位置，放下夹爪
    bsp_gripper_state_set(JIAZHUA_DA_OPEN); // 夹爪完全张开
    osDelay(100);

    identify_materials(num); // 发送开始信息，告诉maixcam开始识别

    // 等物料到达指定位置
    while (is_error_little() == 0)
    {
        osDelay(20);
    }
    
    buzzer_rings(2000, 20, 500); // 物料移动到对应位置，蜂鸣器响一声
    
    // 降下夹爪抓物料
    
    gimbal_turn(1, 2000); // 放到一号位
    
    // 1松开夹爪2恢复位置
    
    
    visual_idle(); // 发送停止信息，告诉maixcam停止识别
    bsp_gripper_state_set(JIAZHUA_XIAO_OPEN); // 张开夹爪放物料
    

    osDelay(1500);
    
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