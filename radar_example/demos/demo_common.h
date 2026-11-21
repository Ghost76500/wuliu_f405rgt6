#ifndef DEMO_COMMON_H
#define DEMO_COMMON_H

#include "radar_client.h"

#include <stdbool.h>

/* ======================== 回调表初始化 ======================== */

/*
 * 先清空整张回调表，再注册HELLO、状态、错误、点云和统计等公共回调。
 * 各Demo随后只需覆盖自己关心的业务回调。
 */
void DemoCommon_InitCallbacks(radar_client_callbacks_t *callbacks);

/* ======================== 普通辅助函数 ======================== */

void DemoCommon_PrintRequest(const char *name,
                             radar_client_request_result_t result);
bool DemoCommon_PreparePointCloud(bool *configured);

/* ======================== 可直接注册的业务结果回调 ======================== */

/* 以下函数参数只在调用期间有效；函数内部只打印，不保存data指针。 */
void DemoCommon_PrintPersisted(const radar_client_persisted_state_t *data);
void DemoCommon_PrintGrid(const radar_client_grid_result_t *data);
void DemoCommon_PrintObstacles(const radar_client_obstacle_result_t *data);
void DemoCommon_PrintCalibration(const radar_client_calibration_t *data);
void DemoCommon_PrintPath(const radar_client_path_result_t *data);
void DemoCommon_PrintPointCloud(const radar_client_point_cloud_t *data);
void DemoCommon_PrintStats(const radar_client_stats_t *data);

#endif
