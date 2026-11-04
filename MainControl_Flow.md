# STM32 物流车主控代码流程分析

> 分析对象：`E:\STM32VSCode\wuliu_f405rgt6` 当前工作区代码。  
> 当前代码包含未提交修改，本分析按工作区现有代码描述，不按 Git 历史版本描述。

## 1. 总体结论

这套程序不是通用的任务状态机，而是一套“比赛流程脚本”：

1. `main.c` 完成 HAL、外设和 FreeRTOS 初始化。
2. `main_task` 作为总调度器，用阻塞式顺序调用完成扫码、抓取、跑图、加工、放置和入库。
3. `position_task` 以 50 ms 周期完成全局位置闭环，把世界坐标误差转换为底盘世界系速度。
4. `chassis_task` 以 20 ms 标称周期完成航向闭环、世界系/车体系转换、麦轮运动学解算和 CAN 速度下发。
5. MaixCam 视觉数据和 OPS 里程计数据分别通过 USART1 和 CAN1 异步进入，由全局变量或队列与主流程交接。
6. 机械臂、丝杆、伸缩机构、云台和夹爪均属于开环时序控制，依靠固定 `osDelay` 等待动作完成。

主控制链可概括为：

```text
main_task
  ├─ Chassis_Go_Pos
  │    └─ position_task -> chassis_task -> CAN2 -> 四轮电机
  ├─ grab_turntable_B
  │    ├─ color_materials_calibration -> MaixCam + 底盘微调
  │    └─ grab_plate_material/place_material
  │         ├─ TIM3 PWM -> 云台/夹爪
  │         └─ USART3 -> 张大头步进驱动
  └─ grab_materials_car / put_materials_ground / put_materials_materials
       └─ 云台、夹爪、丝杆、伸缩电机组合动作
```

## 2. 工程分层与外设映射

| 模块 | 主要文件 | 职责 |
|---|---|---|
| 启动与 HAL | `Core/Src/main.c` | 时钟、外设、调度器启动 |
| FreeRTOS 任务 | `Core/Src/freertos.c` | 创建队列和 8 个线程 |
| 总流程 | `Application/main_task.c` | 比赛动作编排、扫码、路径点调用 |
| 位置外环 | `Application/position_task.c` | OPS 坐标反馈、位置 PID、到位判定 |
| 底盘控制 | `Application/chassis_task.c` | 航向 PID、速度滤波、麦轮解算、电机下发 |
| CAN 通信 | `Application/CAN_receive.c` | OPS 报文解析、底盘电机收发 |
| 视觉通信 | `User/visual_identity.c`、`User/command.c` | MaixCam 命令、协议解析、误差计算 |
| 机械执行 | `Application/tower.c` | 视觉微调、抓取/放置动作组合 |
| 电机封装 | `Application/zdt_motor.c` | 丝杆电机、伸缩电机参数封装 |
| 舵机 PWM | `Bsp/bsp_servo_pwm.c` | TIM3 控制云台和夹爪 |

主要硬件接口：

| 接口 | 用途 | 关键参数 |
|---|---|---|
| USART1，PA9/PA10 | MaixCam 视觉模块 | 115200，8N1，DMA 空闲中断接收 |
| UART5，PC12/PD2 | GM65 扫码模块 | 9600，8N1，单字节中断 |
| USART3，PC10/PC11 | 张大头步进驱动 | 115200，DMA 发送 |
| CAN1，PA11/PA12 | OPS 里程计 | 500 kbit/s，标准帧 ID `0x200` |
| CAN2，PB12/PB13 | 四路底盘电机 | 500 kbit/s，扩展帧电机 ID `0x01` 到 `0x04` |
| TIM3 CH1/CH3 | 夹爪/云台舵机 | 50 Hz，比较值约 500 到 2500 |
| TIM4 CH3 | 蜂鸣器 | PWM 发声 |
| PC9 | 主流程启动按键 | 低电平按下 |
| PC2 | `key_task` 调试按键 | 低电平按下 |

