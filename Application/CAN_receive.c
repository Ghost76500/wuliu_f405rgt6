#include "CAN_receive.h"
#include "bsp_can.h"
#include "bsp_led.h"
#include "position_task.h"


extern CAN_HandleTypeDef hcan;
// 发送配置结构体（发送时的快递单）
CAN_TxHeaderTypeDef TxHeader;
// 接收配置结构体（接收到的快递单详情）
CAN_RxHeaderTypeDef RxHeader;

// 发送和接收数据缓存
uint8_t TxData[8];
uint8_t RxData[8];

// 发送邮箱句柄（硬件用来存待发送消息的地方，系统会自动分配）
uint32_t TxMailbox;

// 用于存储从 CAN 接收到的陀螺仪 Yaw 角（单位：rad）
// 该变量在 CAN 中断里写、在控制任务里读：使用 volatile 防止编译器缓存。
volatile float can_gyro_yaw_rad = 0.0f;
volatile float can_distence_x_m = 0.0f;
volatile float can_distence_y_m = 0.0f;
volatile uint8_t can_odom_new_data_flag = 0;

// 协议约定：yaw_raw 取值大约在 [-31415, +31415]，表示 [-3.1415, +3.1415] rad 的 1e4 放大。
// 如你的上位机/陀螺仪发送端使用不同缩放，请同步修改该比例。
#define CAN_GYRO_YAW_SCALE (0.0001f)
#define CAN_DISTENCE_SCALE (0.001f)


/**
 * @brief  对外提供的发送接口
 * @param  std_id: 目标设备的ID (比如 0x201)
 * @param  data:   要发送的数据指针 (8字节)
 */
void BSP_CAN_Send_Msg(uint32_t std_id, uint8_t *data)
{
    // 1. 现场配置 Header (因为每次发送对象可能不同)
    TxHeader.StdId = std_id;        // 填入 ID
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;

    // 2. 拷贝数据到发送缓存 TxData
    // (为了安全，不要直接用外部指针，而是复制到自己的 TxData 数组里)
    for(int i=0; i<8; i++)
    {
        TxData[i] = data[i];
    }

    // 3. 发送
    // 注意：这里要用 hcan (看你第18行extern的是 hcan 还是 hcan1，要保持一致)
    if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK)
    {
        // 这里可以加错误处理
        bsp_led_toggle(CORE_THREE);
    } 
}

// CAN 接收中断回调函数
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{

    // 从 FIFO0 读取数据
    // 参数：CAN句柄, FIFO号, 接收头指针(存ID等信息), 数据缓存指针
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData);
    
    //bsp_led_toggle(CORE_TWO);
    // --- 在这里处理你的业务逻辑 ---
      
    // 比如：判断 ID 是不是 0x101
    if (RxHeader.StdId == 0x200) 
    {
        uint8_t cmd[8] = {0};
        for (int i=0; i<6; i++)
        {
            cmd[i] = RxData[i];
        }
        
        int16_t yaw_raw = (int16_t)((cmd[0] << 8) | cmd[1]);
        can_gyro_yaw_rad = ((float)yaw_raw) * CAN_GYRO_YAW_SCALE;
        
        can_distence_y_m = (float)((int16_t)((RxData[2] << 8) | RxData[3])) * CAN_DISTENCE_SCALE; // mm to m  
        can_distence_x_m = (float)((int16_t)((RxData[4] << 8) | RxData[5])) * CAN_DISTENCE_SCALE; // mm to m

        // 标记里程计已收到新数据，供位置环做离线保护判定
        can_odom_new_data_flag = 1;
        
        
    }
    
}