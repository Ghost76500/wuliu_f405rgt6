# FreeRTOS雷达控制任务移植示例

适用雷达固件版本：1.35；应用协议版本：6；Flash保存状态版本：11。

本文只说明第三方控制器如何以非阻塞方式调用公开功能。固件升级、方格布局、回波抑制、最低置信度、启停区屏蔽、路径区域设置、原始数据、内部定位和诊断接口不属于本文范围。

## 1. 必须遵守的规则

- 串口接收使用中断或DMA环形缓冲，接收中断中只搬运字节和通知任务。
- 发送函数只把完整命令放入队列并立即返回，不等待ACK或最终结果。
- 以`00`切分COBS帧；坏帧只丢弃当前分隔段，从下一帧继续解析。
- 每500 ms向当前节点发送一次心跳；至少每2秒发送一次HELLO探测。
- 连续5秒没有收到当前节点的有效事件时，结束本地事务、清除残留半帧并重新握手。
- STOP使用高优先级发送队列，在扫描和校准期间也必须能立即发送。
- 每个节点同一时刻只允许一个普通业务事务；STOP可抢占。同一命令不得并发等待多个ACK。
- 未选择工作区域前，禁止开始扫描、校准、位姿、路径、节点ID和点云操作。

## 2. 推荐任务结构

直接复用例程时，只建立一个拥有客户端的雷达任务，不再额外建立协议解析任务：

```text
UART/DMA中断
  └─执行HAL处理并登记发送完成、接收错误；不解析帧

RadarTask（客户端唯一调用者）
  ├─优先检查独立STOP邮箱
  ├─RadarClient_Poll：DMA取数、解析、恢复、心跳和事务超时
  ├─每轮最多处理一个普通请求，并返回提交结果
  └─回调把完整结果复制到结果队列，状态写入最新状态邮箱

MissionTask（业务）
  ├─明确需要时提交一次请求
  ├─读取提交回复、最终结果和最新状态
  └─根据成功、拒绝、取消或超时推进状态机
```

- 所有客户端API（包括状态查询）只由RadarTask调用；其他任务通过队列交互。两个中断通知接口除外。
- 不得让另一个任务读取同一DMA缓冲、再次COBS解码，或并行调用客户端。
- 普通请求队列和提交回复队列各长1；业务任务一次只等待一个提交回复。
- STOP使用独立的长1邮箱，通过覆盖方式合并重复停止请求，不受普通队列满影响。
- 最终结果队列按值复制整个结构，不能传递回调内的临时指针。
- 最新状态允许覆盖；扫描、路径和错误结果不能当成“只保留最新值”静默覆盖。
- 自行实现底层时，解码缓冲至少235字节，发送缓冲至少237字节，RX环形缓冲至少512字节。复用客户端时不要再额外创建第二套协议缓冲。

## 3. 连接和工作区域状态机

```text
RadarClient_Init(callbacks, node_id)；初始化不选区、不启动扫描
  -> 客户端自动HELLO握手，读取保存状态
  -> HELLO.zone=1或2：恢复雷达已有区域；保存状态确认后READY
  -> HELLO.zone=0：保持不可操作，等待业务明确选择
       -> RequestSelectZone(实际区域1或2)
       -> 同时收到成功ACK和目标区域HELLO
       -> 读取保存状态完成后READY
```

连接恢复不能无条件发送SELECT_ZONE。只有业务明确指定区域或中途切换区域时才发送；这会加载该区已保存位姿，覆盖当前临时会话位姿。

初始化后的选区调用必须保持独立。本文任务默认不发送选择命令；业务任务从按键、车辆流程或明确配置取得实际区域后提交选择请求，不能把区域1写死作为所有场景的默认值。

保存状态的`source_zone`只表示最近一次修改位姿的区域，不能用来选区。`zone1_pose`和`zone2_pose`彼此独立。重新上电、节点ID修改或恢复默认参数后必须重新确认；短暂断线恢复则以新HELLO为准。

区域未确认时只允许HELLO、HEARTBEAT、STOP、GET_PERSISTED_STATE和SELECT_ZONE。业务必须等READY，不得把选区请求返回ACCEPTED当作已选区。

## 4. 非阻塞移植骨架

