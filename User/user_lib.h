/**
 * @file user_lib.h
 * @brief 基本函数支持
 * ----------------------------------------------------------------------------
 * @version 1.0.0.0
 * @author RM
 * @date 2018-12-26
 * @remark 官步初始代码
 * ----------------------------------------------------------------------------
 * @version 1.0.0.1
 * @author 周明杨
 * @date 2024-12-30
 * @remark 优化整体架构
 */

#ifndef USER_LIB_H
#define USER_LIB_H
#include "struct_typedef.h"
#include <stdint.h>

// 兼容不同编译器：Keil/ARMCC 将 __packed 作为关键字；GCC/Clang 需要定义为 packed attribute
#if !defined(__CC_ARM) && !defined(__ARMCC_VERSION)
#ifndef __packed
#if defined(__GNUC__) || defined(__clang__)
#define __packed __attribute__((packed))
#else
#define __packed
#endif
#endif
#endif

#ifndef PI
#define PI 3.14159265358979323846f
#endif

typedef struct
{
    fp32 input;        // 输入数据
    fp32 out;          // 输出数据
    fp32 min_value;    // 限幅最小值
    fp32 max_value;    // 限幅最大值
    fp32 frame_period; // 时间间隔
} ramp_function_source_t;

typedef struct
{
    fp32 input;        // 输入数据
    fp32 out;          // 滤波输出的数据
    fp32 num[1];       // 滤波参数
    fp32 frame_period; // 滤波的时间间隔，单位s
} first_order_filter_type_t;

// 快速开方
extern fp32 invSqrt(fp32 num);

// 斜波函数初始化
void ramp_init(ramp_function_source_t *ramp_source_type, fp32 frame_period, fp32 max, fp32 min);

// 斜波函数计算
void ramp_calc(ramp_function_source_t *ramp_source_type, fp32 input);

// 一阶滤波初始化
extern void first_order_filter_init(first_order_filter_type_t *first_order_filter_type, fp32 frame_period, const fp32 num[1]);

// 一阶滤波计算
extern void first_order_filter_cali(first_order_filter_type_t *first_order_filter_type, fp32 input);

// 绝对限制
extern void abs_limit(fp32 *num, fp32 Limit);

// 判断符号位
extern fp32 sign(fp32 value);

// 浮点死区
extern fp32 fp32_deadline(fp32 Value, fp32 minValue, fp32 maxValue);

// int16死区
extern int16_t int16_deadline(int16_t Value, int16_t minValue, int16_t maxValue);

// fp32限幅函数
extern fp32 fp32_constrain(fp32 Value, fp32 minValue, fp32 maxValue);

// int16限幅函数
extern int16_t int16_constrain(int16_t Value, int16_t minValue, int16_t maxValue);

// 循环限幅函数
extern fp32 loop_fp32_constrain(fp32 Input, fp32 minValue, fp32 maxValue);

// 角度限幅函数
extern fp32 theta_format(fp32 Ang);

// 弧度限幅函数
#define rad_format(Ang) loop_fp32_constrain((Ang), -PI, PI)

#endif