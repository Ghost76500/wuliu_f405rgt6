#include "visual_identity.h"
#include "tower.h"
#include "usart.h" 
#include "bsp_led.h"


Maxicam_Info_t g_maxicam_info;

/**
 * @brief  【底层】向 Maxicam 发送 5 字节控制命令
 * @param  cmd 具体命令字节
 */
void MaixCam_SendCommand(uint8_t cmd)
{
    uint8_t tx_buffer[5];

    tx_buffer[0] = 0xAA; // 帧头 1
    tx_buffer[1] = 0x55; // 帧头 2
    tx_buffer[2] = cmd;  // 模式命令字
    
    // 校验和：帧头1 + 帧头2 + 命令字，取低8位
    tx_buffer[3] = (uint8_t)(tx_buffer[0] + tx_buffer[1] + tx_buffer[2]);
    tx_buffer[4] = 0x6B; // 帧尾

    HAL_UART_Transmit(&MAXICAM_UART_HANDLE, tx_buffer, 5, HAL_MAX_DELAY);
}

/**
 * @brief  【应用层】请求 Maxicam 识别物料
 * @param  num 颜色 1(49 红) / 2(50 绿) / 3(51 蓝)
 */
void identify_materials(uint8_t num)
{
    uint8_t cmd = 0;
    switch(num)
    {
        case '1': case 1: cmd = 0x0A; break;
        case '2': case 2: cmd = 0x0B; break;
        case '3': case 3: cmd = 0x0C; break;
        default: return; 
    }
    MaixCam_SendCommand(cmd);
}

void visual_idle(void)
{
    MaixCam_SendCommand(0xFF); 
    // 重置标志位
    visual_data_ready = 0;
}

/**
 * @brief  【应用层】请求 Maxicam 识别色环
 * @param  num 颜色 1(49 红) / 2(50 绿) / 3(51 蓝)
 */
void identify_color_rings(uint8_t num)
{
    uint8_t cmd = 0;
    switch(num)
    {
        case '1': case 1: cmd = 0x3A; break; // 红色环
        case '2': case 2: cmd = 0x3B; break; // 绿色环 (Python 端已扩展支持)
        case '3': case 3: cmd = 0x3C; break; // 蓝色环 (Python 端已扩展支持)
        default: return; 
    }
    MaixCam_SendCommand(cmd);
}

/**
 * @brief  【应用层】请求 Maxicam 识别有物料的色环
 * @param  num 颜色 1(49 红) / 2(50 绿) / 3(51 蓝)
 */
void identify_rings_with_materials(uint8_t num)
{
    uint8_t cmd = 0;
    switch(num)
    {
        case '1': case 1: cmd = 0x6A; break; // 有物料的红色环
        case '2': case 2: cmd = 0x6B; break; // 有物料的绿色环 (Python 端已扩展支持)
        case '3': case 3: cmd = 0x6C; break; // 有物料的蓝色环 (Python 端已扩展支持)
        default: return; 
    }
    MaixCam_SendCommand(cmd);
}

/**
 * @brief  【应用层】解析环形缓冲区提取出的 10 字节完美数据包
 * @param  maixcam_data 经过 Command_GetCommand 校验过的 10 字节数组
 * @note   只要数据能进这个函数，说明头尾和校验和肯定是正确的，直接拼坐标
 */
void maixcam_data_unpack(uint8_t maixcam_data[10])
{
    // 提取模式和颜色
    g_maxicam_info.mode  = maixcam_data[2];
    g_maxicam_info.color = maixcam_data[3];

    // 提取 Y 坐标 (注意协议是先发 Y 后发 X，初学者巨容易搞反)
    // 高 8 位左移 8 次，然后和低 8 位合并
    g_maxicam_info.y_pos = (maixcam_data[4] << 8) | maixcam_data[5];

    // 提取 X 坐标
    g_maxicam_info.x_pos = (maixcam_data[6] << 8) | maixcam_data[7];
}

/**
 * @brief  【应用层】计算摄像头中心与目标位置（物料、色环）中心的误差
 * @param  在uart_rx_task.c里调用，确保每次解析出新数据后都更新误差值
 * @note   none
 */
void calculate_error(void)
{
    // 这里假设摄像头中心坐标是 (320, 240)，你需要根据实际情况修改
    const uint16_t cam_center_x = 160;
    const uint16_t cam_center_y = 120;

    g_maxicam_info.error_x = g_maxicam_info.x_pos - cam_center_x;
    g_maxicam_info.error_y = g_maxicam_info.y_pos - cam_center_y;

    // 现在 error_x 和 error_y 就是目标相对于摄像头中心的偏移，可以用来控制底盘移动
    // 例如，如果 error_x 是正的，说明目标在右边，底盘应该向右移动；如果 error_y 是正的，说明目标在下方，底盘应该向后移动。
}

/**
 * @brief  【应用层】检查误差是否足够小
 * @param  在main_task.c里调用，等待物料移动到指定位置（误差足够小）后再执行抓取动作
 * @note   none
 */
uint8_t is_error_little(int8_t error_x, int8_t error_y, uint8_t count)
{
    static uint8_t calibration_count = 0;
    if (g_maxicam_info.error_x < error_x && g_maxicam_info.error_x > -error_x &&
        g_maxicam_info.error_y < error_y && g_maxicam_info.error_y > -error_y)
    {
        bsp_led_on(CORE_TWO); // 误差足够小，切换LED状态以示确认
        calibration_count++;
        if (calibration_count >= count) // 连续满足条件传入的次数才返回1，避免偶尔的误差过小导致误判
        {
            calibration_count = 0; // 重置计数器
            return 1;
        }
    }
    return 0;
    bsp_led_off(CORE_TWO); // 误差较大，保持LED关闭
}

/*
 * @brief  【应用层】检查视觉误差是否足够小
 * @param  在main_task.c里调用，等待物料移动到指定位置（误差足够小）后再执行抓取动作
 * @note   连续满足2次条件才能返回1
 */
uint8_t is_calibration_error_little(void)
{
    static uint8_t calibration_count = 0;
    const int16_t error_x = g_maxicam_info.error_x - target_x;
    const int16_t error_y = g_maxicam_info.error_y - target_y;
    
    if (error_x < 2 && error_x > -2 &&
        error_y < 2 && error_y > -2)
    {
        calibration_count++;
        
        if (calibration_count >= 2)
        {
            bsp_led_on(CORE_TWO);
            return 1;
        }
    }
    else
    {
        calibration_count = 0; // 误差较大，重置计数器
    }
    bsp_led_off(CORE_TWO);
    return 0;
}