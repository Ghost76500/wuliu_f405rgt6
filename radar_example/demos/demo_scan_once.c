#include "demo_select.h"

#if RADAR_DEMO == RADAR_DEMO_SCAN_ONCE

#include "demo_common.h"
#include "demo_console.h"

/*
 * 单次完整扫描流程：等待连接和区域确认，只提交一次14圈SCAN_LOCKED。
 * 最终从obstacles回调取得障碍坐标、方格编号和置信度。
 */

static bool submitted;
static bool completed;
static bool aborted;
static bool point_cloud_configured;

/* ======================== 业务回调函数 ======================== */

/*
 * 同一次扫描的GRID和RESULT已由RadarClient按scan_sequence组合后调用。
 * data->obstacles[0..obstacle_count-1]就是本次障碍坐标和方格结果。
 * 如需在回调结束后继续使用，应复制整个结构体或所需字段。
 */
static void on_obstacles(const radar_client_obstacle_result_t *data)
{
    DemoCommon_PrintObstacles(data);
    /* 收到最终障碍结果后结束本次Demo，主循环不会重复发起扫描。 */
    completed = true;
}

/* ======================== 回调表初始化 ======================== */

/* 先注册公共回调，再把完整障碍结果交给本Demo的on_obstacles。 */
void Demo_InitCallbacks(radar_client_callbacks_t *callbacks)
{
    DemoCommon_InitCallbacks(callbacks);
    callbacks->obstacles = on_obstacles;
}

/* ======================== Demo初始化与轮询 ======================== */

void Demo_Init(void)
{
    submitted = false;
    completed = false;
    aborted = false;
    point_cloud_configured = false;
    DemoConsole_Printf("DEMO scan_once: one 14-circle obstacle scan.\r\n");
}

void Demo_Poll(void)
{
    radar_client_state_t link;
    radar_client_request_result_t result;

    (void)RadarClient_GetState(&link);
    if (!link.connected) {
        submitted = false;
        completed = false;
        aborted = false;
        point_cloud_configured = false;
        return;
    }
    if (DemoConsole_TakeStopRequest()) {
        aborted = true;
        DemoCommon_PrintRequest("stop", RadarClient_RequestStop());
        return;
    }
    /* ready=1表示HELLO、工作区域和保存状态都已经确认。 */
    if (aborted || completed || submitted || !link.ready) return;
    if (!DemoCommon_PreparePointCloud(&point_cloud_configured)) return;
    result = RadarClient_RequestScanOnce();
    DemoCommon_PrintRequest("scan_once", result);
    if (result == RADAR_CLIENT_REQUEST_ACCEPTED) submitted = true;
}

#endif
