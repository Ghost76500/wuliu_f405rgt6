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
    /* 定义过滤器配置结构体 */
    CAN_FilterTypeDef can_filter_st;
    /* 1. 激活过滤器 */
    can_filter_st.FilterActivation = ENABLE; // 开启这个过滤器，让它开始工作
    
    /* 2. 设置过滤器模式：掩码模式 (Mask Mode) */
    // CAN过滤器有两种模式：列表模式(List)和掩码模式(Mask)。
    // 掩码模式就像“通配符”，更灵活。
    can_filter_st.FilterMode = CAN_FILTERMODE_IDMASK;
    
    /* 3. 设置过滤器位宽：32位 */
    // CAN ID有标准帧(11位)和扩展帧(29位)。
    // 32位模式可以覆盖所有情况（包括扩展帧ID、IDE标志位、RTR标志位）。
    can_filter_st.FilterScale = CAN_FILTERSCALE_32BIT;
    
    /* 4. 配置“验证码”（ID）和“屏蔽码”（Mask） */
    // 核心逻辑：接收到的ID & Mask == FilterID & Mask
    // 当我们将 Mask 全部设为 0 时，意味着“我不关心任何一位是否匹配”。
    // 结果：任何 ID 的报文都能通过验证。
    can_filter_st.FilterIdHigh = 0x0000;      // 验证码高16位
    can_filter_st.FilterIdLow = 0x0000;       // 验证码低16位
    can_filter_st.FilterMaskIdHigh = 0x0000;  // 屏蔽码高16位 (0表示不关心)
    can_filter_st.FilterMaskIdLow = 0x0000;   // 屏蔽码低16位 (0表示不关心)
    
    /* 5. 设置过滤器组编号 */
    // STM32F1/F4 通常有 0~27 共28个过滤器组。
    // 这里我们将配置应用到“第0号”过滤器组。
    can_filter_st.FilterBank = 0;
    
    /* 6. 设置通过后的去向：FIFO0 */
    // CAN硬件有两个接收缓冲区：FIFO0 和 FIFO1。
    // 这里指定通过过滤器的报文放入 FIFO0。
    can_filter_st.FilterFIFOAssignment = CAN_RX_FIFO0;
    
    /* [重要细节] 7. 配置主从过滤器分割线 */
    // 这里的变量在第一次调用时建议显式赋值，否则可能是随机值。
    // 它的作用是告诉硬件：从哪个编号开始是分配给 CAN2 的。
    // 设置为 14，表示 0~13 给 CAN1 用，14~27 给 CAN2 用。
    //can_filter_st.SlaveStartFilterBank = 14; // (注：原代码是在下面赋值的，建议提到上面更安全)

    /* 8. 将配置写入硬件寄存器 (针对 hcan1) */
    HAL_CAN_ConfigFilter(&hcan1, &can_filter_st);
    
    /* 9. 启动 CAN1 模块 */
    // 从初始化模式切换到正常工作模式，开始参与总线通信。
    HAL_CAN_Start(&hcan1);
    
    /* 10. 开启接收中断 */
    // 当 FIFO0 里有新消息时，触发中断 (CAN1_RX0_IRQHandler)
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
}
