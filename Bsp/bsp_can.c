/**
 * @file bsp_can.c
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

#include "can.h"

void can_filter_init(void)
{
    CAN_FilterTypeDef can1_filter_st = {0};
    CAN_FilterTypeDef can2_filter_st = {0};

    // CAN1: 仅接收标准数据帧 ID 0x200
    can1_filter_st.FilterActivation = ENABLE;
    can1_filter_st.FilterMode = CAN_FILTERMODE_IDMASK;
    can1_filter_st.FilterScale = CAN_FILTERSCALE_32BIT;
    can1_filter_st.FilterBank = 0;
    can1_filter_st.SlaveStartFilterBank = 14;
    can1_filter_st.FilterFIFOAssignment = CAN_RX_FIFO0;
    can1_filter_st.FilterIdHigh = (uint16_t)(0x200U << 5);
    can1_filter_st.FilterIdLow = 0x0000U;
    can1_filter_st.FilterMaskIdHigh = (uint16_t)(0x7FFU << 5);
    can1_filter_st.FilterMaskIdLow = 0x0006U;
    HAL_CAN_ConfigFilter(&hcan1, &can1_filter_st);
    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

    // CAN2: 保持全接收
    can2_filter_st.FilterActivation = ENABLE;
    can2_filter_st.FilterMode = CAN_FILTERMODE_IDMASK;
    can2_filter_st.FilterScale = CAN_FILTERSCALE_32BIT;
    can2_filter_st.FilterIdHigh = 0x0000;
    can2_filter_st.FilterIdLow = 0x0000;
    can2_filter_st.FilterMaskIdHigh = 0x0000;
    can2_filter_st.FilterMaskIdLow = 0x0000;
    can2_filter_st.FilterFIFOAssignment = CAN_RX_FIFO0;
    can2_filter_st.SlaveStartFilterBank = 14;
    can2_filter_st.FilterBank = 14;
    HAL_CAN_ConfigFilter(&hcan2, &can2_filter_st);
    HAL_CAN_Start(&hcan2);
    HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);
}