## 3. 系统启动流程

入口位于 `Core/Src/main.c:75`。

```text
复位/启动
  -> HAL_Init()
  -> SystemClock_Config()
       HSE + PLL，系统时钟 168 MHz
  -> MX_GPIO_Init()
  -> MX_DMA_Init()
  -> MX_CAN1_Init()
  -> MX_CAN2_Init()
  -> MX_I2C1_Init()
  -> MX_UART4_Init()
  -> MX_UART5_Init()
  -> MX_USART1_UART_Init()
  -> MX_USART2_UART_Init()
  -> MX_TIM3_Init()
  -> MX_I2C2_Init()
  -> MX_TIM4_Init()
  -> MX_USART3_UART_Init()
  -> buzzer_init()
  -> can_filter_init()
  -> bsp_servo_pwm_init()
  -> OLED_Init()
  -> osKernelInitialize()
  -> MX_FREERTOS_Init()
       ├─ 创建 KeyQueue
       ├─ 创建 maxicamQueue
       └─ 创建 8 个线程
  -> osKernelStart()
```

调度器启动后，`main()` 中的 `while (1)` 正常情况下不会执行，控制权由 FreeRTOS 接管。

## 4. FreeRTOS 任务结构

任务创建位于 `Core/Src/freertos.c:138`。

| 任务 | 周期/启动延时 | 实际职责 | 当前状态 |
|---|---|---|---|
| `CHASSIS_TASK` | 357 ms 启动延时，20 ms 标称周期 | 底盘反馈、解算、CAN 控制 | 核心运行 |
| `POSITION_TASK` | 387 ms 启动延时，50 ms 周期 | 位置和航向闭环 | 核心运行 |
| `MAIN_TASK` | 3 s 启动延时 | 比赛动作主流程 | 核心运行 |
| `UART_RX_TASK` | 100 ms 启动延时 | 视觉 DMA 数据解析 | 核心运行 |
| `KEY_TASK` | 无延时，10 ms 轮询 | PC2 按键 | 运行，但含调试动作 |
| `MUSIC_TASK` | 无延时，1 ms 轮询 | 歌曲播放 | `song_mode=0` 时空闲 |
| `OUTPUT_TASK` | 150 ms，100 ms 轮询 | OLED 状态页/串口输出 | 显示和发送均已注释 |
| `defaultTask` | 1 ms 轮询 | 空任务 | 空闲 |

任务优先级从高到低大致为：

```text
CHASSIS_TASK
POSITION_TASK = defaultTask
UART_RX_TASK
KEY_TASK
MAIN_TASK
OUTPUT_TASK
MUSIC_TASK
```

其中底盘控制优先级最高，主流程会频繁 `osDelay`，不会长期占用 CPU。

### 队列

- `KeyQueue`：容量 16，元素 1 字节；当前没有生产者和消费者使用。
- `maxicamQueue`：容量 16，元素 34 字节；USART1 空闲中断将视觉串口碎片投递到队列，`uart_rx_task` 消费。

## 5. `main_task` 启动阶段

代码位于 `Application/main_task.c:49`。

```text
main_task 启动
  -> osDelay(3000)
  -> can_chassis_ready_mode()
       失能 1 到 4 号底盘电机，允许人工摆车
  -> 云台回 GIMBAL_INIT_ANGLE
  -> 夹爪回 JIAZHUA_INIT
  -> uart_init_it(&huart5)
       开启 GM65 单字节中断接收
  -> visual_idle()
       向 MaixCam 发送 0xFF，停止识别
  -> Motor_28_Move(QIAN, WAITING)
       伸缩臂伸出，进入检录姿态
  -> osDelay(1000)
  -> 等待 PC9 START 按键变为低电平
  -> osDelay(500)
  -> can_chassis_init()
       使能四路底盘电机
  -> 进入比赛主流程
```

