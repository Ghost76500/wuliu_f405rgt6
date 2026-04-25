#ifndef VISUAL_IDENTITY_H
#define VISUAL_IDENTITY_H

#include "struct_typedef.h"
#include "main.h" // 确保包含了 HAL 库的头文件

// --- 这里我为你补充一个结构体，专门存解析好的数据，方便其他C文件调用 ---
typedef struct {
    uint8_t  mode;        // 模式 (物料0x0A, 色环0x3A等)
    uint8_t  color;       // 颜色 (红0x0A, 绿0x0B, 蓝0x0C等)
    uint16_t x_pos;       // 实际X坐标
    uint16_t y_pos;       // 实际Y坐标
} Maxicam_Info_t;

// 声明外部可以使用的全局变量
extern Maxicam_Info_t g_maxicam_info;

extern void MaxiCam_SendCommand(uint8_t cmd);
extern void identify_materials(uint8_t num);

// 【修改点】长度改为10
extern void maixcam_data_unpack(uint8_t maixcam_data[10]); 

#endif // VISUAL_IDENTITY_H