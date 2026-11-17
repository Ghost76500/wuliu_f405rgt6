# 雷达控制端函数使用说明

适用雷达固件版本：1.35；应用协议版本：6；Flash保存状态版本：11。

本文用于实现单片机或PC控制端。只开放连接、工作区域、扫描、校准、位姿、保存状态、路径、节点ID和显示点云开关。固件升级、方格布局、回波抑制、最低置信度、启停区屏蔽和路径区域设置只能由配套上位机完成；原始数据、内部定位和诊断接口不开放。

目录内F407VET6和STM32F103C8T6工程已经使用同一套协议v6公共模块。两份工程只有HAL串口端口和芯片工程配置不同。

## 1. 推荐的公共函数

下面的函数名是控制端适配层建议接口。函数都必须立即返回，实际结果通过事件回调或事件队列异步返回。

```c
void RadarClient_Init(const radar_client_callbacks_t *callbacks,
                      uint8_t node_id);
void RadarClient_Poll(void);
bool RadarClient_GetState(radar_client_state_t *state);
bool RadarClient_IsReady(void);
bool RadarClient_IsBusy(void);

radar_client_request_result_t RadarClient_RequestHello(void);
radar_client_request_result_t RadarClient_RequestReadPersistedState(void);
radar_client_request_result_t RadarClient_RequestSelectZone(uint8_t zone);
radar_client_request_result_t RadarClient_RequestStartContinuous(void);
radar_client_request_result_t RadarClient_RequestScanOnce(void);
radar_client_request_result_t RadarClient_RequestGridScan(void);
radar_client_request_result_t RadarClient_RequestCalibrate(uint8_t zone);
radar_client_request_result_t RadarClient_RequestSetPose(
    const radar_client_pose_t *pose);
radar_client_request_result_t RadarClient_RequestSavePose(
    const radar_client_pose_t *pose);
radar_client_request_result_t RadarClient_RequestResetCalibration(uint8_t zone);
radar_client_request_result_t RadarClient_RequestPlanPath(
    uint8_t start, uint8_t goal);
radar_client_request_result_t RadarClient_RequestSetNodeId(uint8_t new_id);
radar_client_request_result_t RadarClient_RequestPointCloudOutput(bool enabled);
radar_client_request_result_t RadarClient_RequestClear(void);
radar_client_request_result_t RadarClient_RequestStop(void);
```

建议返回值：

| 返回值 | 含义 |
|---|---|
| `RADAR_CLIENT_REQUEST_ACCEPTED` | 已进入发送队列，不代表命令已完成 |
| `RADAR_CLIENT_REQUEST_NOT_READY` | 尚未连接或工作区域未确认 |
| `RADAR_CLIENT_REQUEST_BUSY` | 当前已有互斥业务事务 |
| `RADAR_CLIENT_REQUEST_BAD_PARAMETER` | 参数超出范围 |
| `RADAR_CLIENT_REQUEST_QUEUE_FULL` | 发送队列没有空间 |

禁止在函数内部同步等待ACK、14圈结果或路径结果。`RadarClient_RequestStop()`使用独立的高优先级发送位置。

最小裸机初始化：

```c
static void OnGrid(const radar_client_grid_result_t *result)
{
    /* 立即复制或使用result，回调返回后不要继续保存该指针。 */
}

static void OnError(const radar_client_error_t *error)
{
    /* 保存错误码，由业务状态机处理。 */
}

int main(void)
{
    radar_client_callbacks_t callbacks = {0};
    callbacks.grid = OnGrid;
    callbacks.error = OnError;

    /* Init只初始化节点1的通信客户端，不包含工作区域选择。 */
    RadarClient_Init(&callbacks, 1U);
    /* 单独选择启停区1；未完成HELLO时请求会暂存，不阻塞等待。 */
    (void)RadarClient_RequestSelectZone(1U);
    for (;;) {
        RadarClient_Poll();
    }
}
```

回调结构还提供`hello`、`state`、`persisted`、`obstacles`、`point_cloud`、`stats`、`calibration`和`path`。全部回调都由`RadarClient_Poll()`所在上下文执行，回调中不能延时、再次调用Poll或执行长时间计算。

## 2. 强制工作区域前置条件

每次连接、重新上电、通信恢复或节点切换后，都先执行：

