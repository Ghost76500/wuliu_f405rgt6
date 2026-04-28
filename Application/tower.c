#include "tower.h"
#include "bsp_servo_pwm.h"
#include "Emm_V5.h"

/*
 * @brief 识别姿态
 * @param void
 * @retval void
 */
void identify_posture(int radius, int height)
{
    // 夹爪打开
    bsp_gripper_state_set(JIAZHUA_DA_OPEN);
}