注意：底盘和位置任务在 357/387 ms 时已经初始化，但主任务到 3 s 才重新失能、等待按键并使能底盘。

## 6. 比赛主流程

所有路径点都通过 `Chassis_Go_Pos(x, y, yaw, 到位后延时)` 阻塞执行。

### 6.1 出库、扫码、从转盘抓取

```text
osDelay(10)
Chassis_Go_Pos(-0.15, 0.15, QIANMIAN, 200)
Motor_28_Move(HOU, WAITING)
Chassis_Go_Pos(-0.229, 0.77, QIANMIAN, 200)
get_task_code()
    等待 color_task[0] 和 color_task[5] 至少有一个非 0
    每约 3 s 调用两次相同路径点 (-0.223, 0.77, QIANMIAN)
show_task_code()
    OLED 显示 7 位任务码
Chassis_Go_Pos(-0.127, 1.475, QIANMIAN, 200)
grab_turntable_B(color_task[0], 1)
grab_turntable_B(color_task[1], 2)
grab_turntable_B(color_task[2], 3)
```

三次抓取分别把物料放到车上 1、2、3 号位。

### 6.2 第一轮：前往加工区

```text
Chassis_Go_Pos(-0.175, 1.475, QIANMIAN, 200)
Chassis_Go_Pos(-0.240, 1.076, ZUOBIAN, 200)
Chassis_Go_Pos(-1.904, 1.039, ZUOBIAN, 200)
Chassis_Go_Pos(-1.974, 1.039, HOUMIAN, 200)
```

当前位置的语义是加工区，航向转成 `HOUMIAN`，即朝向后方。

### 6.3 第一轮加工动作

代码按同一套“车上取料 -> 放到对应颜色地面位 -> 从地面取回 -> 放回车上”执行：

```text
grab_materials_car(1) -> put_materials_ground(color_task[0])
grab_materials_car(2) -> put_materials_ground(color_task[1])
grab_materials_car(3) -> put_materials_ground(color_task[2])

grab_materials_ground(color_task[0]) -> put_materials_car(1)
grab_materials_ground(color_task[1]) -> put_materials_car(2)
grab_materials_ground(color_task[2]) -> put_materials_car(3)

reset_posture()
```

代码本身没有改变物料数据或加工状态，动作实质上是按颜色完成搬运和归位。

### 6.4 第一轮放置

```text
Chassis_Go_Pos(-1.887, 1.887, HOUMIAN, 200)
Chassis_Go_Pos(-1.887, 1.887, ZUOBIAN, 200)
Chassis_Go_Pos(-1.079, 1.965, ZUOBIAN, 200)

grab_materials_car(1) -> put_materials_ground(color_task[0])
grab_materials_car(2) -> put_materials_ground(color_task[1])
grab_materials_car(3) -> put_materials_ground(color_task[2])

reset_posture()
```

### 6.5 返回取料区，开始第二轮

```text
Chassis_Go_Pos(-0.163, 1.857, ZUOBIAN, 200)
Chassis_Go_Pos(-0.163, 1.857, QIANMIAN, 200)
Chassis_Go_Pos(-0.122, 1.475, QIANMIAN, 200)

grab_turntable_B(color_task[4], 1)
grab_turntable_B(color_task[5], 2)
grab_turntable_B(color_task[6], 3)
```

因此 7 位任务码的实际使用方式是：

```text
下标 0、1、2 -> 第一轮三种物料颜色
下标 3       -> 当前主流程未使用
下标 4、5、6 -> 第二轮三种物料颜色
```

### 6.6 第二轮加工

路径与第一轮加工区相同，动作变为：

