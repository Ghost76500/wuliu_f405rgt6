#ifndef VISUAL_IDENTITY_H
#define VISUAL_IDENTITY_H

#include "struct_typedef.h"

extern void identify_materials(uint8_t num); // 识别物料 物料编号颜色1/2/3红绿蓝
extern void maixcam_data_unpack(uint8_t maixcam_data[16]); // MaxiCam发送的16字节数据包解析

#endif // VISUAL_IDENTITY_H