#include "cmsis_os2.h"
#include "usart.h"
#include "chassis_task.h"
#include "position_task.h"
#include "gm65.h"
#include "OLED.h"
#include "visual_identity.h"
#include <stdio.h>


#define BOOT_LOGO_CENTER_X 69
#define BOOT_LOGO_TOP_Y 4
#define BOOT_LOGO_WIDTH 28
#define BOOT_LOGO_HEIGHT 18
#define BOOT_MICROSOFT_X 37
#define BOOT_MICROSOFT_Y 24
#define BOOT_WINDOWS_X 24
#define BOOT_WINDOWS_Y 31
#define BOOT_XP_X 82
#define BOOT_XP_Y 33
#define BOOT_PROGRESS_X 42
#define BOOT_PROGRESS_Y 49
#define BOOT_PROGRESS_WIDTH 44
#define BOOT_PROGRESS_HEIGHT 7
#define BOOT_COPYRIGHT_X 0
#define BOOT_COPYRIGHT_Y 56
#define BOOT_BRAND_X 74
#define BOOT_BRAND_Y 56

static char msg[64];
static void chassis_data_send(UART_HandleTypeDef *huart); // 发送底盘数据到上位机的函数声明，传入UART句柄
static void output_boot_animation(void);
static void output_draw_xp_boot_frame(uint8_t frame);
static void output_draw_status_page(void);
static void output_draw_task_code_line(void);
static void oled_draw_windows_logo(int16_t center_x, int16_t top_y, uint8_t width, uint8_t height);
static void oled_draw_xp_progress_bar(uint8_t frame);
static void oled_fill_slanted_box(int16_t x, int16_t y, uint8_t width, uint8_t height, int8_t slant);

void output_task(void *argument)
{
    osDelay(150);
    // output_boot_animation();

    for (;;)
    {
        //chassis_data_send(&huart2); // 传入UART2句柄，发送底盘数据到上位机

        output_draw_status_page();
        osDelay(100);
    }
}

static void output_boot_animation(void)
{
    uint8_t frame;

    for (frame = 0U; frame < 40U; frame++)
    {
        output_draw_xp_boot_frame(frame);
        OLED_Update();
        osDelay(85);
    }

    output_draw_xp_boot_frame(12U);
    OLED_Update();
}

static void output_draw_status_page(void)
{
    static char odom_text[] = "ODOM";
    static char mode_text[] = "M:";
    static char arrive_text[] = "A:";
    static char x_text[] = "X:";
    static char y_text[] = "Y:";
    static char yaw_text[] = "Yaw:";
    static char visual_x_text[] = "CX:";
    static char visual_y_text[] = "CY:";
    static char error_x_text[] = "EX:";
    static char error_y_text[] = "EY:";
    static char wz_text[] = "Wz:";
    static char target_yaw_text[] = "TYaw:";
    static char rad_text[] = "rad";
    static char rad_per_s_text[] = "rad/s";
    static char wait_text[] = "ODOM DATA WAIT";
    const chassis_odometry_t *odom = get_position_data();
    const chassis_move_t *chassis = get_chassis_move_data();

    OLED_Clear();

    if (odom == NULL || chassis == NULL)
    {
        OLED_ShowString(18, 28, wait_text, OLED_6X8);
        OLED_Update();
        return;
    }

    OLED_ShowString(0, 0, odom_text, OLED_6X8);
    OLED_ShowString(36, 0, mode_text, OLED_6X8);
    OLED_ShowNum(48, 0, (uint32_t)odom->mode, 1, OLED_6X8);
    OLED_ShowString(66, 0, arrive_text, OLED_6X8);
    OLED_ShowNum(78, 0, (uint32_t)odom->arrive_flag, 1, OLED_6X8);

    OLED_ShowString(0, 8, x_text, OLED_6X8);
    OLED_ShowFloatNum(12, 8, odom->position_x, 2, 3, OLED_6X8);
    OLED_ShowString(64, 8, y_text, OLED_6X8);
    OLED_ShowFloatNum(76, 8, odom->position_y, 2, 3, OLED_6X8);

    OLED_ShowString(0, 16, yaw_text, OLED_6X8);
    OLED_ShowFloatNum(24, 16, odom->yaw, 2, 3, OLED_6X8);
    OLED_ShowString(72, 16, rad_text, OLED_6X8);

    OLED_ShowString(0, 24, visual_x_text, OLED_6X8);
    OLED_ShowSignedNum(18, 24, g_maxicam_info.x_pos, 4, OLED_6X8);
    OLED_ShowString(64, 24, visual_y_text, OLED_6X8);
    OLED_ShowSignedNum(82, 24, g_maxicam_info.y_pos, 4, OLED_6X8);

    OLED_ShowString(0, 32, wz_text, OLED_6X8);
    OLED_ShowFloatNum(18, 32, chassis->wz, 1, 3, OLED_6X8);
    OLED_ShowString(60, 32, rad_per_s_text, OLED_6X8);

    output_draw_task_code_line();

    OLED_ShowString(0, 48, error_x_text, OLED_6X8);
    OLED_ShowSignedNum(18, 48, g_maxicam_info.error_x, 4, OLED_6X8);
    OLED_ShowString(64, 48, error_y_text, OLED_6X8);
    OLED_ShowSignedNum(82, 48, g_maxicam_info.error_y, 4, OLED_6X8);

    OLED_ShowString(0, 56, target_yaw_text, OLED_6X8);
    OLED_ShowFloatNum(30, 56, odom->yaw_set, 2, 3, OLED_6X8);
    OLED_ShowString(78, 56, rad_text, OLED_6X8);

    OLED_Update();
}

