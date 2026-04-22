#ifndef BSP_KEY_H
#define BSP_KEY_H

#include <stdint.h>

typedef enum {
    KEY_NONE = 0,     // 没有按键
    KEY1_PRESSED_MSG  // 按键1被按下
} KeyMessage_t;

extern uint8_t isKeyClicked(void);

#endif /* BSP_KEY_H */