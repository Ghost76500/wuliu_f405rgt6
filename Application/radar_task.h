#ifndef RADAR_TASK_H
#define RADAR_TASK_H

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "radar_client.h"

typedef enum {
    REQ_SELECT_ZONE,
    REQ_READ_STATE,
    REQ_SCAN_ONCE,
    REQ_GRID_SCAN,
    REQ_PATH,
    REQ_CALIBRATE
} RequestType;

typedef struct {
    uint32_t token;
    RequestType type;
    uint8_t zone;
    uint8_t start;
    uint8_t goal;
    TickType_t posted_at;
} RadarRequest;

typedef enum {
    REPLY_ACCEPTED,
    REPLY_NOT_READY,
    REPLY_BUSY,
    REPLY_BAD_PARAMETER,
    REPLY_QUEUE_FULL,
    REPLY_CANCELED,
    REPLY_EXPIRED,
    REPLY_NO_SAVED_GRID
} ReplyCode;

typedef struct {
    uint32_t token;
    ReplyCode code;
} RadarReply;

typedef enum {
    EVENT_PERSISTED,
    EVENT_OBSTACLES,
    EVENT_GRID,
    EVENT_PATH,
    EVENT_CALIBRATION,
    EVENT_ERROR
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
    STOP_IDLE,
    STOP_PENDING,
    STOP_WAIT_RESULT,
    STOP_CONFIRMED,
    STOP_FAILED
} StopState;

typedef struct {
    radar_client_state_t client;
    StopState stop;
    bool result_delivery_failed;
} RadarSnapshot;

extern bool RadarBridge_CreateQueues(void);
extern bool RadarBridge_Submit(const RadarRequest *request);
extern bool RadarBridge_Stop(void);
extern bool RadarBridge_GetReply(RadarReply *reply);
extern bool RadarBridge_GetEvent(RadarEvent *event);
extern bool RadarBridge_GetSnapshot(RadarSnapshot *snapshot);
extern void radar_task(void *argument);

#endif /* RADAR_TASK_H */