下面四段C代码按顺序放入同一适配文件。它展示队列、任务和常用回调，不是额外的雷达协议实现。工程需要先完成UART/DMA初始化、配置FreeRTOS，并把现有例程的protocol和port加入编译。

### 4.1 类型、队列与业务入口

```c
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "radar_client.h"
#include "radar_uart_port.h"

typedef enum {
    REQ_SELECT_ZONE, REQ_READ_STATE, REQ_SCAN_ONCE,
    REQ_GRID_SCAN, REQ_PATH, REQ_CALIBRATE
} RequestType;

typedef struct {
    uint32_t token;          /* 本地业务编号，不是串口帧sequence。 */
    RequestType type;
    uint8_t zone, start, goal;
    TickType_t posted_at;
} RadarRequest;

typedef enum {
    REPLY_ACCEPTED, REPLY_NOT_READY, REPLY_BUSY, REPLY_BAD_PARAMETER,
    REPLY_QUEUE_FULL, REPLY_CANCELED, REPLY_EXPIRED, REPLY_NO_SAVED_GRID
} ReplyCode;

typedef struct {
    uint32_t token;
    ReplyCode code;          /* 只表示提交情况，不是14圈或路径最终结果。 */
} RadarReply;

typedef enum {
    EVENT_PERSISTED, EVENT_OBSTACLES, EVENT_GRID,
    EVENT_PATH, EVENT_CALIBRATION, EVENT_ERROR
} EventType;

typedef struct {
    EventType type;
    union {
        radar_client_persisted_state_t persisted;
        radar_client_obstacle_result_t obstacles;
        radar_client_grid_result_t grid;
        radar_client_path_result_t path;
        radar_client_calibration_t calibration;
        radar_client_error_t error;
    } data;
} RadarEvent;

typedef enum {
    STOP_IDLE, STOP_PENDING, STOP_WAIT_RESULT, STOP_CONFIRMED, STOP_FAILED
} StopState;

typedef struct {
    radar_client_state_t client;
    StopState stop;
    bool result_delivery_failed; /* 粘滞故障；不能继续使用不完整结果。 */
} RadarSnapshot;

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
```

### 4.2 回调区：只复制数据，不等待、不打印长日志

```c
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
```

`event_q`必须按`sizeof(RadarEvent)`创建。业务收到错误或`result_delivery_failed`后，不能把此前收到的部分数据判为完整成功。该故障保留到适配层重新初始化；先安全停止、消费并记录故障，再由应用决定恢复，不能每轮自动Init。

### 4.3 普通请求分发区

```c
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
```

`REPLY_BUSY/QUEUE_FULL`是明确拒绝，不代表已执行。业务保留原请求，等待就绪或短间隔后再试，并设置自己的重试总时限。未就绪、参数错误、取消、过期必须显式反馈，不能无限重试。已经ACCEPTED的耗时请求只能等待结果，不能按定时器重复发送。

路径请求前先提交READ_STATE，等到有效保存状态且客户端不忙后再提交PATH；没有有效记录就拒绝，不自动扫描。有效空方格结果仍允许规划。

### 4.4 雷达任务区：有界轮询和独立停止保护

```c
void RadarTask(void *argument)
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
```

STOP邮箱只表示停止意图已送达本地任务，`STOP_WAIT_RESULT`只表示命令被客户端接受；只有`STOP_CONFIRMED`才用于确认雷达停止。`STOP_FAILED`时禁止普通业务，报告停止失败，由业务显式重新提交STOP。掉线时不能保证远端已停止，不能把超时当成停止成功。该2秒是适配层完整停止流程期限，不是串口ACK超时。

STOP只停止数据采集，不能代替小车急停或雷达供电开关。已在物理发送中的帧不能从中途截断；STOP抢占的是下一次可发送位置。

## 5. 业务如何调用

创建队列并检查成功后，再创建RadarTask和业务任务。还必须检查任务创建返回值。任务栈深度单位为`StackType_t`元素，不是字节；按实际编译结果、栈高水位和队列内存占用验证，不能照搬其他芯片的固定栈值。

