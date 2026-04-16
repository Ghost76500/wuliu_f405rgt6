#ifndef BSP_OLED_H
#define BSP_OLED_H

#include "OLED.h"
#include "OLED_Data.h"
#include "bsp_delay.h"
#include <stdint.h>


extern void oled_init(void);
extern void oled_show(uint8_t *code_data);


#endif // BSP_OLED_H