1. 对目标节点发送HELLO。
2. 收到协议为6、节点ID匹配的HELLO后读取保存状态。
3. 如果调用方没有提交选区请求，HELLO的`zone=1或2`可恢复雷达本次上电期间已经选择的区域；`zone=0`时等待上层选择。
4. 如果调用方已在`RadarClient_Init()`后单独调用`RadarClient_RequestSelectZone(1或2)`，则以该区域为目标；HELLO返回另一区域时也要切换。
5. 发送SELECT_ZONE，并等待成功ACK和所选区域的HELLO，两者都到达后设置`zone_confirmed=true`。

保存状态中的`source_zone`只表示最近一次修改位姿的区域，不能作为当前工作区域，也不能据此自动选择区域。固件1.34中的`zone1_pose`和`zone2_pose`彼此独立，切换区域时必须采用HELLO回传的当前区域位姿，不能从另一区位姿计算对称结果。

区域未确认时只允许HELLO、HEARTBEAT、STOP、GET_PERSISTED_STATE和SELECT_ZONE。其余公开请求函数必须返回`RADAR_CLIENT_REQUEST_NOT_READY`，不得发出串口帧。

普通STOP、广播STOP和5秒通信超时只停止当前操作，不清除雷达本次上电期间的区域。硬件重新上电、节点ID修改或恢复默认参数后，HELLO会回到`zone=0`。

## 3. 通信与帧格式

- 串口：115200 baud，8N1，无流控。
- 多字节整数：小端。
- 最大载荷：224字节。
- 帧内容先做COBS编码，串口末尾发送`00`分隔符。
- CRC16-CCITT：初值`0xFFFF`，多项式`0x1021`，不反射，不异或输出。

COBS解码后的帧：

```text
magic[2] | version[1] | node_id[1] | type[1] | sequence[2] |
payload_length[2] | payload[N] | crc16[2]
```

| 字段 | 要求 |
|---|---|
| `magic` | ASCII `LR`，字节为`4C 52` |
| `version` | 固定为6 |
| `node_id` | 普通节点1～254；0只允许无回复广播STOP；255无效 |
| `sequence` | 发送方独立递增，65535后回到0 |
| `payload_length` | 必须等于实际载荷，且不超过224 |
| `crc16` | 覆盖magic至payload，不包含CRC和分隔符 |

解码、长度、版本或CRC错误时，只丢弃当前`00`分隔段并继续接收。其他节点的事件不能更新当前节点状态、结果或ACK队列。

## 4. 公开命令与函数映射


| 类型 | 命令 | 载荷 | 建议函数 |
|---:|---|---|---|
| `0x01` | HELLO | 空 | `RadarClient_RequestHello()` |
| `0x02` | HEARTBEAT | 空 | 由`Poll()`每500 ms自动发送 |
| `0x03` | START | 空 | `RadarClient_RequestStartContinuous()` |
| `0x04` | STOP | 空 | `RadarClient_RequestStop()` |
| `0x05` | SELECT_ZONE | `<B zone>` | `RadarClient_RequestSelectZone()` |
| `0x06` | SET_POSE | `<BhhH>` | `RadarClient_RequestSetPose()` |
| `0x07` | SCAN_LOCKED | 空 | `RadarClient_RequestScanOnce()` |
| `0x08` | SINGLE_MEASURE | 空 | `RadarClient_RequestGridScan()` |
| `0x09` | CALIBRATE | `<B zone>` | `RadarClient_RequestCalibrate()` |
| `0x0A` | CLEAR | 空 | `RadarClient_RequestClear()` |
| `0x0B` | RESET_CALIBRATION | `<B zone>` | `RadarClient_RequestResetCalibration()` |
| `0x11` | SAVE_POSE | `<BhhH>` | `RadarClient_RequestSavePose()` |
| `0x13` | GET_PERSISTED_STATE | 空 | `RadarClient_RequestReadPersistedState()` |
| `0x14` | PLAN_PATH | `<BB start, goal>` | `RadarClient_RequestPlanPath()` |
| `0x17` | SET_NODE_ID | `<B new_id>` | `RadarClient_RequestSetNodeId()` |
| `0x19` | SET_POINT_CLOUD_OUTPUT | `<B enabled>` | `RadarClient_RequestPointCloudOutput()` |

广播地址0只允许空载荷STOP，节点不会回复广播STOP。其他命令必须定向发送到1～254的节点。

位姿`<BhhH>`依次为`zone, x_mm, y_mm, heading_q6`。区域2的X/Y范围为-200～500 mm；区域1的X范围为-200～500 mm、Y范围为1900～2600 mm。`heading_q6`单位为`1/64°`：

