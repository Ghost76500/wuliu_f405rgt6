#ifndef VISUAL_IDENTITY_H
#define VISUAL_IDENTITY_H

#include "struct_typedef.h"
#include "main.h" // 确保包含了 HAL 库的头文件

#define MAXICAM_UART_HANDLE huart1

// maixcam解析好的数据
typedef struct {
    uint8_t  mode;        // 模式 (物料0x0A, 色环0x3A等)
    uint8_t  color;       // 颜色 (红0x0A, 绿0x0B, 蓝0x0C等)
    int16_t x_pos;       // 实际X坐标
    int16_t y_pos;       // 实际Y坐标
    int16_t error_x;
    int16_t error_y;
} Maxicam_Info_t;

// 声明外部可以使用的全局变量
extern Maxicam_Info_t g_maxicam_info;

extern uint8_t visual_data_ready; // 视觉数据就绪标志，供其他任务查询

extern void maixcam_data_unpack(uint8_t maixcam_data[10]); // 解析 Maxicam 数据包

extern void MaixCam_SendCommand(uint8_t cmd); // 发送命令给 Maxicam
extern void identify_materials(uint8_t num); // 请求 Maxicam 识别物料
extern void identify_color_rings(uint8_t num); // 请求 Maxicam 识别色环
extern void identify_rings_with_materials(uint8_t num); // 请求 Maxicam 识别有物料的色环
extern void visual_idle(void); // 视觉空闲时的处理函数，可以放一些默认动作或者状态重置

extern void calculate_error(void); // 计算误差值，更新全局变量
extern uint8_t is_error_little(int8_t error_x, int8_t error_y, uint8_t count); // 检查误差是否足够小
extern uint8_t is_calibration_error_little(void); // 检查校准误差是否足够小

#endif // VISUAL_IDENTITY_H