| 业务动作 | 调用及后续检查 |
|---|---|
| 首次使用/切区 | 将实际区域1或2放入REQ_SELECT_ZONE；提交回复ACCEPTED后等状态ready=true且selected_zone匹配 |
| 14圈完整扫描 | REQ_SCAN_ONCE；等EVENT_OBSTACLES和事务完成，检查overflow=false |
| 障碍方格 | REQ_GRID_SCAN；等EVENT_GRID和事务完成，count=0也有效 |
| 路径 | 先REQ_READ_STATE；等保存状态有效、事务完成，再REQ_PATH；检查EVENT_PATH.status |
| 校准 | REQ_CALIBRATE携带当前区域；等校准结果、匹配区域的保存状态和事务完成 |
| 停止 | RadarBridge_Stop；检查STOP_CONFIRMED或STOP_FAILED，不等待普通请求队列 |
| 查询状态 | RadarBridge_GetSnapshot；不从其他任务调用RadarClient_GetState |

业务任务应当：

1. 每次生成一个递增的本地`token`，同一时刻只等待一个提交回复。
2. `RadarBridge_Submit`返回false时保留请求，不清除业务触发标志；有界重试或报告队列满。
3. 取得同token的`RadarReply`；只有ACCEPTED才进入“等待最终结果”。其他返回值按表意拒绝或稍后重试。
4. 每轮有界消费结果队列，例如最多4项；复制需要长期使用的结果。
5. 回调数据可能早于任务读到提交回复到达，需按本地活动事务缓存，不能丢弃“暂时没看到ACCEPTED”的结果。
6. 校验结果类型、扫描/路径序号、区域和错误状态；ACK及`busy=false`均不能单独作为成功依据。
7. 遇到ERROR、取消、通信丢失或结果投递故障，废弃本次部分结果，不因后来收到旧事件而恢复成功。
8. 收到STOP意图后不再提交新的扫描；旧请求收到CANCELED后不得自动重试。
9. 持续扫描可不停产生结果，业务必须及时消费；若带宽或业务处理能力不足，应停止扫描而非静默覆盖结果。
10. 扩展节点切换或其他公开功能时，清除对应旧缓存和业务事务，不能沿用旧节点的方格有效标志。

### 时间基准和中断配置

- 建议实际Poll间隔1～10 ms，并验证高负载下最大间隔；本骨架采用约5 ms轮询。若内核Tick低于100 Hz，1 Tick已超过10 ms，需要调整Tick或任务唤醒方案。
- `RadarUartPort_GetTick()`当前返回HAL毫秒时间。移植后必须保证HAL时间持续递增，不能直接把FreeRTOS Tick数当作毫秒。
- 避免在SysTick里重复增加HAL计数；按工程选定的HAL时间源配置，不改乱已有时间基准。
- UART/DMA异常仍由中断登记，由Poll按当前客户端恢复机制处理。保留现有HAL中断调用和两个通知回调，不能再从另一任务重新启动同一个RX DMA。
- 当前中断通知函数不调用FreeRTOS。若自行增加任务通知或队列，必须使用`...FromISR`接口，遵守可调用系统API的中断优先级限制，必要时请求中断退出后调度；不能在中断里调用普通队列API。
- 回调不得阻塞串口打印、写Flash、延时或再进入Poll。调试输出应交给另一条非阻塞队列。
- 使用本例程时不调用裸机Demo_Poll，业务任务替代Demo流程，避免两套业务同时发命令。

## 6. 常用业务顺序

### 持续扫描

```text
START -> 成功ACK -> 持续扫描状态 -> 周期性GRID + RESULT
```

结束时提交定向STOP。持续结果不应触发新的START。

### 单次完整障碍扫描

```text
SCAN_LOCKED -> 成功ACK -> STATUS 0/14 ... 14/14
            -> 同一scan_sequence的GRID + RESULT -> 完成状态
```

必须等GRID与RESULT数量、序号一致后再把结果交给业务。

### 方格扫描

```text
SINGLE_MEASURE -> 成功ACK -> STATUS 0/14 ... 14/14
               -> GRID -> 完成状态
```

GRID可以是空列表，仍表示一次有效的无障碍结果。路径规划依赖雷达Flash中已经保存的有效方格结果。连接后应从PERSISTED_STATE检查`grid_result_valid`；没有有效记录时拒绝路径请求，不自动启动方格扫描。有效空扫描仍可用于规划。

### 校准

