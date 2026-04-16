/*
 * bsp_music.h
 * 描述：蜂鸣器音乐播放模块的头文件
 */

#ifndef __BSP_MUSIC_H__
#define __BSP_MUSIC_H__

// 包含 main.h 以获取 STM32 的基础定义 (如 uint16_t, HAL库等)
#include "main.h"

/* --- 1. 音符频率定义 (扩充版) --- */
// 低音区 (数字下面有点)
#define NOTE_G3  196   // 低音 So
#define NOTE_A3  220   // 低音 La
#define NOTE_B3  247   // 低音 Si
#define NOTE_C4  262   // 中央C (Do)
#define NOTE_D4  294   // Re
#define NOTE_E4  330   // Mi
#define NOTE_F4  349   // Fa
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494

// 中音区 (数字本身)
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988

// 高音区 (数字上面有点)
#define NOTE_C6  1047
#define NOTE_D6  1175
#define NOTE_E6  1318
#define NOTE_F6  1397
#define NOTE_FS6  1480  // F#6 (升Fa) <--- 你点的
#define NOTE_G6   1568
#define NOTE_GS6  1661  // G#6 (升So)
#define NOTE_A6   1760
#define NOTE_AS6  1865  // A#6 (升La)
#define NOTE_B6   1976  // <--- 你点的

/* --- 第7组 (超高音 Super High Octave) --- */
// 这里的频率很高，普通无源蜂鸣器声音会变小或变得刺耳，属于正常现象
#define NOTE_C7   2093
#define NOTE_CS7  2217  // C#7
#define NOTE_D7   2349  // <--- 你点的
#define NOTE_DS7  2489  // D#7
#define NOTE_E7   2637  // <--- 你点的
#define NOTE_F7   2794  
#define NOTE_FS7  2960
#define NOTE_G7   3136

/* --- 2. 节奏定义 (基于 6/8 拍) --- */
// 设定基准 BPM = 97 (谱面速度)
// 在 6/8 拍中，我们以十六分音符为最小单位来拼接
#define T_BASE   150             // 基础单位 (十六分音符)

#define T_16     (T_BASE)        // 十六分音符 (双下划线)
#define T_8      (T_BASE * 2)    // 八分音符 (单下划线)
#define T_8_DOT  (T_BASE * 3)    // 附点八分
#define T_4      (T_BASE * 4)    // 四分音符 (无下划线)
#define T_4_DOT  (T_BASE * 6)    // 附点四分 (一拍)

/* --- 专属节奏定义: Fuwa Fuwa Time (BPM 180) --- */
// 1拍 = 60秒 / 180 = 333ms
#define T_FW_4    333           // 四分音符 (1拍)
#define T_FW_8    167           // 八分音符 (0.5拍) - 谱子里的单下划线
#define T_FW_16   83            // 十六分音符 (0.25拍) - 谱子里的双下划线
#define T_FW_8_D  250           // 附点八分 (0.75拍)
#define T_FW_4_D  500           // 附点四分 (1.5拍)
#define T_FW_2    666           // 二分音符 (2拍)

/* --- 专属节奏: 蜜雪冰城 (BPM 115) --- */
#define T_MX_BASE 130           // 十六分音符 (最小单位)

#define T_MX_16   (T_MX_BASE)   // 十六分 (双下划线)
#define T_MX_8    (T_MX_BASE*2) // 八分 (单下划线)
#define T_MX_8_D  (T_MX_BASE*3) // 附点八分 (主要用于"5. 6"那个切分)
#define T_MX_4    (T_MX_BASE*4) // 四分 (一拍)
#define T_MX_2    (T_MX_BASE*8) // 二分 (两拍，长音)

/* --- 补充半音定义 (Bad Apple 需要用到 #5) --- */
#define NOTE_GS4  415   // G#4 (升So，低音)
#define NOTE_GS5  831   // G#5 (升So，中音)

/* --- 专属节奏: Bad Apple (BPM 140) --- */
// 1拍 = 60/140 = 0.428s = 428ms
#define T_BA_BASE 107            // 十六分音符 (基准单位)

#define T_BA_16   (T_BA_BASE)    // 十六分音符 (0.25拍)
#define T_BA_8    (T_BA_BASE*2)  // 八分音符 (0.5拍) - 谱面主力
#define T_BA_4    (T_BA_BASE*4)  // 四分音符 (1拍)
#define T_BA_2    (T_BA_BASE*8)  // 二分音符 (2拍)

