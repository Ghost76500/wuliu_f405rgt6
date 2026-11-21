#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "radar_client.h"
#include "radar_uart_port.h"
#include "radar_task.h"

static QueueHandle_t request_q, reply_q, event_q, stop_q, snapshot_q;
static bool delivery_failed;
static bool saved_grid_known, saved_grid_valid;
static uint32_t error_generation;

/* 调度器启动前调用一次，任何队列创建失败都不能创建RadarTask。
 * 返回false时由主程序报告启动失败；不要在失败后反复调用而泄漏队列。 */
bool RadarBridge_CreateQueues(void)
{
    request_q = xQueueCreate(1U, sizeof(RadarRequest));
    reply_q = xQueueCreate(1U, sizeof(RadarReply));
    event_q = xQueueCreate(4U, sizeof(RadarEvent));
    stop_q = xQueueCreate(1U, sizeof(uint8_t));
    snapshot_q = xQueueCreate(1U, sizeof(RadarSnapshot));
    return request_q && reply_q && event_q && stop_q && snapshot_q;
}

/* 以下入口只给任务调用，不给中断调用。队列满返回false，调用方保留请求。 */
bool RadarBridge_Submit(const RadarRequest *request)
{
    RadarRequest copy;
    if (request == NULL || request_q == NULL) return false;
    copy = *request;
    copy.posted_at = xTaskGetTickCount();
    return xQueueSend(request_q, &copy, 0U) == pdPASS;
}

bool RadarBridge_Stop(void)
{
    uint8_t value = 1U;
    return stop_q != NULL && xQueueOverwrite(stop_q, &value) == pdPASS;
}

bool RadarBridge_GetReply(RadarReply *reply)
{
    return reply && xQueueReceive(reply_q, reply, 0U) == pdPASS;
}

bool RadarBridge_GetEvent(RadarEvent *event)
{
    return event && xQueueReceive(event_q, event, 0U) == pdPASS;
}

bool RadarBridge_GetSnapshot(RadarSnapshot *snapshot)
{
    return snapshot && xQueuePeek(snapshot_q, snapshot, 0U) == pdPASS;
}

/* 只从客户端回调调用；所有回调都在同一个RadarTask上下文。 */
static void PublishEvent(EventType type, const void *data, size_t size)
{
    static RadarEvent copy; /* 避免大联合体反复占用任务栈。 */
    copy.type = type;
    memset(&copy.data, 0, sizeof(copy.data));
    memcpy(&copy.data, data, size);
    if (xQueueSend(event_q, &copy, 0U) != pdPASS) {
        delivery_failed = true;
        /* 队列满也不能阻塞Poll；由任务进入停止保护并在状态邮箱报告。 */
    }
}

static void OnPersisted(const radar_client_persisted_state_t *data)
{
    saved_grid_known = true;
    saved_grid_valid = data->grid_result_valid;
    PublishEvent(EVENT_PERSISTED, data, sizeof(*data));
}
static void OnObstacles(const radar_client_obstacle_result_t *data)
{
    PublishEvent(EVENT_OBSTACLES, data, sizeof(*data));
}
static void OnGrid(const radar_client_grid_result_t *data)
{
    PublishEvent(EVENT_GRID, data, sizeof(*data));
}
static void OnPath(const radar_client_path_result_t *data)
{
    PublishEvent(EVENT_PATH, data, sizeof(*data));
}
static void OnCalibration(const radar_client_calibration_t *data)
{
    PublishEvent(EVENT_CALIBRATION, data, sizeof(*data));
}
static void OnError(const radar_client_error_t *data)
{
    ++error_generation;
    saved_grid_known = false; /* 失败后先重新读取，不使用旧有效标志。 */
    PublishEvent(EVENT_ERROR, data, sizeof(*data));
}

/* 回调注册区：与任务、请求分发区分开。 */
static void InitCallbacks(radar_client_callbacks_t *callbacks)
{
    memset(callbacks, 0, sizeof(*callbacks));
    callbacks->persisted = OnPersisted;
    callbacks->obstacles = OnObstacles;
    callbacks->grid = OnGrid;
    callbacks->path = OnPath;
    callbacks->calibration = OnCalibration;
    callbacks->error = OnError;
}

static ReplyCode ConvertReply(radar_client_request_result_t result)
{
    switch (result) {
    case RADAR_CLIENT_REQUEST_ACCEPTED: return REPLY_ACCEPTED;
    case RADAR_CLIENT_REQUEST_NOT_READY: return REPLY_NOT_READY;
    case RADAR_CLIENT_REQUEST_BUSY: return REPLY_BUSY;
    case RADAR_CLIENT_REQUEST_QUEUE_FULL: return REPLY_QUEUE_FULL;
    default: return REPLY_BAD_PARAMETER;
    }
}