```text
CALIBRATE(zone) -> 成功ACK -> 校准进度
                -> 保存状态 + CALIBRATION，或ERROR
```

校准期间仍要处理心跳和STOP。只有解算结果和保存结果都成功，才能认为校准完成。

固件1.34只保存本次校准区域，另一区域保持不变。状态机必须等待成功ACK、`source_zone`与请求区域一致的PERSISTED_STATE以及CALIBRATION；两个区域需要分别校准。

### 路径规划

```text
PERSISTED_STATE.grid_result_valid=1
    -> PLAN_PATH(start, goal) -> 成功ACK -> PATH_RESULT

PERSISTED_STATE.grid_result_valid=0
    -> 本地拒绝，提示先单独执行方格扫描
```

路径结果不包含起点，包含拐点和终点；起点等于终点时成功结果为空。

## 7. 事件处理要点

- HELLO：核对节点ID和协议版本6，更新工作区域。
- ACK：按`node_id + command`先进先出匹配，不把ACK当作耗时操作完成。
- STATUS：14圈进度直接读取`frame_count`，不要用点云块数量推算。
- GRID和RESULT：仅完整障碍扫描需要以`scan_sequence`关联；方格扫描的GRID本身就是最终数据，不等待RESULT。完整障碍中的`grid_id=0xFF`表示方格区域外，坐标仍有效。
- PATH_RESULT：以`request_sequence`关联路径请求。
- ERROR：结束对应事务并保留错误码和详情。
- 未列入公开白名单的事件：忽略并继续接收，不改变当前状态。

## 8. 超时建议

| 操作 | 超时 |
|---|---:|
| HELLO单次轮询 | 200 ms |
| 建立连接一轮 | 1.5 s |
| 普通ACK | 1.5 s |
| 选择区域ACK与HELLO | 1.5 s |
| 主动断开时的停止等待建议 | 400 ms，不代表已确认远端停止 |
| 14圈扫描或校准总时限 | 8 s |
| 14圈操作进度静默 | 3.5 s |
| 路径结果 | 1.5 s |
| 保存位姿或恢复校准 | 2 s |
| 节点ID切换 | 2 s |

超时后不要阻塞等待。扫描或校准超时应先提交STOP，再启动新事务；保存类命令超时后先读取保存状态，不要立即反复写入。

## 9. 最小验收

1. 逐字节、任意分块、粘包、垃圾前缀和坏CRC后都能继续解析。
2. 等待14圈期间心跳不中断，其他任务不冻结。
3. 工作区域未确认时不会发出受限业务命令。
4. STOP能中止扫描和校准，并且优先于普通请求。
5. 当前节点之外的事件不会更新结果或ACK队列。
6. 点云默认关闭时，方格、障碍、校准、路径和错误功能仍正常。
7. 通信中断5秒后自动重新握手，不要求重启控制器。
8. 程序只构造公开命令；未公开事件只忽略，不中断接收。


## 10. 新增移植验收

- 普通队列满时停止邮箱仍可提交；STOP超时或失败保持禁止业务。
- BUSY、NOT_READY和QUEUE_FULL均有提交回复，请求不会取出后无声丢失。
- 结果队列满时产生粘滞故障并触发停止，不把部分结果用于车辆控制。
- 每轮有界处理请求，即使业务持续提交，Poll、心跳和DMA恢复仍能运行。
- 没有明确选区时不默认选择1；重连恢复已有区域不会重复发送SELECT_ZONE。
- 校准和扫描错误、停止后的旧结果不会被判为成功。
- 测试不同内核Tick设置；不出现零Tick忙循环，毫秒超时不随Tick频率变化。

本文件为移植骨架，不包含FreeRTOS内核、任务创建及板级工程；完成编译后仍需在目标控制器上测试调度、内存和串口异常恢复。

文档四段C代码已分别对照两个例程的客户端头文件，用主机桩以1000 Hz和100 Hz Tick编译并模拟验证请求拒绝、队列满、独立STOP、停止超时、结果复制和轮询持续运行。该检查不等同于真实FreeRTOS调度或串口硬件验收。

FreeRTOS API依据：[队列按值复制及队列满处理](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch05.md)、[Tick时间分辨率](https://www.freertos.org/tick-resolution.html)。
