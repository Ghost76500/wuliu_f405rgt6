#include "bsp_oled.h"
#include "OLED.h"

static uint8_t OLED_Buffer[18]; // OLED显示缓冲区

void oled_init(void)
{
    OLED_Init(); // 初始化OLED显示屏
    delay_ms(5);
    OLED_Clear(); // 清屏
    delay_ms(2);
}

void oled_show(uint8_t *code_data)
{
    OLED_Clear();
    // 将传入的数据复制到显示缓冲区
    for (uint16_t i = 0; i < sizeof(OLED_Buffer); i++)
    {
        OLED_Buffer[i] = code_data[i];
    }
    OLED_ShowString(0, 0, (char*)OLED_Buffer, OLED_16X64); // 显示字符
    OLED_Update(); // 更新显示
}