static ReplyCode DispatchRequest(const RadarRequest *request)
{
    radar_client_request_result_t result;
    /* 必须先检查BUSY，不能反复调用扫描函数冲掉正在接收的结果缓存。 */
    if (RadarClient_IsBusy()) return REPLY_BUSY;
    if (request->type != REQ_SELECT_ZONE &&
        request->type != REQ_READ_STATE && !RadarClient_IsReady())
        return REPLY_NOT_READY;

    switch (request->type) {
    case REQ_SELECT_ZONE:
        result = RadarClient_RequestSelectZone(request->zone);
        if (result == RADAR_CLIENT_REQUEST_ACCEPTED) saved_grid_known = false;
        break;
    case REQ_READ_STATE:
        saved_grid_known = false;
        result = RadarClient_RequestReadPersistedState();
        break;
    case REQ_SCAN_ONCE:
        result = RadarClient_RequestScanOnce();
        break;
    case REQ_GRID_SCAN:
        result = RadarClient_RequestGridScan();
        if (result == RADAR_CLIENT_REQUEST_ACCEPTED) saved_grid_known = false;
        break;
    case REQ_PATH:
        if (!saved_grid_known || !saved_grid_valid) return REPLY_NO_SAVED_GRID;
        result = RadarClient_RequestPlanPath(request->start, request->goal);
        break;
    case REQ_CALIBRATE:
        result = RadarClient_RequestCalibrate(request->zone);
        break;
    default:
        return REPLY_BAD_PARAMETER;
    }
    return ConvertReply(result);
}

void radar_task(void *argument)
{
    static radar_client_callbacks_t callbacks;
    RadarSnapshot snapshot = {0};
    RadarRequest request;
    RadarReply reply;
    uint8_t stop_signal;
    uint32_t stop_started = 0U, last_stop_try = 0U, stop_errors = 0U;
    bool cancel_queued = false, overflow_handled = false;
    TickType_t period = pdMS_TO_TICKS(5);
    (void)argument;

    /* 低Tick频率下换算可能为0；至少等待1 Tick，禁止零延时忙转。 */
    if (period == 0U) period = 1U;
    InitCallbacks(&callbacks);
    RadarClient_Init(&callbacks, 1U); /* 节点ID按实际设置，不在Init里选区。 */
    /* 不默认选区；业务明确需要时单独提交REQ_SELECT_ZONE。 */

    for (;;) {
        uint32_t now = RadarUartPort_GetTick();
        bool stop_requested = xQueueReceive(stop_q, &stop_signal, 0U) == pdPASS;
        if (delivery_failed && !overflow_handled) {
            overflow_handled = true;
            stop_requested = true;
        }
        if (stop_requested) {
            /* 合并重复STOP，不延长正在执行的停止期限。 */
            if (snapshot.stop != STOP_PENDING && snapshot.stop != STOP_WAIT_RESULT) {
                snapshot.stop = STOP_PENDING;
                stop_started = now;
                last_stop_try = now - 100U;
            }
            if (uxQueueMessagesWaiting(request_q) != 0U) cancel_queued = true;
        }

        if (snapshot.stop == STOP_PENDING &&
            (uint32_t)(now - last_stop_try) >= 100U) {
            radar_client_request_result_t result;
            last_stop_try = now;
            stop_errors = error_generation;
            result = RadarClient_RequestStop();
            if (result == RADAR_CLIENT_REQUEST_ACCEPTED)
                snapshot.stop = STOP_WAIT_RESULT;
            else if (result == RADAR_CLIENT_REQUEST_BAD_PARAMETER)
                snapshot.stop = STOP_FAILED;
            /* 未连接或队列满：保留STOP，每100 ms尝试，最多2秒。 */
        }

        RadarClient_Poll(); /* 每一轮都执行，停止和故障期间也不能跳过。 */
        (void)RadarClient_GetState(&snapshot.client);
        if (!snapshot.client.connected) saved_grid_known = false;
        now = RadarUartPort_GetTick();

        if (snapshot.stop == STOP_WAIT_RESULT) {
            if (error_generation != stop_errors)
                snapshot.stop = STOP_FAILED;
            else if (snapshot.client.connected && !snapshot.client.busy &&
                     !snapshot.client.running)
                snapshot.stop = STOP_CONFIRMED;
        }
        if ((snapshot.stop == STOP_PENDING || snapshot.stop == STOP_WAIT_RESULT) &&
            (uint32_t)(now - stop_started) >= 2000U)
            snapshot.stop = STOP_FAILED;

        /* 留出回复槽位后才取请求；每轮最多一项，不无限清空队列。 */
        if (uxQueueSpacesAvailable(reply_q) != 0U &&
            xQueueReceive(request_q, &request, 0U) == pdPASS) {
            reply.token = request.token;
            if (cancel_queued || delivery_failed ||
                (snapshot.stop != STOP_IDLE && snapshot.stop != STOP_CONFIRMED)) {
                reply.code = REPLY_CANCELED;
                cancel_queued = false;
            } else if ((TickType_t)(xTaskGetTickCount() - request.posted_at) >=
                       pdMS_TO_TICKS(2000)) {
                reply.code = REPLY_EXPIRED;
            } else {
                reply.code = DispatchRequest(&request);
            }
            /* reply_q只有本任务写入，已检查空位；失败按适配层故障处理。 */
            if (xQueueSend(reply_q, &reply, 0U) != pdPASS)
                delivery_failed = true;
        }

        /* 分发请求会改变busy/ready，发布前重新取快照，不能发布旧状态。 */
        (void)RadarClient_GetState(&snapshot.client);
        snapshot.result_delivery_failed = delivery_failed;
        (void)xQueueOverwrite(snapshot_q, &snapshot); /* 长1的最新状态邮箱。 */
        vTaskDelay(period); /* 周期以实际Tick与处理时间为准。 */
    }
}