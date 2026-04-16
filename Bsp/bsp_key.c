#include "main.h"
#include "bsp_key.h"

#include <stdlib.h>

#define IS_KEY_PRESSED() (HAL_GPIO_ReadPin(Key1_GPIO_Port, Key1_Pin) == GPIO_PIN_RESET)
// 按键检测间隔
#define KEY_CHECK_INTERVAL 10
// 按键消抖时间
#define KEY_DEBOUNCE_TIME 30
// 按键消抖次数 (30 / 10 = 3次)
#define KEY_DEBOUNCE_COUNT (KEY_DEBOUNCE_TIME / KEY_CHECK_INTERVAL)

uint8_t isKey1Clicked() {
  static uint8_t count = 0;   // 消抖计数
  static uint8_t pressed = 0; // 用于每次按键只返回一次点击事件
  if (IS_KEY_PRESSED() && !pressed) {
    count++;
    if (count >= KEY_DEBOUNCE_COUNT && IS_KEY_PRESSED()) {
      pressed = 1;
      return 1; // 按键被点击
    }
  }
  if (!IS_KEY_PRESSED()) {
    pressed = 0; // 按键释放，重置状态
    count = 0;   // 按键释放，重置消抖计数
  }
  return 0;
}