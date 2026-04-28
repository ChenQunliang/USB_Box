#include "tv5725_callbacks.h"
#include "tv5725.h"

/* Forward-declared to avoid duplicate symbol with main.c */
extern const uint8_t preset_720p[];
extern const uint8_t preset_1080p[];

/* ====================================================================
   Input source callbacks
   Each maps a menu item to a physical input via ASW + input mode.
   ==================================================================== */

void cb_input_rgbs(xpMenu Menu)
{
    (void)Menu;
    tv5725_input_set_mode(TV5725_INPUT_RGBS);
}

void cb_input_rgsb(xpMenu Menu)
{
    (void)Menu;
    tv5725_input_set_mode(TV5725_INPUT_RGSB);
}

void cb_input_yuv(xpMenu Menu)
{
    (void)Menu;
    tv5725_input_set_mode(TV5725_INPUT_YUV);
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
   Each switches the output preset. 720p and 1080p share a 108 MHz
   pixel clock (s0_41=0x85); 480p uses 81 MHz (s0_41=0x65).
   4:3 vs 16:9 variants use the same preset for now; separate presets
   can be added when the VDS timing differences are needed.
   input_is_yuv: 0 = RGBS input path, 1 = YUV input path.
   ==================================================================== */

void cb_res_720p_4_3(xpMenu Menu)
{
    (void)Menu;
    //    tv5725_output_path_init(preset_720p, 0);
}

void cb_res_720p_16_9(xpMenu Menu)
{
    (void)Menu;
    //    tv5725_output_path_init(preset_720p, 0);
}

void cb_res_1080p_4_3(xpMenu Menu)
{
    (void)Menu;
    //    tv5725_output_path_init(preset_1080p, 0);
}

void cb_res_1080p_16_9(xpMenu Menu)
{
    (void)Menu;
    //    tv5725_output_path_init(preset_1080p, 0);
}
