#include "tv5725_callbacks.h"
#include "tv5725.h"

/* 预设声明（定义在 preset_*.h 中） */
extern const uint8_t preset_480p[];
extern const uint8_t preset_720p[];
extern const uint8_t preset_1080p[];

/* ====================================================================
   当前状态跟踪
   ==================================================================== */
static const uint8_t *s_cur_preset = preset_480p; /* 当前输出分辨率预设 */
static uint8_t        s_input_is_yuv = 0;          /* 1 = YUV 输入，0 = RGB */

/* ====================================================================
   输出路径重载（保持当前分辨率和输入模式）
   ==================================================================== */
static void reload_output(void)
{
    tv5725_output_path_init(s_cur_preset, s_input_is_yuv);
}

/* ====================================================================
   Input source callbacks

   切换输入模式后重载输出路径，使矩阵/同步等配置与新模式匹配。
   ==================================================================== */

void cb_input_rgbs(xpMenu Menu)
{
    (void)Menu;
    s_input_is_yuv = 0;
    reload_output();                       /* 先加载预设 + 配置输出 */
    tv5725_input_set_mode(TV5725_INPUT_RGBS);  /* 再覆盖输入寄存器 */
}

void cb_input_rgsb(xpMenu Menu)
{
    (void)Menu;
    s_input_is_yuv = 0;
    reload_output();
    tv5725_input_set_mode(TV5725_INPUT_RGSB);
}

void cb_input_vga(xpMenu Menu)
{
    (void)Menu;
    s_input_is_yuv = 0;
    reload_output();
    tv5725_input_set_mode(TV5725_INPUT_VGA);
}

/* ====================================================================
   SOG mode callbacks
   ==================================================================== */

void cb_sog_normal(xpMenu Menu)
{
    (void)Menu;
    tv5725_reg_write(TV5725_RW_SYNC_PROC_48, 0x00);
    uint32_t val = tv5725_reg_read(TV5725_RW_SYNC_PROC_48);
    printf("SOG Mode: Normal (0x%02X)\n", val);
}

void cb_sog_force(xpMenu Menu)
{
    (void)Menu;
    tv5725_reg_write(TV5725_RW_PIP_V_SP, 0x123);
    uint32_t val = tv5725_reg_read(TV5725_RW_PIP_V_SP);
    printf("TV5725_RW_PIP_V_SP: Force (0x%03X)\n", val);
}

void cb_sog_show(xpMenu Menu)
{
    (void)Menu;
    uint32_t val = tv5725_reg_read(TV5725_RW_SYNC_PROC_48);
    printf("SOG Mode = 0x%02X\n", val);
}

/* ====================================================================
   Chip ID callbacks
   ==================================================================== */

void cb_chip_id_show(xpMenu Menu)
{
    (void)Menu;
    uint32_t id = tv5725_get_chip_id();
    printf("TV5725 Chip ID: 0x%06lX\n", id);
}

/* ====================================================================
   Output resolution callbacks

   切换输出分辨率预设并重新初始化输出路径。
   移植自 gbs-control 的 applyPresets() 逻辑：
   - 选择对应的预设数组写入 TV5725
   - 调用 output_path_init 配置 DAC/同步输出/矩阵
   ==================================================================== */

void cb_res_480p(xpMenu Menu)
{
    (void)Menu;
    s_cur_preset = preset_480p;
    printf("Output: 480p\n");
    reload_output();
}

void cb_res_720p(xpMenu Menu)
{
    (void)Menu;
    s_cur_preset = preset_720p;
    printf("Output: 720p\n");
    reload_output();
}

void cb_res_1080p(xpMenu Menu)
{
    (void)Menu;
    s_cur_preset = preset_1080p;
    printf("Output: 1080p\n");
    reload_output();
}
