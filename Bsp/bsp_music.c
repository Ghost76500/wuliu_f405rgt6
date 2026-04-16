/*
 * bsp_music.c
 * 描述：蜂鸣器音乐播放模块的具体实现
 */

#include "bsp_music.h"
#include "tim.h" // 如果你用CubeMX生成，通常定时器句柄在 tim.h 或 main.h 中
#include "bsp_buzzer_pwm.h" // 包含蜂鸣器控制头文件
#include "cmsis_os2.h" // FreeRTOS 2.0 API

/* * 函数名：Play_Song
 * 功能：按照乐谱播放音乐
 * 参数：song   - 乐谱数组指针
 * length - 音符总个数
 */
void Play_Song(const Note* song, uint16_t length, uint8_t volume)
{
    for (int i = 0; i < length; i++)
    {
        // 播放当前音符，音量设为 20 (适中)
        buzzer_control(song[i].frequency, volume);
        
        // 保持发声指定的时间
        osDelay(song[i].duration);
        
        // --- 断奏处理 ---
        // 为了让音符之间有区分感，稍微停顿一小会儿
        buzzer_control(0, 0); // 静音
        osDelay(20);        // 短暂停顿
    }
}

/* --- 乐谱数据定义 --- */

/* * 乐谱：《春日影》 (CRYCHIC ver.)
 * 对应图片中的前奏 + 第一段主歌
 */
const Note Haruhikage_Full[] = {
    // ====== Intro (前奏) ======
    // 谱面第一行: (3 2 1 2 | 3. 4 3 2. | ...)
    // 注意：这里是高音 E6, D6...
    {NOTE_E6, T_8_DOT}, {NOTE_D6, T_16}, {NOTE_C6, T_8_DOT}, {NOTE_D6, T_16}, 
    {NOTE_E6, T_8_DOT}, {NOTE_F6, T_16}, {NOTE_E6, T_8}, {NOTE_D6, T_4_DOT}, 
    
    {NOTE_E6, T_8_DOT}, {NOTE_D6, T_16}, {NOTE_C6, T_8_DOT}, {NOTE_D6, T_16}, 
    {NOTE_E6, T_8_DOT}, {NOTE_F6, T_16}, {NOTE_E6, T_8}, {NOTE_D6, T_8}, {NOTE_C6, T_8}, {NOTE_D6, T_8},

    // ====== Verse 1 (主歌) ======
    // 谱面第三行: |: 3 3 2 4 3 2 | 2 2 1 4 3 2 ...
    // 歌词：Hi ka ri no na ka (光芒之中)
    // 3(四分) 3(八分) 2(八分) 4(十六) 3(十六) 2(八分)
    {NOTE_E5, T_4}, {NOTE_E5, T_8}, {NOTE_D5, T_8}, {NOTE_F5, T_16}, {NOTE_E5, T_16}, {NOTE_D5, T_8},
    
    // 歌词：I tsu bo u tsu (发呆...)
    // 2(四分) 2(八分) 1(八分) 4(十六) 3(十六) 2(八分)
    {NOTE_D5, T_4}, {NOTE_D5, T_8}, {NOTE_C5, T_8}, {NOTE_F5, T_16}, {NOTE_E5, T_16}, {NOTE_D5, T_8},

    // 歌词：Ko u ni ta da (仅仅是在这样的...)
    // 2(四分) 1(八分) 2(八分) 3(八分)
    {NOTE_D5, T_4}, {NOTE_C5, T_8}, {NOTE_D5, T_8}, {NOTE_E5, T_4_DOT}, 

    // 休止符 (0)
    {0, T_4_DOT}, 
    
    // 歌词：Ko to ba (话语)
    // 3(八分) 5(八分) 1(高音)(八分)
    {NOTE_E5, T_8}, {NOTE_G5, T_8}, {NOTE_C6, T_8}, 

    // 谱面第四行
    // 歌词：Wo ka ki na gu tte (涂改着...)
    // 7(八分) 1(高)(八分) 7(八分) 1(高)(八分) | 7(八分) 6(八分) 5(八分)
    {NOTE_B5, T_8}, {NOTE_C6, T_8}, {NOTE_B5, T_8}, {NOTE_C6, T_8}, 
    {NOTE_B5, T_8}, {NOTE_A5, T_8}, {NOTE_G5, T_8},

    // 歌词：Ki ta i su ru da ke (仅仅是期待...)
    // 5(八分) 2(八分) 4(四分) | 4(八分) 3(八分) 3(八分) 3(八分) 5(附点八分)
    {NOTE_G5, T_8}, {NOTE_D5, T_8}, {NOTE_F5, T_4}, 
    {NOTE_F5, T_8}, {NOTE_E5, T_8}, {NOTE_E5, T_8}, {NOTE_E5, T_8}, {NOTE_G5, T_8_DOT},

    // 结尾稍微停顿
    {0, T_4_DOT}
};

// 计算乐谱长度，方便外部调用
const uint16_t Haruhikage_Full_Len = sizeof(Haruhikage_Full) / sizeof(Haruhikage_Full[0]);

/* * 乐谱：《Fuwa Fuwa Time》
 * 基于图片简谱还原 (Verse 1 ~ Outro)
 */
