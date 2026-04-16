/**
 * @file bsp_delay.h
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

#ifndef BSP_DELAY_H
#define BSP_DELAY_H

#include "stdint.h"

extern void delay_ms(uint16_t ms);
extern void delay_us(uint16_t us);

#endif

