#include "zdt_motor.h"
#include "Emm_V5.h"

/*-----------------------------------宏定义-----------------------------------*/



// 车上物料盘的脉冲为1050 

// Emm_V5 方向定义：0为CW，其余值为CCW，cw为向下，ccw为向上（根据实际安装调整）
#define EMM_DIR_CW 0u
#define EMM_DIR_CCW 1u

void zdt_motor_test(void)
{
    // 位置模式控制：电机1以3000速度加速，245加速度，10000脉冲目标位置，raF=false不返回原点，snF=false不启动
    Emm_V5_Pos_Control(2, 1, 200, 245, 1050, false, false); // 位置模式控制
}

/*
 * @brief 丝杆电机控制函数
 * @param dir 方向，1为向上，0为向下
 * @param void
 * @retval void
 */
void Motor_35_Move(uint8_t dir, fp32 clk)
{
    Emm_V5_Pos_Control(Motor_35_ADDR, dir, 3000, 245, clk, false, false);
}

/*
 * @brief 伸缩电机控制函数
 * @param dir 方向，1为向前，0为向后
 * @param void
 * @retval void
 */
void Motor_28_Move(uint8_t dir, fp32 clk)
{
    Emm_V5_Pos_Control(Motor_28_ADDR, dir, 100, 245, clk, false, false);
}