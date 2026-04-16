#include "visual_task.h"
#include "UART_receive.h"
#include "stdbool.h"
#include "math.h"

/*-----------------------------------宏定义-----------------------------------*/

#define TARGET_PIXEL_U 160.0f
#define TARGET_PIXEL_V 120.0f

// 像素到物理距离的转换系数 (单位：米/像素)
// 这个值需要实测：比如画面差 10 像素，小车实际差了 0.02 米，那就是 0.002
#define K_PIX_TO_METER 0.002f

/*-----------------------------------变量声明-----------------------------------*/

uint8_t readBuffer[128]; // 接收缓冲区



/*-----------------------------------函数实现-----------------------------------*/

void visual_align_update(float *out_target_x, float *out_target_y, float current_x, float current_y)
{
    // 1. 获取 MaixCam 数据
    float maix_u = get_maixcam_u();
    float maix_v = get_maixcam_v();
    bool is_found = get_maixcam_isfound();

    if (!is_found) {
        // 【关键保护】如果没看到目标，目标点直接等于当前点，复用位置环实现“原地刹车锁死”
        *out_target_x = current_x;
        *out_target_y = current_y;
        return;
    }

    // 2. 计算像素误差
    float err_u = TARGET_PIXEL_U - maix_u;
    float err_v = TARGET_PIXEL_V - maix_v;

    // 3. 像素误差死区判断 (防抖)
    // 如果偏差小于 3 个像素，认为已经完全对准，不再更新目标点
    if (fabs(err_u) < 3.0f && fabs(err_v) < 3.0f) {
        *out_target_x = current_x;
        *out_target_y = current_y;
        return;
    }

    // 4. 计算物理偏移量 (Delta X, Delta Y)
    // 注意：这里的对应关系取决于摄像头安装方向。
    // 假设：画面左右(err_u)对应底盘左右(Y轴)，画面上下(err_v)对应底盘前后(X轴)
    float delta_x_meter = err_v * K_PIX_TO_METER;
    float delta_y_meter = err_u * K_PIX_TO_METER;

    // 5. 【核心】生成新的动态目标点！
    // 目标点 = 当前真实位置 + 视觉偏移量
    *out_target_x = current_x + delta_x_meter;
    *out_target_y = current_y + delta_y_meter;
}
