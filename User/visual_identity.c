#include "visual_identity.h"
#include "struct_typedef.h"

/*
 * @brief  识别物料
 * @param[in] num 物料编号 颜色1/2/3红绿蓝
 * @retval none
 */
void identify_materials(uint8_t num);

/*
 * @brief  解析MaxiCam数据包
 * @param[in]MaxiCam发送的16字节数据包
 * @retval none
 */
void maixcam_data_unpack(uint8_t maixcam_data[16]);


