#include "demo_common.h"

#include "demo_console.h"
#include "demo_select.h"

#include <string.h>

static radar_client_state_t last_state;
static bool last_state_valid;

/* ======================== 公共回调函数 ======================== */

/*
 * 收到HELLO事件时调用。
 * 可从data读取固件版本、协议版本、当前工作区域、位姿和运行状态。
 * data只在本次回调内有效，需要长期使用时必须复制。
 */
static void on_hello(const radar_client_hello_t *data)
{
    DemoConsole_Printf("HELLO firmware=%u.%u protocol=%u zone=%u running=%u max_points=%u\r\n",
                       data->firmware_major, data->firmware_minor,
                       data->protocol, data->pose.zone,
                       data->running ? 1U : 0U, data->max_scan_points);
}

/*
 * 客户端连接状态、事务状态或14圈进度发生变化时调用。
 * 为避免调试串口刷屏，这里只打印与上一次不同的状态快照。
 */
static void on_state(const radar_client_state_t *data)
{
    if (!last_state_valid || memcmp(&last_state, data, sizeof(*data)) != 0) {
        DemoConsole_Printf("STATE connected=%u ready=%u busy=%u link=%u operation=%u zone=%u status=%u mode=%u running=%u frames=%u/14\r\n",
                           data->connected ? 1U : 0U,
                           data->ready ? 1U : 0U,
                           data->busy ? 1U : 0U,
                           (unsigned int)data->link_state,
                           (unsigned int)data->operation,
                           data->selected_zone, data->status, data->mode,
                           data->running ? 1U : 0U, data->frame_count);
        last_state = *data;
        last_state_valid = true;
    }
}

/*
 * 收到雷达Flash保存状态时调用。
 * 路径Demo会覆盖默认persisted回调，并在自己的回调中再次调用本函数，
 * 因而既保留统一打印，也能读取grid_result_valid作业务判断。
 */
void DemoCommon_PrintPersisted(const radar_client_persisted_state_t *data)
{
    DemoConsole_Printf("PERSISTED version=%u record=%lu modified_zone=%u grid_valid=%u grid_count=%u\r\n",
                       data->storage_version,
                       (unsigned long)data->record_sequence,
                       data->source_zone,
                       data->grid_result_valid ? 1U : 0U,
                       data->grid_count);
    DemoConsole_Printf("  zone1=(%d,%d,%u/64deg) zone2=(%d,%d,%u/64deg)\r\n",
                       data->zone1_pose.x_mm, data->zone1_pose.y_mm,
                       data->zone1_pose.heading_q6,
                       data->zone2_pose.x_mm, data->zone2_pose.y_mm,
                       data->zone2_pose.heading_q6);
}

/*
 * 协议、UART、超时或雷达拒绝命令时调用。
 * operation指出出错时的业务，code/detail提供具体错误信息。
 */
static void on_error(const radar_client_error_t *data)
{
    DemoConsole_Printf("ERROR kind=%u operation=%u code=%u detail=%u\r\n",
                       (unsigned int)data->kind,
                       (unsigned int)data->operation,
                       data->code, data->detail);
}

/* ======================== 回调表初始化 ======================== */

/*
 * 先清空整个回调表，再注册所有Demo共用的HELLO、状态、Flash和错误回调。
 * 各Demo应先调用本函数，然后只覆盖自己关心的业务回调。
 */
void DemoCommon_InitCallbacks(radar_client_callbacks_t *target)
{
    if (target == NULL) return;
    memset(target, 0, sizeof(*target));
    target->hello = on_hello;
    target->state = on_state;
    target->persisted = DemoCommon_PrintPersisted;
    target->error = on_error;
#if RADAR_DEMO_POINT_CLOUD_ENABLE
    target->point_cloud = DemoCommon_PrintPointCloud;
    target->stats = DemoCommon_PrintStats;
#endif
    last_state_valid = false;
}

/* ======================== 普通辅助函数 ======================== */

/* 打印Request函数的立即返回值；ACCEPTED不表示业务已经完成。 */
void DemoCommon_PrintRequest(const char *name,
                             radar_client_request_result_t result)
{
    DemoConsole_Printf("REQUEST %s result=%u\r\n",
                       name, (unsigned int)result);
}