```text
grab_materials_car(1) -> put_materials_ground(color_task[4])
grab_materials_car(2) -> put_materials_ground(color_task[5])
grab_materials_car(3) -> put_materials_ground(color_task[6])

grab_materials_ground(color_task[4]) -> put_materials_car(1)
grab_materials_ground(color_task[5]) -> put_materials_car(2)
grab_materials_ground(color_task[6]) -> put_materials_car(3)
```

### 6.7 第二轮叠放

```text
Chassis_Go_Pos(-1.887, 1.887, HOUMIAN, 200)
Chassis_Go_Pos(-1.887, 1.887, ZUOBIAN, 200)
Chassis_Go_Pos(-1.079, 1.965, ZUOBIAN, 200)

color_materials_calibration(50, 1)
grab_materials_car(1) -> put_materials_materials(color_task[4])
grab_materials_car(2) -> put_materials_materials(color_task[5])
grab_materials_car(3) -> put_materials_materials(color_task[6])
reset_posture()
```

这里视觉校准固定传入绿色值 `50`，没有使用当前物料颜色。

### 6.8 返回与结束

```text
Chassis_Go_Pos(-0.163, 1.857, ZUOBIAN, 200)
Chassis_Go_Pos(-0.163, 1.857, QIANMIAN, 200)
Chassis_Go_Pos(-0.15, 0.15, QIANMIAN, 200)
Chassis_Go_Pos(0.0, 0.0, QIANMIAN, 200)
while (1) { osDelay(100); }
```

完成一次完整流程后，主任务停在无限等待中，不会重新开始。

## 7. 底盘移动调用链

### 7.1 主流程入口

`Chassis_Go_Pos` 位于 `Application/position_task.c:207`：

```text
Chassis_Go_Pos(x, y, yaw, delay)
  -> position_enable(0)
  -> position_set_position_yaw(x, y, yaw)
  -> while (!position_is_arrived()) osDelay(10)
  -> osDelay(delay)
```

到位后没有调用 `position_disable()`，该调用被注释，所以位置环会继续保持最后目标。

### 7.2 50 ms 位置闭环

`position_task` 位于 `Application/position_task.c:93`：

```text
position_feedback_update()
    读取 CAN1 OPS 的 x/y/yaw
    连续 10 个周期没有新里程计数据 -> 判定离线
position_control_calc()
    err_x = target_x - position_x
    err_y = target_y - position_y
    dist = sqrt(err_x^2 + err_y^2)
    err_yaw = rad_format(target_yaw - yaw)
    dist < 0.010 m 且 |err_yaw| < 0.007 rad
        连续 3 个周期 -> arrive_flag = 1
    位置 PID 分别计算 vx_out、vy_out
position_output()
    chassis_set_world_target_yaw(vx_out, vy_out, target_yaw)
```

关键参数：

- 控制周期：50 ms。
- 位置 PID：`Kp=1.3`、`Ki=0`、`Kd=0.05`。
- PID 输出限幅：±1.0。
- 到位距离阈值：10 mm。
- 到位航向阈值：0.007 rad。
- 到位稳定周期：3 次。
- OPS 离线阈值：连续 10 个周期没有新数据，约 0.5 s。

如果 OPS 没有数据，位置环会把 `arrive_flag` 保持为 0，而 `Chassis_Go_Pos` 没有超时，因此主流程会永久阻塞。

### 7.3 20 ms 底盘控制

`chassis_task` 位于 `Application/chassis_task.c:178`：

```text
chassis_task
  ├─ chassis_feedback_update()
  │    ├─ 依次向 1 到 4 号电机发送 S_VEL 读取命令
  │    ├─ 每个电机等待 1 ms
  │    ├─ 从 CAN 反馈更新轮速
  │    └─ 计算底盘 vx、vy、wz 和当前 yaw
  ├─ chassis_set_control()
  │    ├─ 选择世界系或车体系速度
  │    ├─ 世界系速度按 yaw 旋转到车体系
  │    ├─ 一阶低通滤波
  │    ├─ 限制线速度
  │    └─ 航向 PID 输出 wz_set
  ├─ chassis_control_loop()
  │    ├─ 麦轮逆运动学分解为四轮速度
  │    ├─ 轮端方向修正和死区处理
  │    └─ 最大轮速等比例限幅
  └─ chassis_execute_wheel_speeds()
       ├─ m/s 转为 RPM
       └─ can_send_chassis_speed()
```

