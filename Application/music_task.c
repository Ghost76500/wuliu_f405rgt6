#include "music_task.h"
#include "bsp_music.h"
#include "cmsis_os2.h"
#include "bsp_buzzer_pwm.h"


uint8_t song_mode = 0; // 0=不播放, 1=歌曲1, 2=歌曲2, 3=歌曲3

void music_task(void *argument)
{
    for (;;)
    {
        switch (song_mode)
        {
            case 0:
                // 不播放
                break;
            case 1:
                Play_Song(Haruhikage_Full, Haruhikage_Full_Len, 20);
                break;
            case 2:
                Play_Song(SenrenBanka, SenrenBanka_Len, 20);
                break;
            case 3:
                Play_Song(Mixue_Theme, Mixue_Theme_Len, 20);
                break;
            default:
                // 处理默认情况
                break;
        }
        osDelay(1);
    }
}