#include "main_task.h"
#include "position_task.h"
#include "cmsis_os2.h"
#include "gm65.h"
#include "bsp_buzzer_pwm.h"
#include "CAN_receive.h"

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



/*-----------------------------------函数实现-----------------------------------*/

void main_task(void *argument)
{
    can_chassis_init(); // 使能底盘can步进电机驱动板
    osDelay(3000); // 任务启动延时，等待外设、速度环、位置环完成初始化
     // 蜂鸣器响一下，提示任务开始
    buzzer_rings(2000, 10, 500); // 2kHz频率，20音量，响500ms
    for (;;)
    {

        Chassis_Go_Pos(-0.15, 0.15, QIANMIAN, 200); // 出库
        Chassis_Go_Pos(-0.18, 0.77, QIANMIAN, 200); // 扫码
        
        Chassis_Go_Pos(-0.128, 1.467, QIANMIAN, 200); // 拿物料

        Chassis_Go_Pos(-0.217, 1.076, ZUOBIAN, 200); // 跑图：二维码和物料盘之间的路口

        Chassis_Go_Pos(-1.070, 1.070, ZUOBIAN, 200); // 跑图：地图中心
        Chassis_Go_Pos(-1.070, 1.959, ZUOBIAN, 200); // 加工区

        Chassis_Go_Pos(-1.897, 1.897, ZUOBIAN, 200); // 跑图：地图左上角
        Chassis_Go_Pos(-1.897, 1.897, HOUMIAN, 200); // 转向朝下

        Chassis_Go_Pos(-1.962, 1.076, HOUMIAN, 200); // 跑图：地图左下角
        Chassis_Go_Pos(-1.962, 1.076, ZUOBIAN, 200);

        Chassis_Go_Pos(-0.217, 1.076, ZUOBIAN, 200); // 跑图：二维码和物料盘之间的路口
        Chassis_Go_Pos(-0.217, 1.076, QIANMIAN, 200); // 转向朝前

        // 开始第二圈
        
        Chassis_Go_Pos(-0.128, 1.467, QIANMIAN, 200); // 拿物料

        Chassis_Go_Pos(-0.217, 1.076, ZUOBIAN, 200); // 跑图：二维码和物料盘之间的路口

        Chassis_Go_Pos(-1.070, 1.070, ZUOBIAN, 200); // 跑图：地图中心
        Chassis_Go_Pos(-1.070, 1.959, ZUOBIAN, 200); // 加工区

        Chassis_Go_Pos(-1.897, 1.897, ZUOBIAN, 200); // 跑图：地图左上角
        Chassis_Go_Pos(-1.897, 1.897, HOUMIAN, 200); // 转向朝下

        Chassis_Go_Pos(-1.962, 1.076, HOUMIAN, 200); // 跑图：地图左下角
        Chassis_Go_Pos(-1.962, 1.076, ZUOBIAN, 200);

        Chassis_Go_Pos(-0.217, 1.076, ZUOBIAN, 200); // 跑图：二维码和物料盘之间的路口
        Chassis_Go_Pos(-0.217, 1.076, QIANMIAN, 200);
        
        Chassis_Go_Pos(-0.15, 0.15, QIANMIAN, 200); // 入库
        Chassis_Go_Pos(-0.0, 0.0, QIANMIAN, 200); // 出库
        //can_send_chassis_speed(100, 100, 100, 100);

        while(1){osDelay(100);}
    }
}

static void get_task_code(void)
{
    
}

static void show_task_code(void)
{
    
}

static void color_ring_calibration(void)
{
    
}