`can_send_chassis_speed` 位于 `Application/CAN_receive.c:158`：

```text
对每个轮：
  应用电机方向宏
  RPM 限幅到 ±350
  X_V2_Vel_Control(id, dir, acc=255, vel, snF=true)
  osDelay(1)
最后 X_V2_Synchronous_motion(0)
```

X_V2 的命令最终通过 CAN2 扩展帧发送。CAN2 反馈 ID 为 `0x01` 到 `0x04`，中断根据 `RxHeader.ExtId >> 8` 提取真实电机 ID，并检查 `DLC==5`、`RxData[0]==0x35` 后更新轮速。

### 7.4 OPS 数据入口

CAN1 只接收标准帧 ID `0x200`，解析位于 `Application/CAN_receive.c:72`：

```text
RxData[0..1] -> yaw_raw，乘 0.0001 -> rad
RxData[2..3] -> distance_y，乘 0.001 -> m
RxData[4..5] -> distance_x，乘 0.001 -> m
can_odom_new_data_flag = 1
```

`position_task` 读取最新值，并将新数据标志清零。

## 8. 视觉识别与物料对准流程

### 8.1 USART1 接收链

```text
MaixCam
  -> USART1 DMA ReceiveToIdle
  -> HAL_UARTEx_RxEventCallback()
       拷贝串口碎片到 Uart_Msg_t
       osMessageQueuePut(maxicamQueue)
       重启 DMA ReceiveToIdle
  -> uart_rx_task
       Command_Write() 写入 256 字节环形缓冲区
       Command_GetCommand() 循环找完整帧
       maixcam_data_unpack()
       calculate_error()
       visual_data_ready = 1
```

环形缓冲区解析位于 `User/command.c:66`。视觉接收帧固定为 10 字节：

```text
AA 55 mode color y_hi y_lo x_hi x_lo checksum 6B
```

校验规则：

- 帧头必须是 `0xAA 0x55`。
- 帧尾必须是 `0x6B`。
- `checksum` 等于第 1 到第 7 字节之和的低 8 位。
- 坐标按“先 Y 后 X”的字节顺序解析。
- 误差以摄像头中心 `(160, 120)` 为基准计算。

### 8.2 控制命令

`MaixCam_SendCommand` 发送 5 字节：

```text
AA 55 cmd checksum 6B
```

其中：

- `0x0A/0x0B/0x0C`：识别红/绿/蓝物料。
- `0x3A/0x3B/0x3C`：识别红/绿/蓝色环。
- `0x6A/0x6B/0x6C`：识别有物料的色环。
- `0xFF`：停止识别。

### 8.3 物料对准

主动流程使用 `grab_turntable_B`，其内部调用 `color_materials_calibration(color, 0)`：

```text
记录当前 yaw 作为 yaw_hold
云台转到 50 号识别位置
夹爪完全张开
清 visual_data_ready
发送 identify_materials(color)
等待 visual_data_ready

position_disable()
chassis_cmd_disable_world_frame()

循环：
    error_x = g_maxicam_info.error_x
    error_y = g_maxicam_info.error_y
    x_cmd = 0.0013 * error_y
    y_cmd = 0.0013 * error_x
    各自限幅到 ±0.08 m/s
    chassis_set_control_target(x_cmd, y_cmd, yaw_hold)
    osDelay(50)
    如果误差小于阈值并连续满足指定次数 -> 退出

停车
关闭 yaw hold
visual_idle()
```

不同模式：

