#ifndef INC_COMMAND_H_
#define INC_COMMAND_H_

#include "main.h"
#include <string.h>

// 定义 Maxicam 数据包固定长度
#define MAXICAM_FRAME_LENGTH 10

uint8_t Command_Write(uint8_t *data, uint16_t length);
uint8_t Command_GetCommand(uint8_t *command);

#endif /* INC_COMMAND_H_ */