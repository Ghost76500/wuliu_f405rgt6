#include "main_task.h"
#include "position_task.h"
#include "cmsis_os2.h"
#include "gm65.h"
#include "bsp_buzzer_pwm.h"

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
    osDelay(3000); // 任务启动延时，等待外设、速度环、位置环完成初始化
     // 蜂鸣器响一下，提示任务开始
    buzzer_rings(2000, 10, 500); // 2kHz频率，20音量，响500ms
    for (;;)
    {
        Chassis_Go_Pos(-0.15, 0.15, QIANMIAN, 200); // 向右前方移动0.15m，目标航向角-45度，达到后停顿200ms
        Chassis_Go_Pos(-0.15, 1.0, QIANMIAN, 200); // 向左前方移动0.15m，目标航向角+45度，达到后停顿200ms
        Chassis_Go_Pos(-0.15, 1.0, YOUBIAN, 200); // 向左后方移动0.15m，目标航向角+135度，达到后停顿200ms

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