static void output_draw_task_code_line(void)
{
    static char task_text[] = "TASK:";
    char task_code[8];
    uint8_t index;

    for (index = 0U; index < 7U; index++)
    {
        if (color_task[index] == 0U)
        {
            task_code[index] = '|';
        }
        else if ((color_task[index] >= '0') && (color_task[index] <= '9'))
        {
            task_code[index] = (char)color_task[index];
        }
        else if ((color_task[index] >= 1U) && (color_task[index] <= 9U))
        {
            task_code[index] = (char)('0' + color_task[index]);
        }
        else
        {
            task_code[index] = '+';
        }
    }
    task_code[7] = '\0';

    OLED_ShowString(0, 40, task_text, OLED_6X8);
    OLED_ShowString(36, 40, task_code, OLED_6X8);
}

static void output_draw_xp_boot_frame(uint8_t frame)
{
    static char microsoft_text[] = "Microsoft";
    static char windows_text[] = "Windows";
    static char xp_text[] = "xp";
    static char copyright_text[] = "Copyright";
    static char brand_text[] = "Microsoft";

    OLED_Clear();
    oled_draw_windows_logo(BOOT_LOGO_CENTER_X, BOOT_LOGO_TOP_Y, BOOT_LOGO_WIDTH, BOOT_LOGO_HEIGHT);
    OLED_ShowString(BOOT_MICROSOFT_X, BOOT_MICROSOFT_Y, microsoft_text, OLED_6X8);
    OLED_ShowString(BOOT_WINDOWS_X, BOOT_WINDOWS_Y, windows_text, OLED_8X16);
    OLED_ShowString(BOOT_XP_X, BOOT_XP_Y, xp_text, OLED_6X8);
    oled_draw_xp_progress_bar(frame);
    OLED_ShowString(BOOT_COPYRIGHT_X, BOOT_COPYRIGHT_Y, copyright_text, OLED_6X8);
    OLED_ShowString(BOOT_BRAND_X, BOOT_BRAND_Y, brand_text, OLED_6X8);
}