/* 根据编译选项非阻塞开启显示点云；普通障碍检测不依赖此开关。 */
bool DemoCommon_PreparePointCloud(bool *configured)
{
#if RADAR_DEMO_POINT_CLOUD_ENABLE
    radar_client_request_result_t result;
    if (configured == NULL) return false;
    if (*configured) return !RadarClient_IsBusy();
    if (RadarClient_IsBusy()) return false;
    result = RadarClient_RequestPointCloudOutput(true);
    DemoCommon_PrintRequest("point_cloud_on", result);
    if (result == RADAR_CLIENT_REQUEST_ACCEPTED) *configured = true;
    return false;
#else
    (void)configured;
    return true;
#endif
}

/* ======================== 业务结果输出函数 ======================== */

/* 打印方格回调数据。count=0是有效空结果，不是接收失败。 */
void DemoCommon_PrintGrid(const radar_client_grid_result_t *data)
{
    uint8_t index;
    DemoConsole_Printf("GRID sequence=%lu count=%u\r\n",
                       (unsigned long)data->scan_sequence, data->count);
    for (index = 0U; index < data->count; ++index)
        DemoConsole_Printf("  grid[%u]=%u\r\n", index,
                           data->grid_ids[index]);
}

/* 打印完整障碍回调数据，包括坐标、方格、命中圈数和置信度。 */
void DemoCommon_PrintObstacles(const radar_client_obstacle_result_t *data)
{
    uint8_t index;
    DemoConsole_Printf("OBSTACLES sequence=%lu count=%u frames=%u locked=%u overflow=%u\r\n",
                       (unsigned long)data->scan_sequence,
                       data->obstacle_count, data->frame_count,
                       data->locked ? 1U : 0U, data->overflow ? 1U : 0U);
    for (index = 0U; index < data->obstacle_count; ++index) {
        const radar_client_obstacle_t *item = &data->obstacles[index];
        if (item->grid_id == RADAR_CLIENT_GRID_OUTSIDE) {
            DemoConsole_Printf("  %u: x=%d y=%d grid=outside hits=%u confidence=%u points=%u\r\n",
                               index, item->x_mm, item->y_mm,
                               item->frame_hits, item->confidence_percent,
                               item->point_count);
        } else {
            DemoConsole_Printf("  %u: x=%d y=%d grid=%u hits=%u confidence=%u points=%u\r\n",
                               index, item->x_mm, item->y_mm, item->grid_id,
                               item->frame_hits, item->confidence_percent,
                               item->point_count);
        }
    }
}

/* 打印校准回调数据；调用方可从data->pose取得保存后的当前区位姿。 */
void DemoCommon_PrintCalibration(const radar_client_calibration_t *data)
{
    DemoConsole_Printf("CALIBRATION zone=%u x=%d y=%d heading_q6=%u dx=%d dy=%d trim_q6=%d residual=%u pair_error=%u\r\n",
                       data->pose.zone, data->pose.x_mm, data->pose.y_mm,
                       data->pose.heading_q6, data->x_offset_mm,
                       data->y_offset_mm, data->heading_trim_q6,
                       data->residual_mm, data->pair_distance_error_mm);
}

/* 打印路径回调数据；waypoints不包含起点，包含拐点和最终终点。 */
void DemoCommon_PrintPath(const radar_client_path_result_t *data)
{
    uint8_t index;
    DemoConsole_Printf("PATH request=%u status=%u start=%u goal=%u count=%u\r\n",
                       data->request_sequence, data->status,
                       data->start, data->goal, data->waypoint_count);
    for (index = 0U; index < data->waypoint_count; ++index)
        DemoConsole_Printf("  waypoint[%u]=%u\r\n", index,
                           data->waypoints[index]);
}

/* 打印完整重组后的显示点云摘要，不逐点打印以免堵塞调试串口。 */
void DemoCommon_PrintPointCloud(const radar_client_point_cloud_t *data)
{
    DemoConsole_Printf("POINT_CLOUD sequence=%lu displayed=%u source=%u nearest=%u\r\n",
                       (unsigned long)data->scan_sequence,
                       data->point_count, data->source_point_count,
                       data->nearest_mm);
}

/* 显示点云开启时打印逐圈统计事件。 */
void DemoCommon_PrintStats(const radar_client_stats_t *data)
{
    DemoConsole_Printf("STATS sequence=%lu frequency=%u.%uHz points=%u flags=0x%02X\r\n",
                       (unsigned long)data->scan_sequence,
                       data->measured_frequency_deci_hz / 10U,
                       data->measured_frequency_deci_hz % 10U,
                       data->point_count, data->flags);
}
