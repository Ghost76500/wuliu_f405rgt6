#include "radar_task.h"
#include "cmsis_os2.h"

/* ============================== 测试参数 ============================== */

/*
 * 首版只验证连接、区域确认和保存状态读取，不自动启动扫描、校准或路径规划。
 * 必须与雷达实际工作区域一致；不能因为初次接线方便而默认修改为其他区域。
 */
#define TEST_RADAR_ZONE         1U

/*
 * 这些超时只负责控制测试任务自身，不替代 radar_client 中的协议超时。
 * 发生超时后应检查 snapshot 中的 link_state、operation 和错误事件。
 */
#define TEST_REPLY_TIMEOUT_MS   2000U
#define TEST_READY_TIMEOUT_MS   8000U
#define TEST_RESULT_TIMEOUT_MS  2000U

/* ============================== 调试状态 ============================== */

/*
 * 每个枚举值都是一个明确的等待点，便于调试器查看首版流程目前停在哪一步。
 * WAIT_REPLY 只表示雷达任务已经收到请求；ACCEPTED 仍不代表业务最终完成。
 */
typedef enum {
    TEST_RADAR_WAIT_CONNECTION = 0,
    TEST_RADAR_SUBMIT_ZONE,
    TEST_RADAR_WAIT_ZONE_REPLY,
    TEST_RADAR_WAIT_READY,
    TEST_RADAR_SUBMIT_READ_STATE,
    TEST_RADAR_WAIT_READ_REPLY,
    TEST_RADAR_WAIT_PERSISTED,
    TEST_RADAR_DONE,
    TEST_RADAR_FAILED
} TestRadarPhase;

/*
 * 以下变量故意放在文件作用域，并使用 volatile，便于不增加串口打印时
 * 直接通过调试器观察。它们只由 test_radar_task 修改。
 */
volatile TestRadarPhase radar_test_phase = TEST_RADAR_WAIT_CONNECTION;
volatile ReplyCode radar_test_last_reply = REPLY_ACCEPTED;
volatile uint32_t radar_test_last_token = 0U;
volatile radar_client_error_kind_t radar_test_error_kind =
    (radar_client_error_kind_t)0U;
volatile radar_client_operation_t radar_test_error_operation =
    RADAR_CLIENT_OPERATION_NONE;
volatile uint16_t radar_test_error_code = 0U;
volatile uint16_t radar_test_error_detail = 0U;

/*
 * 快照和保存状态按值复制保存，不保存 RadarBridge_GetEvent 返回数据的指针。
 * RadarEvent 中包含联合体，声明为 static 可避免占用测试任务有限的栈空间。
 */
static RadarSnapshot radar_test_snapshot;
static RadarEvent radar_test_event;
static radar_client_persisted_state_t radar_test_persisted;
static bool radar_test_persisted_valid;

/* ============================== 阶段辅助函数 ============================== */

/*
 * 每次进入新阶段都记录进入时刻，用于判断提交、READY或最终事件是否超时。
 * 连接阶段不使用该时间戳，因为雷达可以在物理接线前长时间等待 HELLO。
 */
static uint32_t radar_test_phase_started;

static void RadarTest_SetPhase(TestRadarPhase phase)
{
    radar_test_phase = phase;
    radar_test_phase_started = osKernelGetTickCount();
}

/*
 * 不能用 == 无符号Tick直接比较，因为系统Tick可能回绕。
 * 使用无符号减法判断已过去时间，在 Tick 发生回绕时仍然有效。
 */
static bool RadarTest_PhaseExpired(uint32_t timeout_ms)
{
    return (uint32_t)(osKernelGetTickCount() - radar_test_phase_started) >=
           timeout_ms;
}

/* ============================== 请求提交辅助函数 ============================== */

/*
 * 首版同时只允许一个普通请求处于等待状态。
 * token 是测试任务自己的业务编号，用来匹配提交回复，不是串口 sequence。
 */
static uint32_t radar_test_next_token;
static uint32_t radar_test_active_token;

