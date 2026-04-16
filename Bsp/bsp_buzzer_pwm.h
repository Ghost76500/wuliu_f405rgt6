#ifndef BSP_BUZZER_PWM_H
#define BSP_BUZZER_PWM_H

#include "stdint.h"

extern void buzzer_init(void);
extern void buzzer_control(uint16_t frequency, uint8_t volume);
extern void buzzer_rings(uint16_t frequency, uint8_t volume, uint16_t ring_time_ms);


#endif