| mode | 用途 | 误差阈值 | 退出保护 |
|---|---|---|---|
| 0 | 转盘物料 | ±5 像素 | 误差过大时命令置 0；无总超时 |
| 1 | 码垛物料 | ±2 像素 | 约 80 个控制周期后退出 |
| 2 | 地面物料 | ±5 像素 | 首帧等待 500 ms；对准循环无总超时 |

## 9. 抓取、放置与机械动作

### 9.1 从转盘抓到车上

`grab_turntable_B` 位于 `Application/tower.c:450`：

```text
打开夹爪
color_materials_calibration(color, 0)
visual_idle()
蜂鸣器提示
grab_plate_material()
    丝杆下降 WULIAOPAN
    夹爪闭合
    丝杆上升
gimbal_turn(num, 1200)
place_material()
    伸缩臂伸出 NAWULIAO_28
    丝杆下降到 YUNHUO
    夹爪小开
    伸缩臂收回
    丝杆上升
visual_idle()
```

### 9.2 从车上抓取

```text
grab_materials_car(order)
  云台转到 order
  伸缩臂伸出 NAWULIAO_28
  夹爪小开
  丝杆下降到 YUNHUO
  夹爪闭合
  丝杆上升到 TAIQI
  伸缩臂收回
```

### 9.3 放到地面颜色位

```text
put_materials_ground(color)
  云台按颜色转到红/绿/蓝位置
  伸缩臂伸出到对应颜色距离
  丝杆下降 DIMIAN
  夹爪完全张开
  丝杆上升
  伸缩臂收回
```

颜色映射：

- 红色：`49` 或 `6`。
- 绿色：`50` 或 `5`。
- 蓝色：`51` 或 `4`。

### 9.4 从地面抓取

```text
grab_materials_ground(color)
  云台转到颜色位置
  伸缩臂按颜色伸到地面物料位置
  夹爪完全张开
  丝杆下降 DIMIAN
  夹爪闭合
  丝杆上升
  伸缩臂收回
```

### 9.5 叠放到物料上

```text
put_materials_materials(color)
  云台转到颜色位置
  伸缩臂伸出
  丝杆下降到 MADUO
  夹爪完全张开
  丝杆上升
  伸缩臂收回
```

### 9.6 执行机构调用链

```text
云台/夹爪
  -> bsp_gimbal_angle_set() / bsp_gripper_state_set()
  -> __HAL_TIM_SET_COMPARE(TIM3, ...)

丝杆/伸缩电机
  -> Motor_35_Move() / Motor_28_Move()
  -> Emm_V5_Pos_Control()
  -> usart_SendCmd()
  -> HAL_UART_Transmit_DMA(USART3)

底盘四轮
  -> X_V2_*()
  -> can_SendCmd()
  -> CAN2 扩展帧
```

丝杆和伸缩电机没有读取到位反馈，动作可靠性依赖命令发送成功和固定延时足够长。

## 10. 其他任务的实际行为

### `key_task`

PC2 按键消抖后执行：

```text
翻转 LED1
夹爪闭合
等待 1 s
丝杆下降到 DIMIAN
```

这不是单纯的测试空函数，而是会在比赛运行期间直接驱动机械结构的调试动作。

### `output_task`

原本的状态页刷新和 UART2 数据发送均已注释，当前每 100 ms 只执行一次延时：

```text
while (1) {
    osDelay(100);
}
```

OLED 当前主要由 `main_task -> show_task_code()` 显示任务码。

## 11. 完整伪代码

