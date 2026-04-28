#include "tv5725_callbacks.h"
#include "tv5725.h"

/* Forward-declared to avoid duplicate symbol with main.c */
extern const uint8_t preset_720p[];
extern const uint8_t preset_1080p[];

/* ====================================================================
   Input source callbacks
   Each maps a menu item to a physical input via ASW + input mode.
   ASW pins (PB12-PB15) route the analog mux to select the active
   input connector. The mux output feeds the TV5725 ADC.
   ==================================================================== */

void cb_input_sv(xpMenu Menu)
{
    (void)Menu;
    /* S-Video → YUV mode via ASW */
    //    tv5725_input_set_mode(TV5725_INPUT_YUV);
    /* TODO: set ASW pins for S-Video connector routing */
}

void cb_input_cvbs(xpMenu Menu)
{
    (void)Menu;
    /* Composite → YUV mode via ASW */
    //    tv5725_input_set_mode(TV5725_INPUT_YUV);
    /* TODO: set ASW pins for CVBS connector routing */
}

void cb_input_rgb0(xpMenu Menu)
{
    (void)Menu;
    /* RGB input 0 → RGBS mode */
    //    tv5725_input_set_mode(TV5725_INPUT_RGBS);
    /* Current ASW defaults route RGB0 (ASW01=H, others=L) */
}

void cb_input_rgb1(xpMenu Menu)
{
    (void)Menu;
    /* RGB input 1 → RGBS mode */
    //    tv5725_input_set_mode(TV5725_INPUT_RGBS);
    /* TODO: set ASW pins for RGB1 connector routing */
}

void cb_input_yuv(xpMenu Menu)
{
    (void)Menu;
    /* YPbPr component → YUV mode */
    //    tv5725_input_set_mode(TV5725_INPUT_YUV);
    /* Current ASW defaults route YUV (all ASW pins low) */
}

void cb_input_vga(xpMenu Menu)
{
    (void)Menu;
    /* VGA → RGBS mode (RGBHV, external H/V sync) */
    //    tv5725_input_set_mode(TV5725_INPUT_RGBS);
    /* TODO: set ASW pins for VGA connector routing */
}

/* ====================================================================
   SOG mode callbacks
   ==================================================================== */

void cb_sog_normal(xpMenu Menu)
{
    (void)Menu;
    tv5725_reg_write(TV5725_RW_SYNC_PROC_48, 0x00);
    uint32_t val = tv5725_reg_read(TV5725_RW_SYNC_PROC_48);
    printf("SOG Mode: Normal (0x%01lX)\n", val);
}

void cb_sog_force(xpMenu Menu)
{
    (void)Menu;
    tv5725_reg_write(TV5725_RW_MODE_DET_00, 0x01);
    uint32_t val = tv5725_reg_read(TV5725_RW_MODE_DET_00);
    printf("TV5725_RFF_ENABLE: Force (0x%01lX)\n", val);
}
void cb_sog_show(xpMenu Menu)
{
    (void)Menu;
    uint32_t val = tv5725_reg_read(TV5725_RW_SYNC_PROC_48);
    printf("SOG Mode = 0x%01lX\n", val);
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
