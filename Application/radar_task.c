#include "link_protocol.h"
#include "radar_client.h"
#include "cmsis_os2.h"

void radar_task(void *argument)
{
    radar_client_callbacks_t cb = {0};
    cb.obstacles = on_obstacles;   // 只复制数据到队列/快照
    cb.grid      = on_grid;
    cb.state     = on_state;
    cb.error     = on_error;

    RadarClient_Init(&cb, 1U);
    // 需要固定区域时再请求 RadarClient_RequestSelectZone(实际区域)

    for (;;) 
    {
        RadarClient_Poll();
        // 有界消费请求队列，提交 RequestScanOnce/GridScan/PlanPath 等
        osDelay(5);
    }
}