#include "CAN_receive.h"
#include "bsp_can.h"
#include "bsp_led.h"
#include "position_task.h"
#include "X_V2.h"
#include "struct_typedef.h"
#include "cmsis_os2.h"
#include "bsp_delay.h"


extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
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
volatile motor_measure_t can_y42_motor_measure[4] = {
    {.motor_id = CAN_Y42_M1_ID, .speed_rpm = 0.0f},
    {.motor_id = CAN_Y42_M2_ID, .speed_rpm = 0.0f},
    {.motor_id = CAN_Y42_M3_ID, .speed_rpm = 0.0f},
    {.motor_id = CAN_Y42_M4_ID, .speed_rpm = 0.0f},
};
// volatile uint8_t can_y42_new_data_flag = 0; // 标记底盘电机反馈是否有新数据，供控制任务做离线保护判定


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
    if (HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox) != HAL_OK)
    {
        // 这里可以加错误处理
        bsp_led_toggle(CORE_THREE);
    } 
}

// CAN 接收中断回调函数
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK)
        {
            return; // 接收出错，直接返回
        }
        //bsp_led_toggle(CORE_TWO);
        if (RxHeader.StdId == CAN_OPS_ID) 
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

    if (hcan->Instance == CAN2)
    {
        // 1. 从 FIFO0 读取 CAN2 接收到的报文
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK)
        {
            return;
        }

        uint8_t real_motor_id = (uint8_t)((RxHeader.ExtId >> 8) & 0xFF);

        // 2. 既然你确定只有扩展帧，我们直接拿 ExtId 开刀！
        // ExtId 就是扩展帧专用的“快递单号”，这里对应你的电机 ID
        switch(real_motor_id)
        {
            // 匹配你定义的四个底盘电机 ID
            case CAN_Y42_M1_ID:
            case CAN_Y42_M2_ID:
            case CAN_Y42_M3_ID:
            case CAN_Y42_M4_ID:
            {
                // 3. 校验数据格式：确保数据长度为 5，且帧头标志为 0x35
                if ((RxHeader.DLC == 5) && (RxData[0] == 0x35))
                {
                    const int8_t motor_dir[4] = {
                        CHASSIS_MOTOR1_DIR,
                        CHASSIS_MOTOR2_DIR,
                        CHASSIS_MOTOR3_DIR,
                        CHASSIS_MOTOR4_DIR
                    };
                    uint8_t motor_index = (uint8_t)(real_motor_id - CAN_Y42_M1_ID);
                    uint16_t vel_raw = ((uint16_t)RxData[2] << 8) | (uint16_t)RxData[3];
                    fp32 motor_vel_rpm = ((fp32)vel_raw) * 0.1f;

                    // RxData[1] 非0表示反转
                    if (RxData[1] != 0U)
                    {
                        motor_vel_rpm = -motor_vel_rpm;
                    }

                    // 与发送端保持同一套方向宏映射，确保控制与反馈符号一致
                    motor_vel_rpm = motor_vel_rpm * (fp32)motor_dir[motor_index];

                    can_y42_motor_measure[motor_index].motor_id = real_motor_id;
                    can_y42_motor_measure[motor_index].speed_rpm = motor_vel_rpm;
                    // can_y42_new_data_flag = 1;
                }
                break; // 处理完毕，跳出 switch
            }
            default:
                // 如果收到了这四个电机以外的扩展帧（虽然你说没有，但兜底逻辑要写），直接忽略
                break;
        }
    }

}

void can_send_chassis_speed(int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4)
{
    const int16_t motor_cmd[4] = {
        (int16_t)(motor1 * CHASSIS_MOTOR1_DIR),
        (int16_t)(motor2 * CHASSIS_MOTOR2_DIR),
        (int16_t)(motor3 * CHASSIS_MOTOR3_DIR),
        (int16_t)(motor4 * CHASSIS_MOTOR4_DIR)
    };

    for (uint8_t i = 0; i < 4; i++)
    {
        int16_t target_rpm = motor_cmd[i];

        if (target_rpm > CHASSIS_MOTOR_RPM_LIMIT)
        {
            target_rpm = CHASSIS_MOTOR_RPM_LIMIT;
        }
        else if (target_rpm < -CHASSIS_MOTOR_RPM_LIMIT)
        {
            target_rpm = -CHASSIS_MOTOR_RPM_LIMIT;
        }

        uint8_t dir = (target_rpm < 0) ? 1U : 0U;
        fp32 vel_rpm_abs = (target_rpm < 0) ? (fp32)(-target_rpm) : (fp32)target_rpm;

        X_V2_Vel_Control((uint8_t)(i + 1U), dir, 255U, vel_rpm_abs, true);
        osDelay(1); // 让后两个电机也能收到数据
        //delay_us(500); // 给 CAN 总线一点时间处理发送，避免过快调用导致的拥堵
    }
    // 触发多机同步运动
    X_V2_Synchronous_motion(0);
    
}