static void oled_draw_windows_logo(int16_t center_x, int16_t top_y, uint8_t width, uint8_t height)
{
    uint8_t gap = 3U;
    uint8_t skew = 4U;
    uint8_t pane_width = (uint8_t)((width - gap - skew) / 2U);
    uint8_t pane_height = (uint8_t)((height - gap) / 2U);
    int16_t left_x = center_x - (int16_t)width / 2;
    int16_t right_x = left_x + pane_width + gap;
    int16_t lower_y = top_y + pane_height + gap;

    oled_fill_slanted_box(left_x, top_y, pane_width, pane_height, 3);
    oled_fill_slanted_box(right_x, top_y, pane_width, pane_height, 2);
    oled_fill_slanted_box(left_x + 1, lower_y, pane_width, pane_height, 2);
    oled_fill_slanted_box(right_x + 1, lower_y, pane_width, pane_height, 3);

    OLED_DrawLine(left_x + 1, top_y, right_x + pane_width - 1, top_y + 1);
    OLED_DrawLine(left_x, top_y + 1, left_x + skew + 1, top_y + height - 2);
    OLED_DrawLine(right_x + pane_width - 2, top_y + 1, right_x + pane_width + skew - 1, top_y + height - 2);
    OLED_DrawLine(left_x + skew + 1, top_y + height - 1, right_x + pane_width + skew - 1, top_y + height - 2);
    OLED_DrawArc(center_x, top_y + pane_height, width / 3U, -150, -20, OLED_UNFILLED);
    OLED_DrawArc(center_x + 1, lower_y - 1, width / 3U, 15, 145, OLED_UNFILLED);
}

static void oled_draw_xp_progress_bar(uint8_t frame)
{
    uint8_t i;
    int16_t inner_x = BOOT_PROGRESS_X + 3;
    int16_t inner_width = BOOT_PROGRESS_WIDTH - 6;
    int16_t inner_end = inner_x + inner_width;
    int16_t group_width = 15;
    int16_t start_x = inner_x + ((int16_t)frame * 2) % (inner_width + group_width) - group_width;

    OLED_DrawRectangle(BOOT_PROGRESS_X, BOOT_PROGRESS_Y, BOOT_PROGRESS_WIDTH, BOOT_PROGRESS_HEIGHT, OLED_UNFILLED);
    OLED_DrawRectangle(BOOT_PROGRESS_X + 1,
                       BOOT_PROGRESS_Y + 1,
                       BOOT_PROGRESS_WIDTH - 2U,
                       BOOT_PROGRESS_HEIGHT - 2U,
                       OLED_UNFILLED);
    OLED_DrawLine(BOOT_PROGRESS_X + 2,
                  BOOT_PROGRESS_Y + 1,
                  BOOT_PROGRESS_X + BOOT_PROGRESS_WIDTH - 3,
                  BOOT_PROGRESS_Y + 1);

    for (i = 0U; i < 4U; i++)
    {
        int16_t segment_x = start_x + (int16_t)i * 4;
        int16_t clip_x = (segment_x > inner_x) ? segment_x : inner_x;
        int16_t clip_end = segment_x + 3;

        if (clip_end > inner_end)
        {
            clip_end = inner_end;
        }

        if (clip_end > clip_x)
        {
            OLED_DrawRectangle(clip_x,
                               BOOT_PROGRESS_Y + 2,
                               (uint8_t)(clip_end - clip_x),
                               BOOT_PROGRESS_HEIGHT - 4U,
                               OLED_FILLED);
        }
    }
}

static void oled_fill_slanted_box(int16_t x, int16_t y, uint8_t width, uint8_t height, int8_t slant)
{
    uint8_t row;

    for (row = 0U; row < height; row++)
    {
        int16_t offset = ((int16_t)slant * row) / (int16_t)height;
        OLED_DrawLine(x + offset, y + row, x + offset + width - 1, y + row);
    }
}

static void chassis_data_send(UART_HandleTypeDef *huart)
{
    if (huart == NULL)
    {
        return;
    }

    const chassis_odometry_t *odom = get_position_data();
    const chassis_move_t *chassis = get_chassis_move_data();

    if (odom == NULL || chassis == NULL)
    {
        return;
    }

    float pos_x = (float)odom->position_x;
    float pos_y = (float)odom->position_y;
    float yaw = (float)odom->yaw;
    float vx = (float)chassis->vx;
    float vy = (float)chassis->vy;
    float wz = (float)chassis->wz;

    int len = snprintf(msg, sizeof(msg),
                       "Data:%.3f,%.3f,%.3f\r\n",
                       pos_x, pos_y, yaw);
                       
    if (len > 0)
    {
        uint16_t send_len = (len < (int)sizeof(msg)) ? (uint16_t)len : (uint16_t)(sizeof(msg) - 1U);
        HAL_UART_Transmit_DMA(huart, (uint8_t *)msg, send_len);
    }
}