```text
系统启动并创建任务

chassis_task:
    20 ms 周期运行底盘反馈、解算和 CAN 下发

position_task:
    50 ms 周期运行 OPS 位置闭环

uart_rx_task:
    接收 MaixCam 串口碎片并解析视觉帧

main_task:
    延时 3 s
    失能底盘电机
    初始化云台、夹爪、扫码和视觉
    等待 PC9 启动
    使能底盘电机

    出库
    回缩伸缩臂
    到扫码点
    等待任务码并显示
    到物料盘
    抓 3 个物料到车上 1、2、3 号位

    到加工区
    第一轮 3 个物料：车上 -> 地面 -> 车上
    到放置区
    第一轮 3 个物料：车上 -> 地面

    回物料盘
    抓第二批 3 个物料到车上
    到加工区
    第二轮 3 个物料：车上 -> 地面 -> 车上
    到叠放区
    第二轮 3 个物料：车上 -> 物料堆

    返回入口
    停在无限等待
```

## 12. 需要优先关注的代码问题

### 12.1 OPS 失效会让主流程永久阻塞

`Chassis_Go_Pos` 没有超时或异常退出。OPS 连续 0.5 s 无数据后，`position_task` 不会置到位标志，主任务会一直停在 `while (position_is_arrived() == 0)`。

影响：CAN1 断线、OPS 掉电或报文错误都会使整车流程停死。

### 12.2 GM65 只能成功接收一次

`User/gm65.c:31` 收到 `\r\n` 后复制 7 字节并直接 `return`，没有再次调用 `HAL_UART_Receive_IT()`，同时 `rx_index` 没有清零。因此：

- 第一次扫码失败后，主流程的重定位动作无法触发第二次接收。
- 后续扫码不会更新 `color_task`。
- `rx_data` 没有边界保护。

另外，`get_task_code()` 超时执行的两个目标点完全相同，都是 `(-0.223, 0.77, QIANMIAN)`，即使串口还能接收，也不会产生“前后移动”的激励。

### 12.3 视觉等待可能无限挂起

主动抓取使用的 `color_materials_calibration(color, 0)` 在等待 `visual_data_ready` 时没有首帧超时。MaixCam 无响应、串口异常或协议帧不合法时，主流程会永久等待。

`visual_data_timeout` 目前只在 mode 2 的 500 ms 首帧等待中设置，而 mode 2 只被未接入主流程的 `grab_three_ground_materials` 使用。

### 12.4 视觉 DMA 缓冲区不一致

`uart_rx_task` 首次启动 DMA 时使用 `maxicam_rx_data`，但 USART1 回调拷贝和后续重启使用 `dma_rx_buffer`。因此：

- 首帧回调可能拷贝未更新的 `dma_rx_buffer`。
- 串口错误恢复后又切回 `maxicam_rx_data`，会造成缓冲区来回切换。

建议统一为同一个接收缓冲区，或直接使用 HAL 当前维护的接收指针。

### 12.5 `is_error_little` 不是严格“连续满足”

`User/visual_identity.c:128` 在误差不满足时没有清零 `calibration_count`。函数注释称连续满足若干次，但实际是分散满足也会累计，可能过早判定对准成功。

### 12.6 调试按键会与主流程争用执行机构

`key_task` 在 PC2 按下时会闭合夹爪并让丝杆下降，没有和 `main_task` 互锁。比赛过程中误触会直接干扰抓取动作。

### 12.7 坐标轴注释和实际符号修正不完全一致

`chassis_task.h` 声明外部 x 正方向为左、y 正方向为前；但内部又应用了 `CHASSIS_VX_SIGN=-1`，视觉对准处的注释也使用了另一套方向描述。实际车辆方向必须结合 `CHASSIS_VX_SIGN`、`CHASSIS_VY_SIGN`、轮向宏和云台安装方向一起验证，不能只按某一行注释判断。

### 12.8 若干代码未接入当前主线

- `grab_turntable_A`：未调用。
- `grab_turntable_C`：未调用。
- `grab_three_ground_materials`：未调用，而且逻辑会在识别成功后立即返回。
- `color_ring_calibration`：调用均被注释。
- `Update_OPS`：调用均被注释。
- `output_task` 的状态页和 UART2 输出：均被注释。
- `KeyQueue`：创建但未使用。