```text
heading_q6 = round(角度 × 64) mod 23040
角度 = heading_q6 / 64
```

固件1.34中，CALIBRATE、SAVE_POSE和RESET_CALIBRATION只修改载荷指定且当前已经确认的区域，不会修改另一区域。两个区域必须分别校准或分别保存；执行SELECT_ZONE后等待成功ACK和目标区域HELLO，再使用HELLO中的实际位姿。

## 5. 公开事件

第三方只处理下表13种事件；其他事件直接忽略并继续接收。

| 类型 | 事件 | 载荷 |
|---:|---|---|
| `0x81` | HELLO | `<BBBBhhHBH>`，13字节 |
| `0x82` | ACK | `<BB command,result>`，2字节 |
| `0x83` | STATUS | `<BBBBB>`，5字节 |
| `0x84` | SCAN_BEGIN | `<IIHHH>`，14字节 |
| `0x85` | SCAN_POINTS | `<IHB>`后接每点`<hhB>` |
| `0x86` | SCAN_END | `<IHBH>`，9字节 |
| `0x87` | RESULT | `<IBBBB>`后接每个障碍`<hhBBH>` |
| `0x88` | CALIBRATION | `<BhhHhhhBB>`，15字节 |
| `0x89` | STATS | `<IHHHIIIHB>`，25字节 |
| `0x8A` | ERROR | `<BH code,detail>`，3字节 |
| `0x8D` | OBSTACLE_GRID | `<IB>`后接方格编号 |
| `0x8E` | PERSISTED_STATE | 33字节基础载荷后接方格编号 |
| `0x8F` | PATH_RESULT | `<HBBBB>`后接路径节点编号 |

### HELLO

字段依次为：

```text
firmware_major, firmware_minor, protocol, zone,
x_mm, y_mm, heading_q6, running, max_scan_points
```

协议不是6时停止普通业务。`zone=0`表示工作区域未选择，此时位姿字段为0。

### ACK

`result=0`表示成功，非0表示拒绝。ACK没有请求sequence，必须按`node_id + command`先进先出匹配；同一节点同一命令不能同时等待多个ACK。

### STATUS

字段为`status, mode, running, frame_count, overflow`。

| status | 含义 |
|---:|---|
| 0 | 就绪 |
| 1 | 正在启动 |
| 2 | 正在扫描 |
| 3 | 已停止 |
| 4 | 结果锁定/完成 |
| 5 | 正在校准 |
| 6～7 | 保留，按忙处理 |

| mode | 含义 |
|---:|---|
| 0 | 持续扫描 |
| 1 | 单次完整障碍扫描 |
| 2 | 方格扫描 |
| 3 | 校准 |
| 4 | 空闲 |
| 5～7 | 保留，不启动或解释 |

14圈操作直接使用`frame_count`显示0/14～14/14。`overflow=1`时本次结果无效。

### 点云事件

SCAN_BEGIN字段为`scan_sequence, timestamp_ms, frequency_deci_hz, transmitted_point_count, source_point_count`。

SCAN_POINTS头为`scan_sequence, offset, count`，随后每点为`x_mm, y_mm, intensity`，总长度必须为`7 + count*5`。

SCAN_END字段为`scan_sequence, transmitted_point_count, overflow, nearest_mm`。

点云默认关闭。只有显式请求开启后才接收并重组：BEGIN建立定长缓存；POINTS检查`offset + count`不越界；END必须与序号、数量和接收位图完全一致，并且`overflow=0`。不完整、冲突或被新BEGIN覆盖的旧圈直接丢弃。

### 障碍结果

RESULT头为`scan_sequence, frame_count, locked, overflow, obstacle_count`，随后每个障碍为`x_mm, y_mm, frame_hits, confidence_percent, point_count`。总长度必须为`8 + obstacle_count*8`。

实时和单次完整扫描必须先收到同一`scan_sequence`的OBSTACLE_GRID，再收到RESULT。方格数量和障碍数量一致后才发布完整结果。

### 方格结果

OBSTACLE_GRID头为`scan_sequence, count`，随后为`count`个方格值，总长度必须为`5 + count`。完整扫描中0～24表示方格，`0xFF`表示该障碍在方格区域外，必须与同序号RESULT按顺序一一对应。方格扫描中只会返回0～24，本事件本身就是最终结果；`count=0`仍是有效空结果。

### 校准结果

CALIBRATION字段为：