const Note FuwaFuwaTime[] = {
    // ====== Verse 1 (对应简谱第5行) ======
    // "Ki mi wo mi te ru to" (0 1 1 1 1 1 1)
    {0, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, 
    {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, 
    
    // "I tsu mo Heart DO-KI DO-KI" (1 1 1 1 2 2 | 3 1 1 0)
    {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, 
    {NOTE_D5, T_FW_8}, {NOTE_D5, T_FW_8},
    {NOTE_E5, T_FW_4}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_4}, {0, T_FW_4},

    // ====== Verse 2 (对应简谱第9行) ======
    // "Yu re ru o mo i wa" (0 1 1 1 1 1 1)
    {0, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8},
    {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8},

    // "Ma shu ma ro mi ta i ni" (1 1 1 1 1 2 1)
    {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, 
    {NOTE_C5, T_FW_8}, {NOTE_D5, T_FW_8}, {NOTE_C5, T_FW_4},

    // "Fu wa fu wa" (7. 1 1 2) -> 注意7是低音B4
    {NOTE_B4, T_FW_4_D}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_8}, {NOTE_D5, T_FW_4},

    // "I tsu mo ga" (0 3 3 3 2 1)
    {0, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_8}, 
    {NOTE_D5, T_FW_8}, {NOTE_C5, T_FW_4}, 

    // ====== Bridge (对应简谱第13行) ======
    // "n ba ru" (6. 1 1 0) -> 6是低音A4
    {NOTE_A4, T_FW_4_D}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_4}, {0, T_FW_4},

    // "Ki mi no yo ko ga o" (0 3 3 3 2 1 | 5 1 1 0)
    {0, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_8}, 
    {NOTE_D5, T_FW_8}, {NOTE_C5, T_FW_4},
    {NOTE_G5, T_FW_4}, {NOTE_C5, T_FW_8}, {NOTE_C5, T_FW_4}, {0, T_FW_4},

    // "Zu tto mi" (0 3 3 3 2 1)
    {0, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_8}, 
    {NOTE_D5, T_FW_8}, {NOTE_C5, T_FW_4},

    // ====== Chorus 1 (对应简谱第17行) ======
    // "te te mo" (1 6. 1 0) -> 这里的节奏有切分
    {NOTE_C5, T_FW_4}, {NOTE_A4, T_FW_8}, {NOTE_C5, T_FW_4}, {0, T_FW_4},

    // "Ki zu ka na i yo ne" (2 7. 6. 7. | 6. 5. 5.)
    {NOTE_D5, T_FW_4}, {NOTE_B4, T_FW_4}, {NOTE_A4, T_FW_4}, {NOTE_B4, T_FW_4},
    {NOTE_A4, T_FW_4}, {NOTE_G4, T_FW_8}, {NOTE_G4, T_FW_4_D}, {0, T_FW_4},

    // "Yu me no na" (0 3 3 3 2 1)
    {0, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_8}, 
    {NOTE_D5, T_FW_8}, {NOTE_C5, T_FW_4},

    // ====== Chorus 2 (对应简谱第21行) ======
    // "ka na ra" (6. 1 1 0)
    {NOTE_A4, T_FW_4}, {NOTE_C5, T_FW_4}, {NOTE_C5, T_FW_4}, {0, T_FW_4},

    // "Fu ta ri no kyo" (0 3 3 3 2 1 | 5 - - -)
    {0, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_8}, 
    {NOTE_D5, T_FW_8}, {NOTE_C5, T_FW_4},
    {NOTE_G5, T_FW_2 + T_FW_2}, // 连音 5----

    // "ri" (5 - 0 4)
    {NOTE_G5, T_FW_2}, {0, T_FW_4}, {NOTE_F5, T_FW_4},

    // "chi ji me ra re ru no ni na" (3 1 3 3 1 | 3. 2 2 1 | 5 - - -)
    {NOTE_E5, T_FW_4}, {NOTE_C5, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_4}, {NOTE_C5, T_FW_4},
    {NOTE_E5, T_FW_4_D}, {NOTE_D5, T_FW_8}, {NOTE_D5, T_FW_4}, {NOTE_C5, T_FW_4},
    {NOTE_G5, T_FW_2 + T_FW_2},

    // "a a" (0 0 3 4)
    {0, T_FW_2}, {NOTE_E5, T_FW_4}, {NOTE_F5, T_FW_4},

    // ====== Hook (对应简谱第29行 - 关键部分!) ======
    // "Ka mi sa ma o ne ga i" (5 5 5 5 3 4 | 5 5 5 5 0 3 4)
    {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_F5, T_FW_8},
    {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {0, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_F5, T_FW_8},

    // "Fu ta ri da ke no Dream Time"
    {NOTE_G5, T_FW_4_D}, {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_4}, {NOTE_F5, T_FW_8}, {NOTE_E5, T_FW_8},
    {NOTE_D5, T_FW_4}, {0, T_FW_4}, {NOTE_E5, T_FW_4}, {NOTE_D5, T_FW_4},
    
    // "Ku da sa i" (1 - 6 - | 5 3 1 6. | 3 - - 2)
    {NOTE_C5, T_FW_2}, {NOTE_A4, T_FW_2},
    {NOTE_G4, T_FW_4}, {NOTE_E4, T_FW_4}, {NOTE_C4, T_FW_4}, {NOTE_A4, T_FW_4}, // 这里音很低，注意
    {NOTE_E5, T_FW_2 + T_FW_4}, {NOTE_D5, T_FW_4},

    // "O ki ni i ri no u sa cha n" (2 0 3 4 | 5 5 5 5 3 4 | 5 5 5 5 3 4)
    {NOTE_D5, T_FW_4}, {0, T_FW_4}, {NOTE_E5, T_FW_4}, {NOTE_F5, T_FW_4},
    {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_F5, T_FW_8},
    {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_G5, T_FW_8}, {NOTE_E5, T_FW_8}, {NOTE_F5, T_FW_8},

    // "Da i te" (5 - - -)
    {NOTE_G5, T_FW_2 + T_FW_2},

    // "Ko n ya mo O ya su mi" (b6 - 3 2 | 1 - 3 0 | 4. 3 3 2)
    // 注意 b6 (降La) 近似等于 #5 (升Sol)，即 NOTE_G5_SHARP，这里简化用 G5 代替或者 A5
    // 为了初学者方便，这里用 A5 代替 b6 (稍微有点走音) 或 G5。谱面是 b6，其实是 G# (NOTE_G5) 的异名同音。
    {NOTE_G5, T_FW_2}, {0, T_FW_4}, {NOTE_E5, T_FW_4}, {NOTE_D5, T_FW_4},
    {NOTE_C5, T_FW_2}, {NOTE_E5, T_FW_4}, {0, T_FW_4},
    {NOTE_F5, T_FW_4_D}, {NOTE_E5, T_FW_8}, {NOTE_E5, T_FW_4}, {NOTE_D5, T_FW_4},

    // "Fu wa fu wa Time" (1 - 0 0 | 0 0 1 1 | 3 1 0 0)
    {NOTE_C5, T_FW_2}, {0, T_FW_2},
    {0, T_FW_2}, {NOTE_C5, T_FW_4}, {NOTE_C5, T_FW_4},
    {NOTE_E5, T_FW_4}, {NOTE_C5, T_FW_4}, {0, T_FW_2},

    // "Fu wa fu wa Time" (0 0 1 1 | 3 1 0 0 | 0 0 1 1 | 3 1 0 0)
    {0, T_FW_2}, {NOTE_C5, T_FW_4}, {NOTE_C5, T_FW_4},
    {NOTE_E5, T_FW_4}, {NOTE_C5, T_FW_4}, {0, T_FW_2},
    {0, T_FW_2}, {NOTE_C5, T_FW_4}, {NOTE_C5, T_FW_4},
    {NOTE_E5, T_FW_4}, {NOTE_C5, T_FW_4}, {0, T_FW_2}
};

const uint16_t FuwaFuwaTime_Len = sizeof(FuwaFuwaTime) / sizeof(FuwaFuwaTime[0]);

/* * 乐谱：《蜜雪冰城甜蜜蜜》
 * 结构：主旋律 A -> B -> A -> C -> 桥段 -> A -> C
 */
const Note Mixue_Theme[] = {
    // ====== Phrase 1: "你爱我，我爱你，蜜雪冰城甜蜜蜜" ======
    // 对应简谱: 3 5 | 5. 6 | 5 3 | 1 1 2
    {NOTE_E5, T_MX_8}, {NOTE_G5, T_MX_8}, 
    {NOTE_G5, T_MX_8_D}, {NOTE_A5, T_MX_16}, // "5." 是附点，"6" 是十六分
    {NOTE_G5, T_MX_8}, {NOTE_E5, T_MX_8},
    {NOTE_C5, T_MX_8}, {NOTE_C5, T_MX_16}, {NOTE_D5, T_MX_16}, // "1 12"

    // 对应简谱: 3 3 | 2 1 | 2 - 
    {NOTE_E5, T_MX_8}, {NOTE_E5, T_MX_8},
    {NOTE_D5, T_MX_8}, {NOTE_C5, T_MX_8},
    {NOTE_D5, T_MX_2}, // 长音 "甜蜜蜜~"

    // ====== Phrase 2: 重复前半句 ======
    // 对应简谱: 3 5 | 5. 6 | 5 3 | 1 1 2
    {NOTE_E5, T_MX_8}, {NOTE_G5, T_MX_8}, 
    {NOTE_G5, T_MX_8_D}, {NOTE_A5, T_MX_16}, 
    {NOTE_G5, T_MX_8}, {NOTE_E5, T_MX_8},
    {NOTE_C5, T_MX_8}, {NOTE_C5, T_MX_16}, {NOTE_D5, T_MX_16},

    // ====== Phrase 3: 结尾变化 "甜蜜蜜" ======
    // 对应简谱: 3 3 | 2 2 | 1 -
    {NOTE_E5, T_MX_8}, {NOTE_E5, T_MX_8},
    {NOTE_D5, T_MX_8}, {NOTE_D5, T_MX_8},
    {NOTE_C5, T_MX_2}, // 结束音

    // ====== Bridge: "你爱我呀，我爱你" (第14小节) ======
    // 对应简谱: 4 4 | 4 6 6 | 5 5 3 | 2 -
    {NOTE_F5, T_MX_4}, {NOTE_F5, T_MX_4}, 
    {NOTE_F5, T_MX_4}, {NOTE_A5, T_MX_8}, {NOTE_A5, T_MX_8}, // "我呀"
    {NOTE_G5, T_MX_4}, {NOTE_G5, T_MX_8}, {NOTE_E5, T_MX_8},
    {NOTE_D5, T_MX_2}, 

    // ====== Final: 再来一次主题 ======
    // 对应简谱: 3 5 | 5. 6 | 5 3 | 1 1 2
    {NOTE_E5, T_MX_8}, {NOTE_G5, T_MX_8}, 
    {NOTE_G5, T_MX_8_D}, {NOTE_A5, T_MX_16}, 
    {NOTE_G5, T_MX_8}, {NOTE_E5, T_MX_8},
    {NOTE_C5, T_MX_8}, {NOTE_C5, T_MX_16}, {NOTE_D5, T_MX_16},

    // 对应简谱: 3 3 | 2 2 | 1 -
    {NOTE_E5, T_MX_8}, {NOTE_E5, T_MX_8},
    {NOTE_D5, T_MX_8}, {NOTE_D5, T_MX_8},
    {NOTE_C5, T_MX_2},
    
    // 结束休止
    {0, T_MX_4}
};

const uint16_t Mixue_Theme_Len = sizeof(Mixue_Theme) / sizeof(Mixue_Theme[0]);

/* * 乐谱：《Bad Apple!!》
 * BPM: 140
 * 注意：这是高速曲目，蜂鸣器响应要快
 */
const Note BadApple[] = {
    // ====== Intro / Phrase 1 (对应简谱第1行末尾) ======
    // 6 7 1 2 3 (低音6,7 -> 中音1,2,3)
    {NOTE_A4, T_BA_8}, {NOTE_B4, T_BA_8}, {NOTE_C5, T_BA_8}, {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},
    
    // 6 5 (高音6 5) | 3 6 (3 高音6) | 3 2 | 1 7(低)
    {NOTE_A5, T_BA_8}, {NOTE_G5, T_BA_8}, 
    {NOTE_E5, T_BA_4}, {NOTE_A5, T_BA_4}, 
    {NOTE_E5, T_BA_8}, {NOTE_D5, T_BA_8}, {NOTE_C5, T_BA_8}, {NOTE_B4, T_BA_8},

    // ====== Main Riff A (对应简谱第5行) ======
    // 6 7 1 2 3 (低音6 -> 中音)
    {NOTE_A4, T_BA_8}, {NOTE_B4, T_BA_8}, {NOTE_C5, T_BA_8}, {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},
    
    // 2 1
    {NOTE_D5, T_BA_8}, {NOTE_C5, T_BA_8},
    
    // 7 6 7 1 7 6 #5 7 (注意 #5 是 GS4)
    {NOTE_B4, T_BA_8}, {NOTE_A4, T_BA_8}, {NOTE_B4, T_BA_8}, {NOTE_C5, T_BA_8},
    {NOTE_B4, T_BA_8}, {NOTE_A4, T_BA_8}, {NOTE_GS4, T_BA_8}, {NOTE_B4, T_BA_8},

    // ====== Main Riff A 重复 (对应简谱第6小节) ======
    // 6 7 1 2 3 | 6 5
    {NOTE_A4, T_BA_8}, {NOTE_B4, T_BA_8}, {NOTE_C5, T_BA_8}, {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},
    {NOTE_A5, T_BA_8}, {NOTE_G5, T_BA_8},

    // 3 6 | 3 2 1 7
    {NOTE_E5, T_BA_4}, {NOTE_A5, T_BA_4},
    {NOTE_E5, T_BA_8}, {NOTE_D5, T_BA_8}, {NOTE_C5, T_BA_8}, {NOTE_B4, T_BA_8},

    // ====== Main Riff B (对应简谱第9行 - 旋律升高) ======
    // 6 7 1 2 3 | 2 1 (全高音)
    {NOTE_A5, T_BA_8}, {NOTE_B5, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_D6, T_BA_8}, {NOTE_E6, T_BA_8},
    {NOTE_D6, T_BA_8}, {NOTE_C6, T_BA_8},

    // 7 1 2 3 (高音)
    {NOTE_B5, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_D6, T_BA_8}, {NOTE_E6, T_BA_8},

    // 6 7 1 2 3 | 6 5 (高音)
    {NOTE_A5, T_BA_8}, {NOTE_B5, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_D6, T_BA_8}, {NOTE_E6, T_BA_8},
    {NOTE_A6, T_BA_8}, {NOTE_G6, T_BA_8},

    // 3 6 | 3 2 1 7 (高音)
    {NOTE_E6, T_BA_4}, {NOTE_A6, T_BA_4},
    {NOTE_E6, T_BA_8}, {NOTE_D6, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_B5, T_BA_8},

    // ====== Bridge (对应简谱第13行 - 经典变奏) ======
    // 6 7 1 2 3 | 2 1
    {NOTE_A5, T_BA_8}, {NOTE_B5, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_D6, T_BA_8}, {NOTE_E6, T_BA_8},
    {NOTE_D6, T_BA_8}, {NOTE_C6, T_BA_8},

    // 7 6 7 1 7 6 #5 7 (注意 #5 是 GS5)
    {NOTE_B5, T_BA_8}, {NOTE_A5, T_BA_8}, {NOTE_B5, T_BA_8}, {NOTE_C6, T_BA_8},
    {NOTE_B5, T_BA_8}, {NOTE_A5, T_BA_8}, {NOTE_GS5, T_BA_8}, {NOTE_B5, T_BA_8},

    // 6 7 1 2 3 | 6 5
    {NOTE_A5, T_BA_8}, {NOTE_B5, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_D6, T_BA_8}, {NOTE_E6, T_BA_8},
    {NOTE_A6, T_BA_8}, {NOTE_G6, T_BA_8},

    // 3 6 | 3 2 1 7
    {NOTE_E6, T_BA_4}, {NOTE_A6, T_BA_4},
    {NOTE_E6, T_BA_8}, {NOTE_D6, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_B5, T_BA_8},

    // ====== Chorus Start (对应简谱第17行) ======
    // 6 7 1 2 3 | 2 1
    {NOTE_A5, T_BA_8}, {NOTE_B5, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_D6, T_BA_8}, {NOTE_E6, T_BA_8},
    {NOTE_D6, T_BA_8}, {NOTE_C6, T_BA_8},

    // 7 1 2 3
    {NOTE_B5, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_D6, T_BA_8}, {NOTE_E6, T_BA_8},

    // 5 6 3 2 3 | 2 3 | 5 6 3 2 3 | 2 3
    {NOTE_G5, T_BA_8}, {NOTE_A5, T_BA_8}, {NOTE_E5, T_BA_8}, {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},
    {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},
    {NOTE_G5, T_BA_8}, {NOTE_A5, T_BA_8}, {NOTE_E5, T_BA_8}, {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},
    {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},

    // ====== Phrase End (对应简谱第21行) ======
    // 2 1 7 5 6 | 5 6 | 7 1 2 3 6 | 3 5
    {NOTE_D6, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_B5, T_BA_8}, {NOTE_G5, T_BA_8}, {NOTE_A5, T_BA_8},
    {NOTE_G5, T_BA_8}, {NOTE_A5, T_BA_8},
    {NOTE_B5, T_BA_8}, {NOTE_C6, T_BA_8}, {NOTE_D6, T_BA_8}, {NOTE_E6, T_BA_8}, {NOTE_A6, T_BA_8},
    {NOTE_E6, T_BA_8}, {NOTE_G6, T_BA_8},

    // 5 6 3 2 3 | 2 3 | 5 6 3 2 3 | 2 3
    {NOTE_G5, T_BA_8}, {NOTE_A5, T_BA_8}, {NOTE_E5, T_BA_8}, {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},
    {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},
    {NOTE_G5, T_BA_8}, {NOTE_A5, T_BA_8}, {NOTE_E5, T_BA_8}, {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},
    {NOTE_D5, T_BA_8}, {NOTE_E5, T_BA_8},

    // 结束休止
    {0, T_BA_4}
};

const uint16_t BadApple_Len = sizeof(BadApple) / sizeof(BadApple[0]);

/* * 乐谱：《WHITE ALBUM》
 * 1=bA (这里用 C调映射，1=C5)
 * BPM = 100
 */
const Note WhiteAlbum[] = {
    // ====== Intro (前奏，对应简谱前4行) ======
    // 4 5 6 3. | 1 2 0 1 1 3 (低音)
    {NOTE_F4, T_WA_8}, {NOTE_G4, T_WA_8}, {NOTE_A4, T_WA_4}, {NOTE_E4, T_WA_4_D},
    {NOTE_C4, T_WA_8}, {NOTE_D4, T_WA_8}, {0, T_WA_16}, {NOTE_C4, T_WA_16}, {NOTE_C4, T_WA_4_D}, {NOTE_E4, T_WA_8},

    // 4 5 6 3 1 6. | 2 2 0 1 1 3
    {NOTE_F4, T_WA_8}, {NOTE_G4, T_WA_8}, {NOTE_A4, T_WA_4}, {NOTE_E4, T_WA_4}, {NOTE_C4, T_WA_4}, {NOTE_A4, T_WA_8},
    {NOTE_D4, T_WA_4}, {NOTE_D4, T_WA_8}, {0, T_WA_16}, {NOTE_C4, T_WA_16}, {NOTE_C4, T_WA_4}, {NOTE_E4, T_WA_8},

    // 4 5 6 3 1 6. | 2 2 0 1 1 2 3 | 3. 1 1. 5(低) 6(低)
    {NOTE_F4, T_WA_8}, {NOTE_G4, T_WA_8}, {NOTE_A4, T_WA_4}, {NOTE_E4, T_WA_4}, {NOTE_C4, T_WA_4}, {NOTE_A4, T_WA_8},
    {NOTE_D4, T_WA_8}, {NOTE_D4, T_WA_4}, {0, T_WA_16}, {NOTE_C4, T_WA_16}, {NOTE_C4, T_WA_8}, {NOTE_D4, T_WA_8}, {NOTE_E4, T_WA_4},
    {NOTE_E4, T_WA_4_D}, {NOTE_C4, T_WA_8}, {NOTE_C4, T_WA_4_D}, {NOTE_G3, T_WA_8}, {NOTE_A3, T_WA_8},

    // 5 - - 5 6
    {NOTE_G4, T_WA_1}, {NOTE_A4, T_WA_4}, // 过渡音

    // ====== Verse 1 (主歌，su re chi ga u...) ======
    // 6 3 1 2 2. | 6(低) 5 2 2 1 1
    {NOTE_A4, T_WA_4}, {NOTE_E5, T_WA_8_D}, {NOTE_C5, T_WA_16}, {NOTE_D5, T_WA_8}, {NOTE_D5, T_WA_4_D},
    {NOTE_A4, T_WA_4}, {NOTE_G5, T_WA_8_D}, {NOTE_D5, T_WA_16}, {NOTE_D5, T_WA_8}, {NOTE_C5, T_WA_4}, {NOTE_C5, T_WA_4},

    // 2 3 | 4 3 4 4 3 2 1 3 | 3 - - 3 6
    {NOTE_D5, T_WA_8}, {NOTE_E5, T_WA_8},
    {NOTE_F5, T_WA_8}, {NOTE_E5, T_WA_8}, {NOTE_F5, T_WA_8}, {NOTE_F5, T_WA_8}, {NOTE_E5, T_WA_8}, {NOTE_D5, T_WA_8}, {NOTE_C5, T_WA_8}, {NOTE_E5, T_WA_8},
    {NOTE_E5, T_WA_2}, {0, T_WA_4}, {NOTE_E5, T_WA_8}, {NOTE_A5, T_WA_8},

    // 6 3 1 2 2 | 2 6(高) | 5 5 2 2 1 1.
    {NOTE_A5, T_WA_4}, {NOTE_E5, T_WA_8_D}, {NOTE_C5, T_WA_16}, {NOTE_D5, T_WA_8}, {NOTE_D5, T_WA_4},
    {NOTE_D5, T_WA_8}, {NOTE_A5, T_WA_8},
    {NOTE_G5, T_WA_4}, {NOTE_G5, T_WA_8}, {NOTE_D5, T_WA_8}, {NOTE_D5, T_WA_8}, {NOTE_C5, T_WA_4}, {NOTE_C5, T_WA_8_D},

    // 3 | 4. 5 3 2 | 3 - - 3 1
    {NOTE_E5, T_WA_8},
    {NOTE_F5, T_WA_4_D}, {NOTE_G5, T_WA_8}, {NOTE_E5, T_WA_8}, {NOTE_D5, T_WA_8},
    {NOTE_E5, T_WA_2}, {0, T_WA_4}, {NOTE_E5, T_WA_8}, {NOTE_C5, T_WA_8},

    // ====== Pre-Chorus (过渡段，ta i ri a u...) ======
    // 5 - 5 4 | 3 4 5 5 6 1. | 1 5 - 5 4 | 3 4 5 5 5 1(高)
    {NOTE_G5, T_WA_4}, {0, T_WA_8}, {NOTE_G5, T_WA_8}, {NOTE_F5, T_WA_4},
    {NOTE_E5, T_WA_8}, {NOTE_F5, T_WA_8}, {NOTE_G5, T_WA_8}, {NOTE_G5, T_WA_8}, {NOTE_A5, T_WA_4}, {NOTE_C6, T_WA_4_D},
    {NOTE_C6, T_WA_8}, {NOTE_G5, T_WA_4}, {0, T_WA_8}, {NOTE_G5, T_WA_8}, {NOTE_F5, T_WA_4},
    {NOTE_E5, T_WA_8}, {NOTE_F5, T_WA_8}, {NOTE_G5, T_WA_8}, {NOTE_G5, T_WA_8}, {NOTE_G5, T_WA_4}, {NOTE_C6, T_WA_4},

    // ====== Chromatic Part (高潮前的半音阶：yo na ga ri...) ======
    // 5 - 5 4 | 3 4 5 6 6. | 1
    {NOTE_G5, T_WA_4}, {0, T_WA_8}, {NOTE_G5, T_WA_8}, {NOTE_F5, T_WA_4},
    {NOTE_E5, T_WA_8}, {NOTE_F5, T_WA_8}, {NOTE_G5, T_WA_8}, {NOTE_A5, T_WA_8}, {NOTE_A5, T_WA_4_D},
    {NOTE_C5, T_WA_8},

    // 1 #6 6 b6 5 4 5 | 5 - - 5 3 (这里用到 A#, A, G#, G)
    {NOTE_C5, T_WA_8}, {NOTE_AS4, T_WA_8}, {NOTE_A4, T_WA_8}, {NOTE_GS4, T_WA_8}, {NOTE_G4, T_WA_8}, {NOTE_F4, T_WA_8}, {NOTE_G4, T_WA_8},
    {NOTE_G4, T_WA_2}, {0, T_WA_4}, {NOTE_G4, T_WA_8}, {NOTE_E4, T_WA_8},

    // ====== Chorus (副歌，gi te yu ku...) ======
    // 4 5 6 3. 1 | 7 1 2 1 1 .
    {NOTE_F4, T_WA_8}, {NOTE_G4, T_WA_8}, {NOTE_A4, T_WA_4}, {NOTE_E4, T_WA_4_D}, {NOTE_C4, T_WA_8},
    {NOTE_B3, T_WA_8}, {NOTE_C4, T_WA_8}, {NOTE_D4, T_WA_8}, {NOTE_C4, T_WA_8}, {NOTE_C4, T_WA_2},

    // 3 | 4 5 6 3 3 1 6(低) | 2 . 2 2 3 3 .
    {NOTE_E4, T_WA_4},
    {NOTE_F4, T_WA_8}, {NOTE_G4, T_WA_8}, {NOTE_A4, T_WA_4}, {NOTE_E4, T_WA_4}, {NOTE_E4, T_WA_8}, {NOTE_C4, T_WA_8}, {NOTE_A3, T_WA_4},
    {NOTE_D4, T_WA_4_D}, {NOTE_D4, T_WA_8}, {NOTE_D4, T_WA_8}, {NOTE_E4, T_WA_8}, {NOTE_E4, T_WA_4_D},

    // 结束休止
    {0, T_WA_2}
};

const uint16_t WhiteAlbum_Len = sizeof(WhiteAlbum) / sizeof(WhiteAlbum[0]);

/* * 乐谱：《恋ひ恋ふ缘》 (Senren * Banka OP)
 * 原曲：KOTOKO
 * BPM: 144 (原速)
 * Key: Eb (1 = D#)
 */
const Note SenrenBanka[] = {
    // ====== Intro (前奏) ======
    // 3 2 1 2 | 3 (1=Eb, 所以 3=G, 2=F, 1=Eb)
    {NOTE_G5, T_KL_4}, {NOTE_F5, T_KL_8}, {NOTE_DS5, T_KL_8}, {NOTE_F5, T_KL_4}, {NOTE_G5, T_KL_4},
    
    // 2 3 2 1 5.(低) 5(低)
    {NOTE_F5, T_KL_8}, {NOTE_G5, T_KL_8}, {NOTE_F5, T_KL_8}, {NOTE_DS5, T_KL_8}, {NOTE_AS4, T_KL_4_D}, {NOTE_AS4, T_KL_8},

    // 6.(低) 1 6(低) 4(低) 3(低) 2(低)
    {NOTE_C5, T_KL_4_D}, {NOTE_DS5, T_KL_8}, {NOTE_C5, T_KL_8}, {NOTE_GS4, T_KL_8}, {NOTE_G4, T_KL_8}, {NOTE_F4, T_KL_8},

    // 2(低) 1(低) 2(低) 3(低) - (注意这里转调色彩)
    {NOTE_F4, T_KL_8}, {NOTE_DS4, T_KL_8}, {NOTE_F4, T_KL_8}, {NOTE_G4, T_KL_8}, {NOTE_G4, T_KL_2},

    // 6(低) 5 6 3 | 5 3 (低音旋律)
    {NOTE_C5, T_KL_4}, {NOTE_AS4, T_KL_8}, {NOTE_C5, T_KL_8}, {NOTE_G4, T_KL_4}, {NOTE_AS4, T_KL_8}, {NOTE_G4, T_KL_8},

    // 2 1 2 3 6(低) 6 7(低)
    {NOTE_F4, T_KL_8}, {NOTE_DS4, T_KL_8}, {NOTE_F4, T_KL_8}, {NOTE_G4, T_KL_8}, {NOTE_C5, T_KL_4}, {NOTE_C5, T_KL_8}, {NOTE_D5, T_KL_8},

    // 1 2 3 5
    {NOTE_DS5, T_KL_4}, {NOTE_F5, T_KL_4}, {NOTE_G5, T_KL_4}, {NOTE_AS5, T_KL_4},

    // 3 - - -
    {NOTE_G5, T_KL_2}, {NOTE_G5, T_KL_2},

    // ====== Verse (主歌: Hana no youni...) ======
    // 0 0 0 3 5
    {0, T_KL_2_D}, {NOTE_G5, T_KL_8}, {NOTE_AS5, T_KL_8},

    // 6 5 6 3 -
    {NOTE_C6, T_KL_4}, {NOTE_AS5, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_G5, T_KL_2},

    // 0 6 6 5 6 5 3 (切分)
    {0, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_AS5, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_AS5, T_KL_8}, {NOTE_G5, T_KL_4},

    // 2 3 5 6 3
    {NOTE_F5, T_KL_4}, {NOTE_G5, T_KL_8}, {NOTE_AS5, T_KL_8}, {NOTE_C6, T_KL_4}, {NOTE_G5, T_KL_4},

    // 3 - 0 0
    {NOTE_G5, T_KL_2}, {0, T_KL_2},

    // 4(低) 1 3 4 3(低) 1
    {NOTE_GS4, T_KL_8_D}, {NOTE_DS5, T_KL_16}, {NOTE_G5, T_KL_8}, {NOTE_GS5, T_KL_8}, {NOTE_G5, T_KL_8_D}, {NOTE_DS5, T_KL_16},

    // 5 2 3 5 3 2
    {NOTE_AS5, T_KL_8_D}, {NOTE_F5, T_KL_16}, {NOTE_G5, T_KL_8}, {NOTE_AS5, T_KL_8}, {NOTE_G5, T_KL_8_D}, {NOTE_F5, T_KL_16},

    // 6 2 3 6 3 2
    {NOTE_C6, T_KL_8_D}, {NOTE_F5, T_KL_16}, {NOTE_G5, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_G5, T_KL_8_D}, {NOTE_F5, T_KL_16},

    // ====== Pre-Chorus (过渡) ======
    // 2 3 2 1 3 -
    {NOTE_F5, T_KL_8}, {NOTE_G5, T_KL_8}, {NOTE_F5, T_KL_8}, {NOTE_DS5, T_KL_8}, {NOTE_G5, T_KL_2},

    // 2 1 2 6(低) -
    {NOTE_F5, T_KL_4}, {NOTE_DS5, T_KL_8}, {NOTE_F5, T_KL_8}, {NOTE_C5, T_KL_2},

    // 0 2 2 3 2 1 2
    {0, T_KL_8}, {NOTE_F5, T_KL_8}, {NOTE_F5, T_KL_8}, {NOTE_G5, T_KL_8}, {NOTE_F5, T_KL_8}, {NOTE_DS5, T_KL_8}, {NOTE_F5, T_KL_4},

    // 3 - - -
    {NOTE_G5, T_KL_2}, {NOTE_G5, T_KL_2},

    // 2. #1 3 5 (#1 在 Eb 调里是 E natural)
    // 2=F, #1=E, 3=G, 5=Bb
    {NOTE_F5, T_KL_4_D}, {NOTE_E5, T_KL_8}, {NOTE_G5, T_KL_4}, {NOTE_AS5, T_KL_4},

    // 6 5 6 3 - (6=C, 5=Bb, 3=G)
    {NOTE_C6, T_KL_4}, {NOTE_AS5, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_G5, T_KL_2},

    // 0 6 6 5 6 5 3
    {0, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_AS5, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_AS5, T_KL_8}, {NOTE_G5, T_KL_4},

    // 2 3 5 6 3
    {NOTE_F5, T_KL_4}, {NOTE_G5, T_KL_8}, {NOTE_AS5, T_KL_8}, {NOTE_C6, T_KL_4}, {NOTE_G5, T_KL_4},

    // 6(低)#1 3 6 3 1 (#1=E)
    {NOTE_C5, T_KL_8_D}, {NOTE_E5, T_KL_16}, {NOTE_G5, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_G5, T_KL_8_D}, {NOTE_DS5, T_KL_16},

    // 4 1 3 4 3 1 (4=Ab)
    {NOTE_GS5, T_KL_8_D}, {NOTE_DS5, T_KL_16}, {NOTE_G5, T_KL_8}, {NOTE_GS5, T_KL_8}, {NOTE_G5, T_KL_8_D}, {NOTE_DS5, T_KL_16},

    // 5 2 3 5 3 2 (5=Bb)
    {NOTE_AS5, T_KL_8_D}, {NOTE_F5, T_KL_16}, {NOTE_G5, T_KL_8}, {NOTE_AS5, T_KL_8}, {NOTE_G5, T_KL_8_D}, {NOTE_F5, T_KL_16},

    // 6 2 3 6 3 2 (6=C)
    {NOTE_C6, T_KL_8_D}, {NOTE_F5, T_KL_16}, {NOTE_G5, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_G5, T_KL_8_D}, {NOTE_F5, T_KL_16},

    // 2 1 2 3 6(低) -
    {NOTE_F5, T_KL_8}, {NOTE_DS5, T_KL_8}, {NOTE_F5, T_KL_8}, {NOTE_G5, T_KL_8}, {NOTE_C5, T_KL_2},

    // 6(低) 1 6 2 1 2
    {NOTE_C5, T_KL_4}, {NOTE_DS5, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_F5, T_KL_8}, {NOTE_DS5, T_KL_8}, {NOTE_F5, T_KL_4},

    // 3 2 3 5 7 | 7 - - 3 2 (7=D)
    {NOTE_G5, T_KL_4}, {NOTE_F5, T_KL_8}, {NOTE_G5, T_KL_8}, {NOTE_AS5, T_KL_4}, {NOTE_D6, T_KL_4},
    {NOTE_D6, T_KL_2}, {NOTE_D6, T_KL_4}, {NOTE_G5, T_KL_8}, {NOTE_F5, T_KL_8},

    // ====== Chorus (副歌/高潮) ======
    // 6. #1 1 - (#1=高音C#, 1=高音C)
    // Eb调下: 6=C. 
    // 谱子是 6. #1 1. 原曲这里是 C6 -> E6 -> Eb6
    // 对应关系: 6->C, #1->E, 1->Eb
    {NOTE_C6, T_KL_4_D}, {NOTE_E6, T_KL_8}, {NOTE_DS6, T_KL_2},

    // 2 - - 3 2 (2=F)
    {NOTE_F6, T_KL_2}, {NOTE_F6, T_KL_4}, {NOTE_G6, T_KL_8}, {NOTE_F6, T_KL_8},

    // 6 - - - (6=C)
    {NOTE_C6, T_KL_2}, {NOTE_C6, T_KL_2},

    // 0 6 1 2 6 1 2 (6=C, 1=Eb, 2=F)
    {0, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_DS6, T_KL_8}, {NOTE_F6, T_KL_8}, {NOTE_C6, T_KL_8_D}, {NOTE_DS6, T_KL_16}, {NOTE_F6, T_KL_4},

    // 2 6 1 2 4 2 1 6 (4=Ab)
    {NOTE_F6, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_DS6, T_KL_8}, {NOTE_F6, T_KL_8}, {NOTE_GS6, T_KL_8}, {NOTE_F6, T_KL_8}, {NOTE_DS6, T_KL_8}, {NOTE_C6, T_KL_8},

    // 5 4 5 5 - (#5=G# -> 还原B natural)
    // 谱子: 5 4 #5 5. 
    // Eb调: 5=Bb, 4=Ab. #5=B(natural).
    {NOTE_AS5, T_KL_4}, {NOTE_GS5, T_KL_8}, {NOTE_B5, T_KL_8}, {NOTE_AS5, T_KL_2},

    // 0 6 1 2 6 1 2 (重复)
    {0, T_KL_4}, {0, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_DS6, T_KL_8}, {NOTE_F6, T_KL_8}, {NOTE_C6, T_KL_8_D}, {NOTE_DS6, T_KL_16}, {NOTE_F6, T_KL_4},

    // 2 6 1 2 4 2 4 6
    {NOTE_F6, T_KL_8}, {NOTE_C6, T_KL_8}, {NOTE_DS6, T_KL_8}, {NOTE_F6, T_KL_8}, {NOTE_GS6, T_KL_8}, {NOTE_F6, T_KL_8}, {NOTE_GS6, T_KL_8}, {NOTE_C7, T_KL_8},

    // 5 1 1 1 - (5=Bb, 1=Eb)
    {NOTE_AS6, T_KL_4}, {NOTE_DS6, T_KL_8}, {NOTE_DS6, T_KL_8}, {NOTE_DS6, T_KL_2},

    // 0 0 0 1 2
    {0, T_KL_2}, {0, T_KL_4}, {NOTE_DS6, T_KL_8}, {NOTE_F6, T_KL_8},

    // 3 4 3 4 3 (#4=A natural)
    // 谱子: 3 4 #4 4 3 (简谱里有升4)
    // Eb调: 3=G, 4=Ab. #4=A.
    {NOTE_G6, T_KL_4}, {NOTE_A6, T_KL_8}, {NOTE_G6, T_KL_8}, {NOTE_A6, T_KL_8}, {NOTE_G6, T_KL_8},

    // 3 2 2 - 2
    {NOTE_G6, T_KL_8}, {NOTE_F6, T_KL_8}, {NOTE_F6, T_KL_4}, {0, T_KL_8}, {NOTE_F6, T_KL_8},

    // 2 3 2 3 2
    {NOTE_F6, T_KL_8}, {NOTE_G6, T_KL_8}, {NOTE_F6, T_KL_8}, {NOTE_G6, T_KL_8}, {NOTE_F6, T_KL_4},

    // 2 1 1. 6 7 (7=D)
    {NOTE_F6, T_KL_8}, {NOTE_DS6, T_KL_8}, {NOTE_DS6, T_KL_4_D}, {NOTE_C6, T_KL_8}, {NOTE_D6, T_KL_8},

    // 1 2 3 4 5 (#4=A)
    // 谱子: 1 2 3 #4 5
    // Eb调: Eb F G A Bb
    {NOTE_DS6, T_KL_4}, {NOTE_F6, T_KL_8}, {NOTE_G6, T_KL_8}, {NOTE_A6, T_KL_4}, {NOTE_AS6, T_KL_4},

    // 5 1 1 6 1 2
    {NOTE_AS6, T_KL_8}, {NOTE_DS6, T_KL_8}, {NOTE_DS6, T_KL_4}, {NOTE_C6, T_KL_8}, {NOTE_DS6, T_KL_8}, {NOTE_F6, T_KL_4},

    // 2. 1 1 - (Ending)
    {NOTE_F6, T_KL_4_D}, {NOTE_DS6, T_KL_8}, {NOTE_DS6, T_KL_2},

    // 结束
    {0, T_KL_4}
};

const uint16_t SenrenBanka_Len = sizeof(SenrenBanka) / sizeof(SenrenBanka[0]);

/* * 乐谱：《我的亲爱》 (Sayonara) - 黎明
 * BPM: 114
 * Key: C
 */
const Note MyBeloved[] = {
    // ====== Intro (前奏) ======
    // 3 3 3 4 3 4 | 5 7 1. 1.
    {NOTE_E5, T_MY_4}, {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_F5, T_MY_4}, {NOTE_E5, T_MY_8}, {NOTE_F5, T_MY_8},
    {NOTE_G5, T_MY_4}, {NOTE_B5, T_MY_4}, {NOTE_C6, T_MY_4}, {NOTE_C6, T_MY_4},

    // 5. 1. 6. 1. 3. 2. 1. (5是中音G5, 1是高音C6)
    {NOTE_G5, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_E6, T_MY_8}, {NOTE_D6, T_MY_8}, {NOTE_C6, T_MY_4},

    // 0 1 7 1 2 1 | 7 5 - - (低音7,5 -> B4, G4)
    {0, T_MY_8}, {NOTE_C5, T_MY_8}, {NOTE_B4, T_MY_8}, {NOTE_C5, T_MY_8}, {NOTE_D5, T_MY_8}, {NOTE_C5, T_MY_8},
    {NOTE_B4, T_MY_4}, {NOTE_G4, T_MY_4}, {NOTE_G4, T_MY_2},

    // ====== Verse 1 (主歌: 天色一黑...) ======
    // 6 6 6 5 1 3 | 3 3 3 - 0 1 2
    {NOTE_A4, T_MY_8}, {NOTE_A4, T_MY_8}, {NOTE_A4, T_MY_8}, {NOTE_G4, T_MY_8}, {NOTE_C5, T_MY_4}, {NOTE_E5, T_MY_4},
    {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_4}, {0, T_MY_8}, {NOTE_C5, T_MY_8}, {NOTE_D5, T_MY_8},

    // 3 3 3 3 #5 7 1. | 7 6 6 0 6 7
    {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_GS5, T_MY_4}, {NOTE_B5, T_MY_8}, {NOTE_C6, T_MY_8},
    {NOTE_B5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_A5, T_MY_4}, {0, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_B5, T_MY_8},

    // 1. 1. 6 7 1. 1. 0 3 | 5 4 3 2 2 0 1 2
    {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {0, T_MY_8}, {NOTE_E5, T_MY_8},
    {NOTE_G5, T_MY_8}, {NOTE_F5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_D5, T_MY_8}, {NOTE_D5, T_MY_4}, {0, T_MY_8}, {NOTE_C5, T_MY_8}, {NOTE_D5, T_MY_8},

    // 3 6 7 1. 1. 2. | 7 6 5. 5 0 3
    {NOTE_E5, T_MY_4}, {NOTE_A5, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_C6, T_MY_4}, {NOTE_C6, T_MY_8}, {NOTE_D6, T_MY_8},
    {NOTE_B5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_G5, T_MY_8_D}, {NOTE_G5, T_MY_16}, {0, T_MY_8}, {NOTE_E5, T_MY_8},

    // 6 6 6 6 5 1 1 2 | 4. 3 3 0 1 2
    {NOTE_A5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_G5, T_MY_8}, {NOTE_C5, T_MY_8}, {NOTE_C5, T_MY_8}, {NOTE_D5, T_MY_8},
    {NOTE_F5, T_MY_4_D}, {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_4}, {0, T_MY_8}, {NOTE_C5, T_MY_8}, {NOTE_D5, T_MY_8},

    // ====== Pre-Chorus (过渡: 现实是现实...) ======
    // 3 3 3 3 3 6 7 | 2. 1. 7 6 6 0 6 7
    {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_B5, T_MY_8},
    {NOTE_D6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_G5, T_MY_8}, {0, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_B5, T_MY_8},

    // 1. 1. 1. 3. 2. 1. 7 1. | 1. - - -
    {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_E6, T_MY_8}, {NOTE_D6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_C6, T_MY_8},
    {NOTE_C6, T_MY_2}, {NOTE_C6, T_MY_2},

    // ====== Chorus (副歌: Sayonara Oh!) ======
    // 1. 1. 3. 2. 0 5 | 5 5 5 2. 1. 0 4
    {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_E6, T_MY_8}, {NOTE_D6, T_MY_8}, {0, T_MY_8}, {NOTE_G5, T_MY_8},
    {NOTE_G5, T_MY_8}, {NOTE_G5, T_MY_8}, {NOTE_G5, T_MY_8}, {NOTE_D6, T_MY_8}, {NOTE_C6, T_MY_8}, {0, T_MY_8}, {NOTE_F5, T_MY_8},

    // 4 4 4 1. 7 6 5 | 2. . 3. 2. -
    {NOTE_F5, T_MY_8}, {NOTE_F5, T_MY_8}, {NOTE_F5, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_G5, T_MY_8},
    {NOTE_D6, T_MY_4_D}, {NOTE_E6, T_MY_8}, {NOTE_D6, T_MY_4}, {0, T_MY_8},

    // 6 #5 6 7 3 | 1. 7 1. 3. 0 6
    {NOTE_A5, T_MY_8}, {NOTE_GS5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_E5, T_MY_4},
    {NOTE_C6, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_E6, T_MY_8}, {0, T_MY_8}, {NOTE_A5, T_MY_8},

    // 1. 1. 1. 6 1. 1. 0 6
    {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {0, T_MY_8}, {NOTE_A5, T_MY_8},

    // ====== Post-Chorus (才现代，我没法隐瞒...) ======
    // 3. 4. 3. 1. 1. 2. | 1. 1. 1. 3. 2. 0 5
    {NOTE_E6, T_MY_8}, {NOTE_F6, T_MY_8}, {NOTE_E6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_D6, T_MY_8},
    {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_E6, T_MY_8}, {NOTE_D6, T_MY_8}, {0, T_MY_8}, {NOTE_G5, T_MY_8},

    // 5 5 5 2. 1. 0 4 | 4 4 4 1. 7 6 5
    {NOTE_G5, T_MY_8}, {NOTE_G5, T_MY_8}, {NOTE_G5, T_MY_8}, {NOTE_D6, T_MY_8}, {NOTE_C6, T_MY_8}, {0, T_MY_8}, {NOTE_F5, T_MY_8},
    {NOTE_F5, T_MY_8}, {NOTE_F5, T_MY_8}, {NOTE_F5, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_G5, T_MY_8},

    // 2. . 3. 2. - | 6 #5 6 7 3 3 | 1. 1. 7 1. 3. 0 6
    {NOTE_D6, T_MY_4_D}, {NOTE_E6, T_MY_8}, {NOTE_D6, T_MY_4}, {0, T_MY_4},
    {NOTE_A5, T_MY_8}, {NOTE_GS5, T_MY_8}, {NOTE_A5, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_E5, T_MY_4}, {NOTE_E5, T_MY_4},
    {NOTE_C6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_E6, T_MY_8}, {0, T_MY_8}, {NOTE_A5, T_MY_8},

    // 3. 4. 3. 4. 3. 1. 2. 1. | 1. - - -
    {NOTE_E6, T_MY_8}, {NOTE_F6, T_MY_8}, {NOTE_E6, T_MY_8}, {NOTE_F6, T_MY_8}, {NOTE_E6, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_D6, T_MY_8}, {NOTE_C6, T_MY_8},
    {NOTE_C6, T_MY_2}, {NOTE_C6, T_MY_2},

    // ====== Outro (尾声) ======
    // 3 3 3 4 | 5 7 1. 1. | 0 1. 7 1. 7 1. 2. 5
    {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_E5, T_MY_8}, {NOTE_F5, T_MY_8},
    {NOTE_G5, T_MY_4}, {NOTE_B5, T_MY_4}, {NOTE_C6, T_MY_4}, {NOTE_C6, T_MY_4},
    {0, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_B5, T_MY_8}, {NOTE_C6, T_MY_8}, {NOTE_D6, T_MY_8}, {NOTE_G5, T_MY_8},

    // 5 - - - | 3. 3. 4. 3. 4. | 5. 7. 1.. 1..
    {NOTE_G5, T_MY_2}, {NOTE_G5, T_MY_2},
    {NOTE_E6, T_MY_8}, {NOTE_E6, T_MY_8}, {NOTE_F6, T_MY_8}, {NOTE_E6, T_MY_8}, {NOTE_F6, T_MY_8}, {0, T_MY_8}, // 简化
    {NOTE_G6, T_MY_4}, {NOTE_B6, T_MY_4}, {NOTE_C7, T_MY_2},

    // 结束
    {0, T_MY_4}
};

const uint16_t MyBeloved_Len = sizeof(MyBeloved) / sizeof(MyBeloved[0]);