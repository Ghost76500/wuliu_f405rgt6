#include "visual_identity.h"
#include "usart.h" 

#define MAXICAM_UART_HANDLE huart1 

// 实例化我们在 .h 里声明的结构体
Maxicam_Info_t g_maxicam_info;

/**
 * @brief  【底层】向 Maxicam 发送 5 字节控制命令
 * @param  cmd 具体命令字节 (如 0x0A, 0x0B 等)
 */
void MaxiCam_SendCommand(uint8_t cmd)
{
    // 【修改点】定义 5 字节的发送缓冲区
    uint8_t tx_buffer[5];

    tx_buffer[0] = 0xAA; // 帧头 1
    tx_buffer[1] = 0x55; // 帧头 2
    tx_buffer[2] = cmd;  // 模式命令字
    
    // 校验和：DATA[0] + DATA[1]，即 0xAA + 0x55 = 0xFF
    tx_buffer[3] = 0xFF; 
    
    tx_buffer[4] = 0x6B; // 帧尾

    // 参数3：发送长度 5 字节
    HAL_UART_Transmit(&MAXICAM_UART_HANDLE, tx_buffer, 5, 10);
}

/**
 * @brief  【应用层】请求 Maxicam 识别物料
 * @param  num 颜色 1(红) / 2(绿) / 3(蓝)
 */
void identify_materials(uint8_t num)
{
    uint8_t cmd = 0;
    switch(num)
    {
        case 1: cmd = 0x0A; break;
        case 2: cmd = 0x0B; break;
        case 3: cmd = 0x0C; break;
        default: return; 
    }
    MaxiCam_SendCommand(cmd);
}

/**
 * @brief  【应用层】解析环形缓冲区提取出的 10 字节完美数据包
 * @param  maixcam_data 经过 Command_GetCommand 校验过的 10 字节数组
 * @note   只要数据能进这个函数，说明头尾和校验和肯定是正确的，直接拼坐标即可！
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