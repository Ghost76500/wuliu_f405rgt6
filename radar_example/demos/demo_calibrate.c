#include "demo_select.h"

#if RADAR_DEMO == RADAR_DEMO_CALIBRATE

#include "demo_common.h"
#include "demo_console.h"

/*
 * 两点校准流程：对demo_select.h中明确配置的当前区域校准一次。
 * 固件1.34会单独保存该区域位姿，不会修改另一个区域。
 */

static bool submitted;
static bool completed;
static bool aborted;

/* ======================== 业务回调函数 ======================== */

/*
 * 收到最终校准结果时调用。
 * data->pose是当前区域保存后的实际位姿，偏移、残差和柱距误差也在data中。
 * 雷达客户端还会等待匹配区域的Flash保存状态，不能只把ACK当作成功。
 */
static void on_calibration(const radar_client_calibration_t *data)
{
    /* 客户端已确认对应区域保存状态后才会结束校准事务。 */
    DemoCommon_PrintCalibration(data);
    completed = true;
}

/* ======================== 回调表初始化 ======================== */

/* 先注册公共回调，再把校准最终结果交给本Demo。 */
void Demo_InitCallbacks(radar_client_callbacks_t *callbacks)
{
    DemoCommon_InitCallbacks(callbacks);
    callbacks->calibration = on_calibration;
}

/* ======================== Demo初始化与轮询 ======================== */

void Demo_Init(void)
{
    submitted = false;
    completed = false;
    aborted = false;
    DemoConsole_Printf("DEMO calibrate: one 14-circle calibration for zone %u.\r\n",
                       RADAR_DEMO_ZONE);
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
        return;
    }
    if (DemoConsole_TakeStopRequest()) {
        aborted = true;
        DemoCommon_PrintRequest("stop", RadarClient_RequestStop());
        return;
    }
    if (aborted || completed || submitted || !link.ready) return;
    result = RadarClient_RequestCalibrate(RADAR_DEMO_ZONE);
    DemoCommon_PrintRequest("calibrate", result);
    if (result == RADAR_CLIENT_REQUEST_ACCEPTED) submitted = true;
}

#endif