```text
zone, x_mm, y_mm, heading_q6,
x_offset_mm, y_offset_mm, heading_trim_q6,
residual_mm, pair_distance_error_mm
```

只有同时收到成功ACK、`source_zone`与本次校准区域一致的可靠保存状态、CALIBRATION且没有保存错误时，才能把“校准并保存”判为成功。任一事件缺失都必须等待或按超时失败处理。

### 保存状态

PERSISTED_STATE基础载荷布局：

| 偏移 | 类型 | 字段 |
|---:|---|---|
| 0 | `uint16` | storage_version |
| 2 | `uint32` | record_sequence |
| 6 | `uint8` | source_zone，仅表示最近一次修改的位姿区域 |
| 7 | 2字节 | 保留，跳过 |
| 9 | `uint8` | grid_result_valid |
| 10 | `uint8` | grid_count |
| 11 | 7字节 | zone1_pose，`<hhHB>` |
| 18 | 7字节 | zone2_pose，`<hhHB>` |
| 25 | 8字节 | 保留，跳过 |
| 33 | `uint8[]` | 上次方格结果 |

总长度必须为`33 + grid_count`，编号不能重复。固件1.35正常回传的`storage_version`为11。`zone1_pose`和`zone2_pose`是独立保存的两份位姿，不能互相推导。`grid_result_valid=1, grid_count=0`表示有效空扫描。保留字段只跳过，不能用于修改配置。

### 路径结果

PATH_RESULT头为`request_sequence, status, start, goal, waypoint_count`，随后为路径节点。

| status | 含义 |
|---:|---|
| 0 | 成功 |
| 1 | 没有有效方格结果 |
| 2 | 起点被占用 |
| 3 | 终点被占用 |
| 4 | 不可达 |
| 5 | 参数错误 |
| 6 | 容量超限 |

成功列表不包含起点，包含最终终点；直线路径只返回终点；起点等于终点时返回空列表。

### 错误事件

| code | 含义 |
|---:|---|
| 1 | 命令或参数错误 |
| 2 | 扫描数据超时 |
| 3 | 本圈数据过多，结果无效 |
| 4 | 障碍候选过多，结果无效 |
| 5 | 校准失败 |
| 6 | 参数保存失败 |
| 7 | 事件发送失败 |
| 10 | 尚未选择工作区域；detail为被拒绝命令 |

校准失败且`detail & 0xFF00 == 0x0100`时，低8位依次表示：1候选不足、2标定柱组合不符合要求、3位姿超范围、4拟合误差超限、5存在其他障碍、6结果不唯一、7候选数量超限。其他详情按通用校准失败处理。

## 6. 命令完成判定

ACK只表示命令已受理，不能替代最终事件。

| 请求 | 正常返回 | 完成条件 |
|---|---|---|
| HELLO | HELLO → ACK | 本节点v6 HELLO和成功ACK均收到 |
| HEARTBEAT | 无 | 帧成功进入发送队列，不等待ACK |
| SELECT_ZONE | ACK → HELLO | ACK成功且HELLO.zone等于目标区域 |
| START | ACK → STATUS，之后GRID → RESULT | ACK成功且进入持续扫描 |
| STOP | ACK → stopped STATUS | 收到成功ACK或停止状态 |
| SCAN_LOCKED | ACK → 0/14…14/14 → GRID → RESULT → locked STATUS | GRID、RESULT和完成状态齐全且无溢出 |
| SINGLE_MEASURE | ACK → 0/14…14/14 → GRID → locked STATUS | 收到最终GRID和完成状态 |
| CALIBRATE | ACK → 进度 → 保存状态 → CALIBRATION或ERROR | 解算和保存都成功 |
| SET_POSE | ACK | 成功ACK，只修改会话值 |
| SAVE_POSE | ACK → PERSISTED_STATE → HELLO | 三者一致且成功 |
| RESET_CALIBRATION | ACK → PERSISTED_STATE → HELLO | 三者一致且成功 |
| GET_PERSISTED_STATE | PERSISTED_STATE → ACK | 合法状态和成功ACK都收到 |
| PLAN_PATH | ACK → PATH_RESULT | request_sequence匹配且检查status |
| SET_NODE_ID | 旧ID ACK → 新ID HELLO | 两者都收到后才切换本地目标ID |
| SET_POINT_CLOUD_OUTPUT | ACK | 成功ACK后开关生效 |
| CLEAR | ACK → STATUS | 成功ACK并收到新的就绪或持续状态 |