/*
 * RadarBridge_Submit 在队列满时返回 false，此时测试任务保留状态并于下一轮重试。
 * posted_at 不需要业务侧填写；桥接层提交时会自动填入当前 FreeRTOS Tick。
 */
static bool RadarTest_Submit(RequestType type, uint8_t start, uint8_t goal)
{
    RadarRequest request = {0};

    if (radar_test_phase == TEST_RADAR_FAILED ||
        radar_test_phase == TEST_RADAR_DONE) {
        return false;
    }

    request.token = ++radar_test_next_token;
    request.type = type;
    request.zone = TEST_RADAR_ZONE;
    request.start = start;
    request.goal = goal;

    if (!RadarBridge_Submit(&request)) {
        return false;
    }

    radar_test_active_token = request.token;
    radar_test_last_token = request.token;
    return true;
}

/* ============================== 回复处理辅助函数 ============================== */

/*
 * 回复队列只表示请求是否被 RadarClient 接受，并不代表区域确认、方格扫描或
 * 保存状态读取已经完成。此函数只消费当前 token 的回复。
 */
static bool RadarTest_TakeActiveReply(RadarReply *reply)
{
    if (reply == NULL) {
        return false;
    }

    while (RadarBridge_GetReply(reply)) {
        if (reply->token == radar_test_active_token) {
            radar_test_last_reply = reply->code;
            return true;
        }
        /* 非当前 token 的回复是旧事务残留，只丢弃，不关联到本次测试。 */
    }

    return false;
}

/*
 * BUSY、NOT_READY、QUEUE_FULL 是明确的可重试拒绝，不表示请求已经执行。
 * 所有参数、取消、过期或未预期返回均转为失败，避免状态机无限重试。
 */
static void RadarTest_HandleRejectedReply(ReplyCode code,
                                           TestRadarPhase retry_phase)
{
    switch (code) {
    case REPLY_BUSY:
    case REPLY_NOT_READY:
    case REPLY_QUEUE_FULL:
        radar_test_active_token = 0U;
        RadarTest_SetPhase(retry_phase);
        break;

    case REPLY_ACCEPTED:
    case REPLY_CANCELED:
    case REPLY_EXPIRED:
    case REPLY_BAD_PARAMETER:
    case REPLY_NO_SAVED_GRID:
    default:
        RadarTest_SetPhase(TEST_RADAR_FAILED);
        break;
    }
}

/* ============================== 事件消费辅助函数 ============================== */

/*
 * 每轮最多消费4个事件，防止业务持续发送结果时 test_radar_task 长时间占用CPU。
 * 如果事件队列满，radar_task 会设置 result_delivery_failed 并触发安全停止。
 */
static uint32_t radar_test_persisted_event_count;
static uint32_t radar_test_read_state_event_base;

static void RadarTest_ConsumeEvents(void)
{
    uint8_t processed = 0U;

    while (processed < 4U && RadarBridge_GetEvent(&radar_test_event)) {
        switch (radar_test_event.type) {
        case EVENT_PERSISTED:
            /*
             * PERSISTED_STATE 是按值复制到事件队列的。count=0 仍可能是有效空方格，
             * 因而这里只复制结构，不根据 grid_count 判断是否存在数据。
             */
            radar_test_persisted =
                radar_test_event.data.persisted;
            radar_test_persisted_valid = true;
            ++radar_test_persisted_event_count;
            break;

        case EVENT_ERROR:
            /*
             * ERROR 表示当前事务或链路异常。首版测试不尝试自动修复业务错误，
             * 而是保留错误类别、operation、code和detail供调试。
             */
            radar_test_error_kind = radar_test_event.data.error.kind;
            radar_test_error_operation =
                radar_test_event.data.error.operation;
            radar_test_error_code = radar_test_event.data.error.code;
            radar_test_error_detail = radar_test_event.data.error.detail;
            RadarTest_SetPhase(TEST_RADAR_FAILED);
            break;

        case EVENT_OBSTACLES:
        case EVENT_GRID:
        case EVENT_PATH:
        case EVENT_CALIBRATION:
            /*
             * 首版没有提交这些业务，迟到事件会被忽略，不能被误判为连接测试完成。
             * 后续扩展扫描测试时，应在这里按具体类型增加处理分支。
             */
            break;

        default:
            break;
        }

        ++processed;
    }
}

