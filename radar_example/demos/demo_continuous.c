#include "demo_select.h"

#if RADAR_DEMO == RADAR_DEMO_CONTINUOUS

#include "demo_common.h"
#include "demo_console.h"

/*
 * 持续扫描流程：进入READY后发送一次START，随后持续接收障碍结果。
 * 调试串口输入stop可随时提交高优先级STOP，不要反复发送START。
 */

static bool submitted;
static bool stopped_by_user;
static bool point_cloud_configured;

/* ======================== 业务回调函数 ======================== */

/*
 * 持续扫描每产生一组完整障碍结果就调用一次。
 * 如需保存历史结果，应在这里复制data内容；不能长期保存data指针。
 */
static void on_obstacles(const radar_client_obstacle_result_t *data)
{
    DemoCommon_PrintObstacles(data);
}

/* ======================== 回调表初始化 ======================== */

/* 先注册公共回调，再注册持续扫描的障碍结果回调。 */
void Demo_InitCallbacks(radar_client_callbacks_t *callbacks)
{
    DemoCommon_InitCallbacks(callbacks);
    callbacks->obstacles = on_obstacles;
}

/* ======================== Demo初始化与轮询 ======================== */

void Demo_Init(void)
{
    submitted = false;
    stopped_by_user = false;
    point_cloud_configured = false;
    DemoConsole_Printf("DEMO continuous: print obstacle results until stop.\r\n");
}

void Demo_Poll(void)
{
    radar_client_state_t link;
    radar_client_request_result_t result;

    (void)RadarClient_GetState(&link);
    if (!link.connected) {
        submitted = false;
        stopped_by_user = false;
        point_cloud_configured = false;
        return;
    }
    if (DemoConsole_TakeStopRequest()) {
        stopped_by_user = true;
        DemoCommon_PrintRequest("stop", RadarClient_RequestStop());
        return;
    }
    /* submitted防止主循环每次轮询都重复发送START。 */
    if (stopped_by_user || submitted || !link.ready) return;
    if (!DemoCommon_PreparePointCloud(&point_cloud_configured)) return;
    result = RadarClient_RequestStartContinuous();
    DemoCommon_PrintRequest("start_continuous", result);
    if (result == RADAR_CLIENT_REQUEST_ACCEPTED) submitted = true;
}

#endif