/**
	* @brief   CAN发送多个字节
	* @param   张大头电机
	* @retval  无
	*/
void can_SendCmd(__IO uint8_t *cmd, uint8_t len)
{
    static uint32_t TxMailbox;
    CAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[8] = {0};
    __IO uint8_t i = 0, j = 0, k = 0, l = 0, packNum = 0;

    if ((cmd == NULL) || (len < 2U))
    {
        return;
    }

    j = (uint8_t)(len - 2U);

    while(i < j)
    {
        k = (uint8_t)(j - i);

        tx_header.StdId = 0x00;
        tx_header.ExtId = ((uint32_t)cmd[0] << 8) | (uint32_t)packNum;
        tx_header.IDE = CAN_ID_EXT;
        tx_header.RTR = CAN_RTR_DATA;
        tx_header.TransmitGlobalTime = DISABLE;
        tx_data[0] = cmd[1];

        if(k < 8U)
        {
            for(l = 0; l < k; l++, i++)
            {
                tx_data[l + 1] = cmd[i + 2];
            }
            tx_header.DLC = (uint32_t)(k + 1U);
        }
        else
        {
            for(l = 0; l < 7U; l++, i++)
            {
                tx_data[l + 1] = cmd[i + 2];
            }
            tx_header.DLC = 8;
        }

        while(HAL_CAN_AddTxMessage(&hcan2, &tx_header, tx_data, &TxMailbox) != HAL_OK);

        ++packNum;
    }
}

void can_chassis_init(void)
{
    X_V2_En_Control(1, true, false);
    osDelay(1);
    X_V2_En_Control(2, true, false);
    osDelay(1);
    X_V2_En_Control(3, true, false);
    osDelay(1);
    X_V2_En_Control(4, true, false);
}

// 失能电机，调整位置
void can_chassis_ready_mode(void)
{
    X_V2_En_Control(1, false, false);
    osDelay(1);
    X_V2_En_Control(2, false, false);
    osDelay(1);
    X_V2_En_Control(3, false, false);
    osDelay(1);
    X_V2_En_Control(4, false, false);
}

/**
 * @brief  打包底盘位姿数据到CAN发送数据帧中
 * @param  x:    X轴坐标 (单位: m)
 * @param  y:    Y轴坐标 (单位: m)
 * @param  yaw:  偏航角 (单位: rad)
 * @param  data: 输出的数据缓存指针 (至少6字节)
 */
void Update_OPS(float x, float y, float yaw)
{
    uint8_t data[8] = {0}; // CAN数据帧最多8字节，我们只用前6字节来传位姿
    // 将浮点数乘以1000转为 int16_t (与解包时的 *0.001f 互逆)
    int16_t x_raw = (int16_t)(x * 1000.0f);
    int16_t y_raw = (int16_t)(y * 1000.0f);
    int16_t yaw_raw = (int16_t)(yaw * 1000.0f);

    // 拼装高低字节：高字节在前 (data[0], data[2], data[4])，低字节在后 (data[1], data[3], data[5])
    data[0] = (uint8_t)(x_raw >> 8);
    data[1] = (uint8_t)(x_raw & 0xFF);
    
    data[2] = (uint8_t)(y_raw >> 8);
    data[3] = (uint8_t)(y_raw & 0xFF);
    
    data[4] = (uint8_t)(yaw_raw >> 8);
    data[5] = (uint8_t)(yaw_raw & 0xFF); 

    // 发送到 CAN1，ID为0x200，供里程计使用
    BSP_CAN_Send_Msg(CAN_OPS_ID, data);
}