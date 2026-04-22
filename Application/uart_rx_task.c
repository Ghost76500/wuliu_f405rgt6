#include "uart_rx_task.h"
#include "cmsis_os2.h"
#include "command.h"

extern osMessageQueueId_t global_uart_queueHandle;

void uart_rx_task(void *argument)
{
    osDelay(100); // 任务启动延时，等待系统稳定
    Global_UART_Msg_t recv_msg; // 统一的消息结构体
    
    for (;;)
    {
        /*
        //if (xQueueReceive(global_uart_queueHandle, &recv_msg, 100) == pdTRUE) 
        {
            // 根据包裹上的“发件人”标签，交给不同的部门去解包
            switch(recv_msg.uart_source)
            {
                case 1: // NANO 发来的
                    //NANO_Data_Unpack(((UART1_RX_TypeDef*)recv_msg.pData)->buffer);
                    break;
                case 2: // OPS 定位发来的
                    //POS_Data_Unpack(((UART2_RX_TypeDef*)recv_msg.pData)->buffer);
                    break;
                case 5: // RC 遥控器发来的
                    //RC_Data_Unpack(((UART5_RX_TypeDef*)recv_msg.pData)->buffer);
                    break;
            }
            
        }
        */
       osDelay(5);
    }
}