任何ERROR、ACK失败、节点ID不匹配、载荷非法或事务超时都不能判为成功。STOP或事务失败后到达的旧结果必须按节点、扫描序号和事务状态丢弃。

## 7. 标准使用流程

### 建立连接

```text
打开串口并清空缓冲
  -> 每200 ms定向发送HELLO，单轮等待1.5 s
  -> 收到本节点v6 HELLO
  -> 读取保存状态
  -> 按HELLO.zone恢复或明确选择工作区域
  -> 每500 ms心跳，每2 s HELLO健康探测
```

没有回复时保持串口打开并后台重试，不要阻塞业务任务或反复关闭串口。

### 连续扫描

`RadarClient_RequestStartContinuous()`成功后，持续接收关联好的方格和障碍结果；需要结束时提交STOP。

### 单次完整障碍扫描

`RadarClient_RequestScanOnce()`返回14圈完整障碍结果，适合需要坐标、置信度和方格的单次读取。

### 方格扫描与路径

路径规划使用雷达Flash中最近一次有效方格扫描结果。连接后从`persisted`回调检查`grid_result_valid`：为真时可调用`RadarClient_RequestPlanPath(start, goal)`；为假时应拒绝规划并提示先单独执行方格扫描。`grid_result_valid=true、grid_count=0`是有效空扫描，仍允许规划。

需要更新障碍方格时，单独调用`RadarClient_RequestGridScan()`并等待完成；不要由路径请求自动触发扫描，也不要把方格扫描放入固定周期循环。

### 校准

先选择雷达实际所在区域；区域1和区域2需要分别执行校准。校准一区不会生成或覆盖另一区位姿。

调用`RadarClient_RequestCalibrate(active_zone)`后异步等待进度和最终结果。过程中继续收包、心跳并允许STOP。

### 断开

主动断开前发送定向STOP，最多等待400 ms，再关闭串口。广播STOP不等待回复。

## 8. 超时与重试

| 操作 | 建议超时 |
|---|---:|
| HELLO轮询间隔 | 200 ms |
| 建立连接一轮 | 1.5 s |
| 普通ACK | 1.5 s |
| 选择工作区域 | 1.5 s |
| 定向STOP | 400 ms |
| 14圈扫描或校准总时限 | 8 s |
| 14圈进度最大静默 | 3.5 s |
| 路径结果 | 1.5 s |
| 保存位姿或恢复校准 | 2 s |
| 节点ID切换 | 2 s |

只读请求超时后可用新发送序号重试。扫描或校准超时先STOP，再开始新事务。保存类请求超时后先读取保存状态，不能盲目重复写入。连续5秒没有当前节点有效事件时，取消本地事务、清除半帧并重新握手。

## 9. 坐标与方格

- 场地范围：`0 ≤ X,Y < 2400 mm`。
- 方格编号为0～24，调用端直接使用返回编号，不根据障碍坐标重新计算。
- 编号从`(0,0)`开始沿Y方向蛇形排列：

```text
20 21 22 23 24
19 18 17 16 15
10 11 12 13 14
 9  8  7  6  5
 0  1  2  3  4
```

## 10. 固定协议测试向量

以下是完整串口字节，可直接测试编码、小端序和CRC：

```text
节点1、序号0、HELLO：
06 4C 52 06 01 01 01 01 01 03 AD F8 00

节点1、序号1、选择工作区域2：
07 4C 52 06 01 05 01 02 01 04 02 95 78 00

广播地址0、序号2、STOP：
04 4C 52 06 03 04 02 01 01 03 32 73 00
```

接收端应能处理逐字节输入、任意分块、连续多帧、垃圾前缀和坏CRC后的恢复。

## 11. 最小验收清单

1. 只发送第4节列出的16种公开命令。
2. 只用第5节列出的13种事件更新业务；其他事件忽略。
3. 未确认工作区域时不会发出受限业务请求。
4. 接收、发送、心跳、超时和业务回调全程非阻塞。
5. 等待14圈时500 ms心跳不中断，STOP可立即发送。
6. 不同节点事件不会串入当前结果或ACK队列。
7. GRID与RESULT严格按scan_sequence关联，点云按BEGIN/POINTS/END完整重组。
8. 通信静默5秒后自动重新握手，串口保持可恢复。
9. 点云默认关闭；关闭时其他公开功能正常。
10. ACK和最终完成事件分开判断，失败后不使用迟到旧结果。
