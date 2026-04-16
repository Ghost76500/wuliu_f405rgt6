#ifndef UART_RECEIVE_H
#define UART_RECEIVE_H

#include "main.h"
#include <string.h>

uint8_t Command_Write(uint8_t *data, uint8_t length);

uint8_t Command_GetCommand(uint8_t *command);

#endif