/* ============================== 首版测试任务 ============================== */

/*
 * test_radar_task 只负责业务编排：
 *   1. 等待雷达连接；
 *   2. 根据实际区域提交 SELECT_ZONE；
 *   3. 等待区域、保存状态和客户端 READY；
 *   4. 显式提交一次 READ_STATE；
 *   5. 收到新的 PERSISTED_STATE 后结束。
 *
 * 禁止在本任务中再次调用 RadarClient_Init、RadarClient_Poll 或任何请求函数，
 * radar_task 是客户端唯一所有者，所有交互必须经过 RadarBridge 接口。
 */
void test_radar_task(void *argument)
{
    static RadarReply reply;
    (void)argument;

    RadarTest_SetPhase(TEST_RADAR_WAIT_CONNECTION);

    for (;;) {
        /*
         * 先取最新状态快照，再有界消费结果事件。回调可能在提交回复前产生事件，
         * 因此事件不能只在 WAIT_PERSISTED 阶段读取。
         */
        if (RadarBridge_GetSnapshot(&radar_test_snapshot)) {
            if (radar_test_snapshot.result_delivery_failed) {
                RadarTest_SetPhase(TEST_RADAR_FAILED);
            }
        }
        RadarTest_ConsumeEvents();

        /*
         * 断线时清除本次事务标记并返回连接等待。客户端会自动重新 HELLO，
         * 但测试任务不能继续使用旧区域、旧保存状态或旧业务 token。
         */
        if (radar_test_phase != TEST_RADAR_DONE &&
            radar_test_phase != TEST_RADAR_FAILED &&
            radar_test_snapshot.client.connected == false &&
            radar_test_phase != TEST_RADAR_WAIT_CONNECTION) {
            radar_test_active_token = 0U;
            radar_test_persisted_valid = false;
            RadarTest_SetPhase(TEST_RADAR_WAIT_CONNECTION);
        }

        switch (radar_test_phase) {
        case TEST_RADAR_WAIT_CONNECTION:
            /*
             * 连接状态由 HELLO 驱动。雷达上电前的长等待是正常情况，不计为失败。
             * 若目标区域和Flash状态已经恢复，可直接进入显式读取阶段；
             * 不应重复发送 SELECT_ZONE 覆盖已有工作区域。
             */
            if (radar_test_snapshot.client.connected) {
                if (radar_test_snapshot.client.selected_zone == TEST_RADAR_ZONE &&
                    radar_test_snapshot.client.ready &&
                    !radar_test_snapshot.client.busy) {
                    RadarTest_SetPhase(TEST_RADAR_SUBMIT_READ_STATE);
                } else {
                    RadarTest_SetPhase(TEST_RADAR_SUBMIT_ZONE);
                }
            }
            break;

        case TEST_RADAR_SUBMIT_ZONE:
            /*
             * 只有客户端普通事务为空时提交选区。提交函数立即返回；之后通过
             * reply_q 判断 ACCEPTED，再通过 snapshot 等待目标区域双重确认。
             */
            if (RadarBridge_GetSnapshot(&radar_test_snapshot) &&
                radar_test_snapshot.client.connected &&
                !radar_test_snapshot.client.busy) {
                if (RadarTest_Submit(REQ_SELECT_ZONE, 0U, 0U)) {
                    RadarTest_SetPhase(TEST_RADAR_WAIT_ZONE_REPLY);
                } else if (RadarTest_PhaseExpired(TEST_REPLY_TIMEOUT_MS)) {
                    RadarTest_SetPhase(TEST_RADAR_FAILED);
                }
            }
            break;

        case TEST_RADAR_WAIT_ZONE_REPLY:
            if (RadarTest_TakeActiveReply(&reply)) {
                if (reply.code == REPLY_ACCEPTED) {
                    RadarTest_SetPhase(TEST_RADAR_WAIT_READY);
                } else {
                    RadarTest_HandleRejectedReply(
                        reply.code, TEST_RADAR_SUBMIT_ZONE);
                }
            } else if (RadarTest_PhaseExpired(TEST_REPLY_TIMEOUT_MS)) {
                RadarTest_SetPhase(TEST_RADAR_FAILED);
            }
            break;

        case TEST_RADAR_WAIT_READY:
            /*
             * ACCEPTED 只说明选区命令已入队。只有 selected_zone 匹配、ready 为真
             * 且普通事务结束，才能开始显式读取保存状态。
             */
            if (radar_test_snapshot.client.ready &&
                radar_test_snapshot.client.selected_zone == TEST_RADAR_ZONE &&
                !radar_test_snapshot.client.busy) {
                RadarTest_SetPhase(TEST_RADAR_SUBMIT_READ_STATE);
            } else if (RadarTest_PhaseExpired(TEST_READY_TIMEOUT_MS)) {
                RadarTest_SetPhase(TEST_RADAR_FAILED);
            }
            break;

        case TEST_RADAR_SUBMIT_READ_STATE:
            /*
             * 记录提交前已消费的保存状态事件数，防止把连接初始化时的旧
             * PERSISTED_STATE 当作本次显式 READ_STATE 请求的最终结果。
             */
            if (radar_test_snapshot.client.ready &&
                !radar_test_snapshot.client.busy) {
                radar_test_read_state_event_base =
                    radar_test_persisted_event_count;
                radar_test_persisted_valid = false;
                if (RadarTest_Submit(REQ_READ_STATE, 0U, 0U)) {
                    RadarTest_SetPhase(TEST_RADAR_WAIT_READ_REPLY);
                } else if (RadarTest_PhaseExpired(TEST_REPLY_TIMEOUT_MS)) {
                    RadarTest_SetPhase(TEST_RADAR_FAILED);
                }
            }
            break;

        case TEST_RADAR_WAIT_READ_REPLY:
            /*
             * 保存状态事件可能比业务任务读取提交回复更早出现。
             * 因此事件消费始终在状态机外执行，这里只校验当前请求的提交结果。
             */
            if (RadarTest_TakeActiveReply(&reply)) {
                if (reply.code == REPLY_ACCEPTED) {
                    RadarTest_SetPhase(TEST_RADAR_WAIT_PERSISTED);
                } else {
                    RadarTest_HandleRejectedReply(
                        reply.code, TEST_RADAR_SUBMIT_READ_STATE);
                }
            } else if (RadarTest_PhaseExpired(TEST_REPLY_TIMEOUT_MS)) {
                RadarTest_SetPhase(TEST_RADAR_FAILED);
            }
            break;

        case TEST_RADAR_WAIT_PERSISTED:
            /*
             * 只有提交请求之后新增的 PERSISTED_STATE 才能证明本次读取完成。
             * grid_result_valid=true 且 grid_count=0 是合法空方格结果。
             */
            if (radar_test_persisted_valid &&
                radar_test_persisted_event_count >
                    radar_test_read_state_event_base) {
                radar_test_active_token = 0U;
                RadarTest_SetPhase(TEST_RADAR_DONE);
            } else if (RadarTest_PhaseExpired(TEST_RESULT_TIMEOUT_MS)) {
                RadarTest_SetPhase(TEST_RADAR_FAILED);
            }
            break;

        case TEST_RADAR_DONE:
            /*
             * 成功结束：保留 snapshot 和 persisted 供调试器检查。
             * 首版只执行一次，避免周期性读取状态造成干扰。
             */
            osDelay(100);
            break;

        case TEST_RADAR_FAILED:
            /*
             * 失败后停止提交新请求并保留最后阶段、回复和错误信息。
             * 业务扩展时可在这里增加明确的人工重试，而不能自动循环覆盖错误。
             */
            osDelay(100);
            break;

        default:
            RadarTest_SetPhase(TEST_RADAR_FAILED);
            break;
        }

        /*
         * 10ms轮询足够用于队列调度，不能在测试任务里以忙循环等待状态。
         * RadarClient_Poll 仍由 radar_task 以约5ms周期独立运行。
         */
        osDelay(10);
    }
}