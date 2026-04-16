/**
 * @file bsp_can.h
 * @brief 
 * ----------------------------------------------------------------------------
 * @version 1.0.0.0
 * @author RM
 * @date 2018-12-26
 * @remark 官步初始代码
 * ----------------------------------------------------------------------------
 * @version 1.0.0.1
 * @author 周明杨
 * @date 2024-12-30
 * @remark 优化整体架构
 */

#ifndef BSP_CAN_H
#define BSP_CAN_H

#include <stdint.h>

extern void can_filter_init(void); // CAN 过滤器初始化
extern void BSP_CAN_Send_Msg(uint32_t std_id, uint8_t *data); // 发送接口

#endif