/* --- 补充半音定义 (White Album 需要 A#) --- */
// 之前定义了 G# (415), 这里补充 A#
#define NOTE_AS4  466   // A#4 (升La / 降Si)
#define NOTE_AS5  932   // A#5

/* --- 专属节奏: White Album (BPM 100) --- */
// 1拍 = 60/100 = 0.6s = 600ms
#define T_WA_BASE 150           // 十六分音符

#define T_WA_16   (T_WA_BASE)   // 十六分音符 (双下划线)
#define T_WA_8    (T_WA_BASE*2) // 八分音符 (单下划线)
#define T_WA_8_D  (T_WA_BASE*3) // 附点八分
#define T_WA_4    (T_WA_BASE*4) // 四分音符 (一拍)
#define T_WA_4_D  (T_WA_BASE*6) // 附点四分
#define T_WA_2    (T_WA_BASE*8) // 二分音符
#define T_WA_1    (T_WA_BASE*16)// 全音符

/* --- 补充 Eb 大调需要的半音 --- */
#define NOTE_DS4  311   // D#4 / Eb4 (低音1)
#define NOTE_GS4  415   // G#4 / Ab4 (低音4)
#define NOTE_AS4  466   // A#4 / Bb4 (低音5)
#define NOTE_DS5  622   // D#5 / Eb5 (中音1)
#define NOTE_GS5  831   // G#5 / Ab5 (中音4)
#define NOTE_AS5  932   // A#5 / Bb5 (中音5)
#define NOTE_DS6  1245  // D#6 / Eb6 (高音1)
#define NOTE_GS6  1661  // G#6 / Ab6 (高音4)
#define NOTE_AS6  1865  // A#6 / Bb6 (高音5)
#define NOTE_DS7  2489  // D#7 / Eb7 (超高音1)

/* --- 专属节奏: 千恋万花 OP (BPM 144) --- */
// 1拍 = 60/144 = 0.416s = 416ms
// 为了稍微紧凑一点，稍微缩短一点点延时感
#define T_KL_BASE 104            // 十六分音符

#define T_KL_16   (T_KL_BASE)    // 十六分
#define T_KL_8    (T_KL_BASE*2)  // 八分
#define T_KL_8_D  (T_KL_BASE*3)  // 附点八分
#define T_KL_4    (T_KL_BASE*4)  // 四分 (一拍)
#define T_KL_4_D  (T_KL_BASE*6)  // 附点四分
#define T_KL_2    (T_KL_BASE*8)  // 二分
#define T_KL_2_D  (T_KL_BASE*12) // 附点二分

/* --- 专属节奏: 我的亲爱 (BPM 114) --- */
// 1拍 = 60/114 = 0.526s = 526ms
#define T_MY_BASE 131           // 十六分音符

#define T_MY_16   (T_MY_BASE)   // 十六分
#define T_MY_8    (T_MY_BASE*2) // 八分
#define T_MY_8_D  (T_MY_BASE*3) // 附点八分
#define T_MY_4    (T_MY_BASE*4) // 四分 (一拍)
#define T_MY_4_D  (T_MY_BASE*6) // 附点四分
#define T_MY_2    (T_MY_BASE*8) // 二分
#define T_MY_1    (T_MY_BASE*16)// 全音符

/* --- 3. 数据结构定义 --- */
// 乐谱中的一个音符
typedef struct {
    uint16_t frequency; // 频率 (Hz)
    uint16_t duration;  // 持续时间 (ms)
} Note;

/* --- 4. 外部变量声明 --- */
// 告诉编译器，这首歌的数据在 .c 文件里定义了，大家都能用
extern const Note Haruhikage_Full[]; // 完整歌曲数据
extern const uint16_t Haruhikage_Full_Len; // 歌曲长度

extern const Note FuwaFuwaTime[];
extern const uint16_t FuwaFuwaTime_Len;

extern const Note Mixue_Theme[];
extern const uint16_t Mixue_Theme_Len;

extern const Note BadApple[];
extern const uint16_t BadApple_Len;

extern const Note WhiteAlbum[];
extern const uint16_t WhiteAlbum_Len;

extern const Note SenrenBanka[];
extern const uint16_t SenrenBanka_Len;

extern const Note MyBeloved[];
extern const uint16_t MyBeloved_Len;

/* --- 5. 函数声明 --- */
// 这里列出外部可以调用的函数
extern void Play_Song(const Note* song, uint16_t length, uint8_t volume);

#endif /* __BSP_MUSIC_H__ */