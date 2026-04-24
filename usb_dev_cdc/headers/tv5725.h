#ifndef TV5725_H
#define TV5725_H

#include "main.h"

/* TV5725 GBS I2C 7-bit address */
#define TV5725_I2C_ADDR       (0x17U)
/* TV5725 I2C address in 8-bit form for HC32 V_I2C API */
#define TV5725_I2C_ADDR_8BIT  ((uint16_t)(TV5725_I2C_ADDR << 1))

/* Segment select register (accessible regardless of current segment) */
#define TV5725_SEG_REG        (0xF0U)

/* Number of segments (0-5) */
#define TV5725_SEG_COUNT      (6U)

/* Register descriptor */
typedef struct {
    uint8_t segment;
    uint8_t offset;
    uint8_t bit_offset;
    uint8_t bit_width;
} tv5725_reg_t;

/* Build a register descriptor inline */
#define TV5725_REG(seg, off, bit, w)  ((tv5725_reg_t){(seg), (off), (bit), (w)})

/* ================================================================
   Segment 0x00: STATUS / MISC / PLL / DAC / RESET / PAD / OSD
   ================================================================ */

/* STATUS_IF: input video detection */
#define TV5725_STATUS_00                TV5725_REG(0x00, 0x00, 0, 8)
#define TV5725_STATUS_IF_VT_OK          TV5725_REG(0x00, 0x00, 0, 1)
#define TV5725_STATUS_IF_HT_OK          TV5725_REG(0x00, 0x00, 1, 1)
#define TV5725_STATUS_IF_HVT_OK         TV5725_REG(0x00, 0x00, 2, 1)
#define TV5725_STATUS_IF_INP_NTSC_INT   TV5725_REG(0x00, 0x00, 3, 1)
#define TV5725_STATUS_IF_INP_NTSC_PRG   TV5725_REG(0x00, 0x00, 4, 1)
#define TV5725_STATUS_IF_INP_PAL_INT    TV5725_REG(0x00, 0x00, 5, 1)
#define TV5725_STATUS_IF_INP_PAL_PRG    TV5725_REG(0x00, 0x00, 6, 1)
#define TV5725_STATUS_IF_INP_SD         TV5725_REG(0x00, 0x00, 7, 1)

#define TV5725_REG_S0_01                TV5725_REG(0x00, 0x01, 0, 8)
#define TV5725_STATUS_IF_INP_VGA60      TV5725_REG(0x00, 0x01, 0, 1)
#define TV5725_STATUS_IF_INP_VGA75      TV5725_REG(0x00, 0x01, 1, 1)
#define TV5725_STATUS_IF_INP_VGA86      TV5725_REG(0x00, 0x01, 2, 1)
#define TV5725_STATUS_IF_INP_VGA        TV5725_REG(0x00, 0x01, 3, 1)
#define TV5725_STATUS_IF_INP_SVGA60     TV5725_REG(0x00, 0x01, 4, 1)
#define TV5725_STATUS_IF_INP_SVGA75     TV5725_REG(0x00, 0x01, 5, 1)
#define TV5725_STATUS_IF_INP_SVGA85     TV5725_REG(0x00, 0x01, 6, 1)
#define TV5725_STATUS_IF_INP_SVGA       TV5725_REG(0x00, 0x01, 7, 1)

#define TV5725_REG_S0_02                TV5725_REG(0x00, 0x02, 0, 8)
#define TV5725_STATUS_IF_INP_XGA60      TV5725_REG(0x00, 0x02, 0, 1)
#define TV5725_STATUS_IF_INP_XGA70      TV5725_REG(0x00, 0x02, 1, 1)
#define TV5725_STATUS_IF_INP_XGA75      TV5725_REG(0x00, 0x02, 2, 1)
#define TV5725_STATUS_IF_INP_XGA85      TV5725_REG(0x00, 0x02, 3, 1)
#define TV5725_STATUS_IF_INP_XGA        TV5725_REG(0x00, 0x02, 4, 1)
#define TV5725_STATUS_IF_INP_SXGA60     TV5725_REG(0x00, 0x02, 5, 1)
#define TV5725_STATUS_IF_INP_SXGA75     TV5725_REG(0x00, 0x02, 6, 1)
#define TV5725_STATUS_IF_INP_SXGA85     TV5725_REG(0x00, 0x02, 7, 1)

#define TV5725_REG_S0_03                TV5725_REG(0x00, 0x03, 0, 8)
#define TV5725_STATUS_IF_INP_SXGA       TV5725_REG(0x00, 0x03, 0, 1)
#define TV5725_STATUS_IF_INP_PC         TV5725_REG(0x00, 0x03, 1, 1)
#define TV5725_STATUS_IF_INP_720P50     TV5725_REG(0x00, 0x03, 2, 1)
#define TV5725_STATUS_IF_INP_720P60     TV5725_REG(0x00, 0x03, 3, 1)
#define TV5725_STATUS_IF_INP_720        TV5725_REG(0x00, 0x03, 4, 1)
#define TV5725_STATUS_IF_INP_2200_1125I TV5725_REG(0x00, 0x03, 5, 1)
#define TV5725_STATUS_IF_INP_2376_1250I TV5725_REG(0x00, 0x03, 6, 1)
#define TV5725_STATUS_IF_INP_2640_1125I TV5725_REG(0x00, 0x03, 7, 1)

#define TV5725_REG_S0_04                TV5725_REG(0x00, 0x04, 0, 8)
#define TV5725_STATUS_IF_INP_1080I      TV5725_REG(0x00, 0x04, 0, 1)
#define TV5725_STATUS_IF_INP_2200_1125P TV5725_REG(0x00, 0x04, 1, 1)
#define TV5725_STATUS_IF_INP_2376_1250P TV5725_REG(0x00, 0x04, 2, 1)
#define TV5725_STATUS_IF_INP_2640_1125P TV5725_REG(0x00, 0x04, 3, 1)
#define TV5725_STATUS_IF_INP_1808P      TV5725_REG(0x00, 0x04, 4, 1)
#define TV5725_STATUS_IF_INP_HD         TV5725_REG(0x00, 0x04, 5, 1)
#define TV5725_STATUS_IF_INP_INT        TV5725_REG(0x00, 0x04, 6, 1)
#define TV5725_STATUS_IF_INP_PRG        TV5725_REG(0x00, 0x04, 7, 1)
#define TV5725_INTERLACE_PROGRESSIVE    TV5725_REG(0x00, 0x04, 6, 2)

#define TV5725_REG_S0_05                TV5725_REG(0x00, 0x05, 0, 8)
#define TV5725_USER_DEFINE              TV5725_REG(0x00, 0x05, 0, 1)
#define TV5725_STATUS_IF_NO_SYNC        TV5725_REG(0x00, 0x05, 1, 1)
#define TV5725_STATUS_IF_HT_BAD         TV5725_REG(0x00, 0x05, 2, 1)
#define TV5725_STATUS_IF_VT_BAD         TV5725_REG(0x00, 0x05, 3, 1)
#define TV5725_STATUS_IF_INP_SW         TV5725_REG(0x00, 0x05, 4, 1)

#define TV5725_HPERIOD_IF               TV5725_REG(0x00, 0x06, 0, 9)
#define TV5725_H_TOTAL_LOW              TV5725_REG(0x00, 0x06, 0, 8)
#define TV5725_REG_S0_06                TV5725_REG(0x00, 0x06, 0, 8)

#define TV5725_VPERIOD_IF               TV5725_REG(0x00, 0x07, 1, 11)
#define TV5725_H_TOTAL_HIGH             TV5725_REG(0x00, 0x07, 0, 1)
#define TV5725_V_TOTAL_LOW              TV5725_REG(0x00, 0x07, 1, 7)
#define TV5725_REG_S0_07                TV5725_REG(0x00, 0x07, 0, 8)

#define TV5725_V_TOTAL_HIGH             TV5725_REG(0x00, 0x08, 0, 4)
#define TV5725_REG_S0_08                TV5725_REG(0x00, 0x08, 0, 8)

/* PLL / DAC / Reset / PAD */
#define TV5725_STATUS_MISC_PLL648_LOCK  TV5725_REG(0x00, 0x09, 6, 1)
#define TV5725_STATUS_MISC_PLLAD_LOCK   TV5725_REG(0x00, 0x09, 7, 1)

#define TV5725_STATUS_MISC              TV5725_REG(0x00, 0x0A, 0, 8)
#define TV5725_STATUS_MISC_PIP_EN_V     TV5725_REG(0x00, 0x0A, 0, 1)
#define TV5725_STATUS_MISC_PIP_EN_H     TV5725_REG(0x00, 0x0A, 1, 1)
#define TV5725_STATUS_MISC_VBLK         TV5725_REG(0x00, 0x0A, 4, 1)
#define TV5725_STATUS_MISC_HBLK         TV5725_REG(0x00, 0x0A, 5, 1)
#define TV5725_STATUS_MISC_VSYNC        TV5725_REG(0x00, 0x0A, 6, 1)
#define TV5725_STATUS_MISC_HSYNC        TV5725_REG(0x00, 0x0A, 7, 1)

/* Chip ID */
#define TV5725_CHIP_ID_FOUNDRY          TV5725_REG(0x00, 0x0B, 0, 8)
#define TV5725_CHIP_ID_PRODUCT          TV5725_REG(0x00, 0x0C, 0, 8)
#define TV5725_CHIP_ID_REVISION         TV5725_REG(0x00, 0x0D, 0, 8)

/* GPIO status */
#define TV5725_STATUS_GPIO_GPIO         TV5725_REG(0x00, 0x0E, 0, 1)
#define TV5725_STATUS_GPIO_HALF         TV5725_REG(0x00, 0x0E, 1, 1)
#define TV5725_STATUS_GPIO_SCLSA        TV5725_REG(0x00, 0x0E, 2, 1)
#define TV5725_STATUS_GPIO_MBA          TV5725_REG(0x00, 0x0E, 3, 1)
#define TV5725_STATUS_GPIO_MCS1         TV5725_REG(0x00, 0x0E, 4, 1)
#define TV5725_STATUS_GPIO_HBOUT        TV5725_REG(0x00, 0x0E, 5, 1)
#define TV5725_STATUS_GPIO_VBOUT        TV5725_REG(0x00, 0x0E, 6, 1)
#define TV5725_STATUS_GPIO_CLKOUT       TV5725_REG(0x00, 0x0E, 7, 1)

/* Interrupt status */
#define TV5725_STATUS_INT_SOG_BAD       TV5725_REG(0x00, 0x0F, 0, 1)
#define TV5725_STATUS_INT_SOG_SW        TV5725_REG(0x00, 0x0F, 1, 1)
#define TV5725_STATUS_INT_SOG_OK        TV5725_REG(0x00, 0x0F, 2, 1)
#define TV5725_STATUS_INT_INP_SW        TV5725_REG(0x00, 0x0F, 3, 1)
#define TV5725_STATUS_INT_INP_NO_SYNC   TV5725_REG(0x00, 0x0F, 4, 1)
#define TV5725_STATUS_INT_INP_HSYNC     TV5725_REG(0x00, 0x0F, 5, 1)
#define TV5725_STATUS_INT_INP_VSYNC     TV5725_REG(0x00, 0x0F, 6, 1)

/* VDS status */
#define TV5725_STATUS_VDS_FR_NUM        TV5725_REG(0x00, 0x10, 0, 4)
#define TV5725_STATUS_VDS_OUT_VSYNC     TV5725_REG(0x00, 0x10, 4, 1)
#define TV5725_STATUS_VDS_OUT_HSYNC     TV5725_REG(0x00, 0x10, 5, 1)
#define TV5725_STATUS_VDS_FIELD         TV5725_REG(0x00, 0x11, 0, 1)
#define TV5725_STATUS_VDS_OUT_BLANK     TV5725_REG(0x00, 0x11, 1, 1)
#define TV5725_STATUS_VDS_VERT_COUNT    TV5725_REG(0x00, 0x11, 4, 11)

/* Memory FIFO status */
#define TV5725_STATUS_MEM_FF_WFF_FULL   TV5725_REG(0x00, 0x13, 0, 1)
#define TV5725_STATUS_MEM_FF_WFF_EMPTY  TV5725_REG(0x00, 0x13, 1, 1)
#define TV5725_STATUS_MEM_FF_RFF_FULL   TV5725_REG(0x00, 0x13, 2, 1)
#define TV5725_STATUS_MEM_FF_RFF_EMPTY  TV5725_REG(0x00, 0x13, 3, 1)
#define TV5725_STATUS_MEM_FF_CAP_FULL   TV5725_REG(0x00, 0x13, 4, 1)
#define TV5725_STATUS_MEM_FF_CAP_EMPTY  TV5725_REG(0x00, 0x13, 5, 1)
#define TV5725_STATUS_MEM_FF_PLY_FULL   TV5725_REG(0x00, 0x13, 6, 1)
#define TV5725_STATUS_MEM_FF_PLY_EMPTY  TV5725_REG(0x00, 0x13, 7, 1)
#define TV5725_STATUS_MEM_FF_EXT_FIN    TV5725_REG(0x00, 0x14, 0, 1)

/* Deinterlace status */
#define TV5725_STATUS_DEINT_PULLDN      TV5725_REG(0x00, 0x15, 7, 1)

/* Sync processor status */
#define TV5725_STATUS_SYNC_PROC_HSPOL   TV5725_REG(0x00, 0x16, 0, 1)
#define TV5725_STATUS_SYNC_PROC_HSACT   TV5725_REG(0x00, 0x16, 1, 1)
#define TV5725_STATUS_SYNC_PROC_VSPOL   TV5725_REG(0x00, 0x16, 2, 1)
#define TV5725_STATUS_SYNC_PROC_VSACT   TV5725_REG(0x00, 0x16, 3, 1)
#define TV5725_STATUS_SYNC_PROC_HTOTAL  TV5725_REG(0x00, 0x17, 0, 12)
#define TV5725_STATUS_SYNC_PROC_HLOW_LEN TV5725_REG(0x00, 0x19, 0, 12)
#define TV5725_STATUS_SYNC_PROC_VTOTAL  TV5725_REG(0x00, 0x1B, 0, 11)

/* PLL control */
#define TV5725_PLL_CKIS                 TV5725_REG(0x00, 0x40, 0, 1)
#define TV5725_PLL_DIVBY2Z              TV5725_REG(0x00, 0x40, 1, 1)
#define TV5725_PLL_IS                   TV5725_REG(0x00, 0x40, 2, 1)
#define TV5725_PLL_ADS                  TV5725_REG(0x00, 0x40, 3, 1)
#define TV5725_PLL_MS                   TV5725_REG(0x00, 0x40, 4, 3)
#define TV5725_PLL648_CONTROL_01        TV5725_REG(0x00, 0x41, 0, 8)
#define TV5725_PLL_VS                   TV5725_REG(0x00, 0x41, 0, 2)
#define TV5725_PLL_VS2                  TV5725_REG(0x00, 0x41, 2, 2)
#define TV5725_PLL_VS4                  TV5725_REG(0x00, 0x41, 4, 2)
#define TV5725_PLL_2XV                  TV5725_REG(0x00, 0x41, 6, 1)
#define TV5725_PLL_4XV                  TV5725_REG(0x00, 0x41, 7, 1)
#define TV5725_PLL648_CONTROL_03        TV5725_REG(0x00, 0x43, 0, 8)
#define TV5725_PLL_R                    TV5725_REG(0x00, 0x43, 0, 2)
#define TV5725_PLL_S                    TV5725_REG(0x00, 0x43, 2, 2)
#define TV5725_PLL_LEN                  TV5725_REG(0x00, 0x43, 4, 1)
#define TV5725_PLL_VCORST               TV5725_REG(0x00, 0x43, 5, 1)

/* DAC control */
#define TV5725_DAC_RGBS_PWDNZ           TV5725_REG(0x00, 0x44, 0, 1)
#define TV5725_DAC_RGBS_R0ENZ           TV5725_REG(0x00, 0x44, 2, 1)
#define TV5725_DAC_RGBS_G0ENZ           TV5725_REG(0x00, 0x44, 5, 1)
#define TV5725_DAC_RGBS_B0ENZ           TV5725_REG(0x00, 0x45, 0, 1)
#define TV5725_DAC_RGBS_SPD             TV5725_REG(0x00, 0x45, 2, 1)
#define TV5725_DAC_RGBS_S0ENZ           TV5725_REG(0x00, 0x45, 3, 1)
#define TV5725_DAC_RGBS_S1EN            TV5725_REG(0x00, 0x45, 4, 1)

/* Soft reset */
#define TV5725_SFTRST_IF_RSTZ           TV5725_REG(0x00, 0x46, 0, 1)
#define TV5725_SFTRST_DEINT_RSTZ        TV5725_REG(0x00, 0x46, 1, 1)
#define TV5725_SFTRST_MEM_FF_RSTZ       TV5725_REG(0x00, 0x46, 2, 1)
#define TV5725_SFTRST_FIFO_RSTZ         TV5725_REG(0x00, 0x46, 4, 1)
#define TV5725_SFTRST_VDS_RSTZ          TV5725_REG(0x00, 0x46, 6, 1)
#define TV5725_RESET_CONTROL_46         TV5725_REG(0x00, 0x46, 0, 8)
#define TV5725_SFTRST_DEC_RSTZ          TV5725_REG(0x00, 0x47, 0, 1)
#define TV5725_SFTRST_MODE_RSTZ         TV5725_REG(0x00, 0x47, 1, 1)
#define TV5725_SFTRST_SYNC_RSTZ         TV5725_REG(0x00, 0x47, 2, 1)
#define TV5725_SFTRST_HDBYPS_RSTZ       TV5725_REG(0x00, 0x47, 3, 1)
#define TV5725_SFTRST_INT_RSTZ          TV5725_REG(0x00, 0x47, 4, 1)
#define TV5725_RESET_CONTROL_47         TV5725_REG(0x00, 0x47, 0, 8)

/* PAD control */
#define TV5725_PAD_BOUT_EN              TV5725_REG(0x00, 0x48, 0, 1)
#define TV5725_PAD_SYNC1_IN_ENZ         TV5725_REG(0x00, 0x48, 6, 1)
#define TV5725_PAD_SYNC2_IN_ENZ         TV5725_REG(0x00, 0x48, 7, 1)
#define TV5725_PAD_CKIN_ENZ             TV5725_REG(0x00, 0x49, 0, 1)
#define TV5725_PAD_CKOUT_ENZ            TV5725_REG(0x00, 0x49, 1, 1)
#define TV5725_PAD_SYNC_OUT_ENZ         TV5725_REG(0x00, 0x49, 2, 1)
#define TV5725_PAD_BLK_OUT_ENZ          TV5725_REG(0x00, 0x49, 3, 1)
#define TV5725_PAD_TRI_ENZ              TV5725_REG(0x00, 0x49, 4, 1)
#define TV5725_PAD_OSC_CNTRL            TV5725_REG(0x00, 0x4A, 0, 3)
#define TV5725_DAC_RGBS_BYPS2DAC        TV5725_REG(0x00, 0x4B, 1, 1)
#define TV5725_DAC_RGBS_ADC2DAC         TV5725_REG(0x00, 0x4B, 2, 1)
#define TV5725_TEST_BUS_SEL             TV5725_REG(0x00, 0x4D, 0, 5)
#define TV5725_TEST_BUS_EN              TV5725_REG(0x00, 0x4D, 5, 1)
#define TV5725_OUT_SYNC_CNTRL           TV5725_REG(0x00, 0x4F, 5, 1)
#define TV5725_OUT_SYNC_SEL             TV5725_REG(0x00, 0x4F, 6, 2)

/* GPIO / Interrupt */
#define TV5725_GPIO_CONTROL_00          TV5725_REG(0x00, 0x52, 0, 8)
#define TV5725_GPIO_CONTROL_01          TV5725_REG(0x00, 0x53, 0, 8)
#define TV5725_INTERRUPT_CONTROL_00     TV5725_REG(0x00, 0x58, 0, 8)
#define TV5725_INTERRUPT_CONTROL_01     TV5725_REG(0x00, 0x59, 0, 8)

/* OSD registers (segment 0x00, offsets 0x90-0x98) */
#define TV5725_OSD_SW_RESET             TV5725_REG(0x00, 0x90, 0, 1)
#define TV5725_OSD_HORIZONTAL_ZOOM      TV5725_REG(0x00, 0x90, 1, 3)
#define TV5725_OSD_VERTICAL_ZOOM        TV5725_REG(0x00, 0x90, 4, 2)
#define TV5725_OSD_DISP_EN              TV5725_REG(0x00, 0x90, 6, 1)
#define TV5725_OSD_MENU_EN              TV5725_REG(0x00, 0x90, 7, 1)
#define TV5725_OSD_MENU_ICON_SEL        TV5725_REG(0x00, 0x91, 0, 4)
#define TV5725_OSD_MENU_MOD_SEL         TV5725_REG(0x00, 0x91, 4, 4)
#define TV5725_OSD_MENU_HORI_START      TV5725_REG(0x00, 0x95, 0, 8)
#define TV5725_OSD_MENU_VER_START       TV5725_REG(0x00, 0x96, 0, 8)
#define TV5725_OSD_BAR_LENGTH           TV5725_REG(0x00, 0x97, 0, 8)
#define TV5725_OSD_COMMAND_FINISH       TV5725_REG(0x00, 0x93, 7, 1)

/* ================================================================
   Segment 0x01: Input Formatter (IF) / HD Bypass / Mode Detect
   ================================================================ */
#define TV5725_IF_IN_DREG_BYPS          TV5725_REG(0x01, 0x00, 0, 1)
#define TV5725_IF_MATRIX_BYPS           TV5725_REG(0x01, 0x00, 1, 1)
#define TV5725_IF_UV_REVERT             TV5725_REG(0x01, 0x00, 2, 1)
#define TV5725_IF_SEL_656               TV5725_REG(0x01, 0x00, 3, 1)
#define TV5725_IF_SEL16BIT              TV5725_REG(0x01, 0x00, 4, 1)
#define TV5725_IF_VS_SEL                TV5725_REG(0x01, 0x00, 5, 1)
#define TV5725_IF_PRGRSV_CNTRL          TV5725_REG(0x01, 0x00, 6, 1)
#define TV5725_IF_HS_FLIP               TV5725_REG(0x01, 0x00, 7, 1)
#define TV5725_IF_VS_FLIP               TV5725_REG(0x01, 0x01, 0, 1)
#define TV5725_IF_SEL24BIT              TV5725_REG(0x01, 0x01, 7, 1)
#define TV5725_IF_SEL_WEN               TV5725_REG(0x01, 0x02, 0, 1)
#define TV5725_IF_HSYNC_RST             TV5725_REG(0x01, 0x0E, 0, 11)
#define TV5725_IF_HB_ST                 TV5725_REG(0x01, 0x10, 0, 11)
#define TV5725_IF_HB_SP                 TV5725_REG(0x01, 0x12, 0, 11)
#define TV5725_IF_VB_ST                 TV5725_REG(0x01, 0x1C, 0, 11)
#define TV5725_IF_VB_SP                 TV5725_REG(0x01, 0x1E, 0, 11)
#define TV5725_IF_LINE_ST               TV5725_REG(0x01, 0x20, 0, 12)
#define TV5725_IF_LINE_SP               TV5725_REG(0x01, 0x22, 0, 12)

/* GBS custom app registers (inside IF segment) */
#define TV5725_GBS_PRESET_ID            TV5725_REG(0x01, 0x2B, 0, 7)
#define TV5725_GBS_PRESET_CUSTOM        TV5725_REG(0x01, 0x2B, 7, 1)
#define TV5725_GBS_OPTION_SCANLINES     TV5725_REG(0x01, 0x2C, 0, 1)
#define TV5725_GBS_OPTION_SCALING_RGBHV TV5725_REG(0x01, 0x2C, 1, 1)
#define TV5725_GBS_OPTION_PALFORCED60   TV5725_REG(0x01, 0x2C, 2, 1)
#define TV5725_GBS_PRESET_DISPLAY_CLOCK TV5725_REG(0x01, 0x2D, 0, 8)

/* HD Bypass */
#define TV5725_HD_HSYNC_RST             TV5725_REG(0x01, 0x37, 0, 11)
#define TV5725_HD_INI_ST                TV5725_REG(0x01, 0x39, 0, 11)
#define TV5725_HD_HB_ST                 TV5725_REG(0x01, 0x3B, 0, 12)
#define TV5725_HD_HB_SP                 TV5725_REG(0x01, 0x3D, 0, 12)
#define TV5725_HD_HS_ST                 TV5725_REG(0x01, 0x3F, 0, 12)
#define TV5725_HD_HS_SP                 TV5725_REG(0x01, 0x41, 0, 12)
#define TV5725_HD_VB_ST                 TV5725_REG(0x01, 0x43, 0, 12)
#define TV5725_HD_VB_SP                 TV5725_REG(0x01, 0x45, 0, 12)
#define TV5725_HD_VS_ST                 TV5725_REG(0x01, 0x47, 0, 12)
#define TV5725_HD_VS_SP                 TV5725_REG(0x01, 0x49, 0, 12)

/* ================================================================
   Segment 0x02: Deinterlacer / Scaledown (MADPT)
   ================================================================ */
#define TV5725_DEINT_00                 TV5725_REG(0x02, 0x00, 0, 8)
#define TV5725_DIAG_BOB_PLDY_RAM_BYPS   TV5725_REG(0x02, 0x00, 7, 1)
#define TV5725_MADPT_Y_VSCALE_BYPS      TV5725_REG(0x02, 0x02, 6, 1)
#define TV5725_MADPT_UV_VSCALE_BYPS     TV5725_REG(0x02, 0x02, 7, 1)
#define TV5725_MADPT_Y_MI_OFFSET        TV5725_REG(0x02, 0x0B, 0, 7)
#define TV5725_MADPT_Y_DELAY            TV5725_REG(0x02, 0x17, 0, 4)
#define TV5725_MADPT_UV_DELAY           TV5725_REG(0x02, 0x17, 4, 4)
#define TV5725_MADPT_VSCALE_DEC_FACTOR  TV5725_REG(0x02, 0x31, 0, 2)

/* ================================================================
   Segment 0x03: VDS (Video Display System) / PIP / NR
   ================================================================ */
#define TV5725_VDS_SYNC_EN              TV5725_REG(0x03, 0x00, 0, 1)
#define TV5725_VDS_FIELDAB_EN           TV5725_REG(0x03, 0x00, 1, 1)
#define TV5725_VDS_DFIELD_EN            TV5725_REG(0x03, 0x00, 2, 1)
#define TV5725_VDS_FIELD_FLIP           TV5725_REG(0x03, 0x00, 3, 1)
#define TV5725_VDS_HSCALE_BYPS          TV5725_REG(0x03, 0x00, 4, 1)
#define TV5725_VDS_VSCALE_BYPS          TV5725_REG(0x03, 0x00, 5, 1)
#define TV5725_VDS_HALF_EN              TV5725_REG(0x03, 0x00, 6, 1)
#define TV5725_VDS_SRESET               TV5725_REG(0x03, 0x00, 7, 1)

#define TV5725_VDS_HSYNC_RST            TV5725_REG(0x03, 0x01, 0, 12)
#define TV5725_VDS_VSYNC_RST            TV5725_REG(0x03, 0x02, 4, 11)
#define TV5725_VDS_HB_ST                TV5725_REG(0x03, 0x04, 0, 12)
#define TV5725_VDS_HB_SP                TV5725_REG(0x03, 0x05, 4, 12)
#define TV5725_VDS_VB_ST                TV5725_REG(0x03, 0x07, 0, 11)
#define TV5725_VDS_VB_SP                TV5725_REG(0x03, 0x08, 4, 11)
#define TV5725_VDS_HS_ST                TV5725_REG(0x03, 0x0A, 0, 12)
#define TV5725_VDS_HS_SP                TV5725_REG(0x03, 0x0B, 4, 12)
#define TV5725_VDS_VS_ST                TV5725_REG(0x03, 0x0D, 0, 11)
#define TV5725_VDS_VS_SP                TV5725_REG(0x03, 0x0E, 4, 11)

#define TV5725_VDS_DIS_HB_ST            TV5725_REG(0x03, 0x10, 0, 12)
#define TV5725_VDS_DIS_HB_SP            TV5725_REG(0x03, 0x11, 4, 12)
#define TV5725_VDS_DIS_VB_ST            TV5725_REG(0x03, 0x13, 0, 11)
#define TV5725_VDS_DIS_VB_SP            TV5725_REG(0x03, 0x14, 4, 11)

#define TV5725_VDS_HSCALE               TV5725_REG(0x03, 0x16, 0, 10)
#define TV5725_VDS_VSCALE               TV5725_REG(0x03, 0x17, 4, 10)
#define TV5725_VDS_FRAME_RST            TV5725_REG(0x03, 0x19, 0, 10)
#define TV5725_VDS_FLOCK_EN             TV5725_REG(0x03, 0x1A, 4, 1)
#define TV5725_VDS_FREERUN_FID          TV5725_REG(0x03, 0x1A, 5, 1)

/* VDS color / scaling */
#define TV5725_VDS_Y_GAIN               TV5725_REG(0x03, 0x35, 0, 8)
#define TV5725_VDS_UCOS_GAIN            TV5725_REG(0x03, 0x36, 0, 8)
#define TV5725_VDS_VCOS_GAIN            TV5725_REG(0x03, 0x37, 0, 8)
#define TV5725_VDS_USIN_GAIN            TV5725_REG(0x03, 0x38, 0, 8)
#define TV5725_VDS_VSIN_GAIN            TV5725_REG(0x03, 0x39, 0, 8)
#define TV5725_VDS_Y_OFST               TV5725_REG(0x03, 0x3A, 0, 8)
#define TV5725_VDS_U_OFST               TV5725_REG(0x03, 0x3B, 0, 8)
#define TV5725_VDS_V_OFST               TV5725_REG(0x03, 0x3C, 0, 8)

#define TV5725_VDS_CONVT_BYPS           TV5725_REG(0x03, 0x3E, 3, 1)
#define TV5725_VDS_DYN_BYPS             TV5725_REG(0x03, 0x3E, 4, 1)
#define TV5725_VDS_BLK_BF_EN            TV5725_REG(0x03, 0x3E, 7, 1)
#define TV5725_VDS_1ST_INT_BYPS         TV5725_REG(0x03, 0x40, 0, 1)
#define TV5725_VDS_2ND_INT_BYPS         TV5725_REG(0x03, 0x40, 1, 1)
#define TV5725_VDS_IN_DREG_BYPS         TV5725_REG(0x03, 0x40, 2, 1)

/* Noise reduction */
#define TV5725_VDS_GLB_NOISE            TV5725_REG(0x03, 0x51, 7, 11)
#define TV5725_VDS_NR_Y_BYPS            TV5725_REG(0x03, 0x52, 4, 1)
#define TV5725_VDS_NR_C_BYPS            TV5725_REG(0x03, 0x52, 5, 1)

/* PIP */
#define TV5725_PIP_EN                   TV5725_REG(0x03, 0x81, 7, 1)
#define TV5725_PIP_H_ST                 TV5725_REG(0x03, 0x88, 0, 12)
#define TV5725_PIP_H_SP                 TV5725_REG(0x03, 0x8A, 0, 12)
#define TV5725_PIP_V_ST                 TV5725_REG(0x03, 0x8C, 0, 11)
#define TV5725_PIP_V_SP                 TV5725_REG(0x03, 0x8E, 0, 11)

/* ================================================================
   Segment 0x04: Memory Controller / Playback / Capture / FIFO
   ================================================================ */
#define TV5725_SDRAM_RESET_SIGNAL       TV5725_REG(0x04, 0x00, 4, 1)
#define TV5725_SDRAM_START_INIT         TV5725_REG(0x04, 0x00, 7, 1)
#define TV5725_SDRAM_RESET_CONTROL      TV5725_REG(0x04, 0x00, 0, 8)

#define TV5725_CAPTURE_ENABLE           TV5725_REG(0x04, 0x21, 0, 1)
#define TV5725_CAP_FF_HALF_REQ          TV5725_REG(0x04, 0x21, 1, 1)
#define TV5725_CAP_SAFE_GUARD_EN        TV5725_REG(0x04, 0x21, 5, 1)

#define TV5725_PB_CUT_REFRESH           TV5725_REG(0x04, 0x2B, 0, 1)
#define TV5725_PB_REQ_SEL               TV5725_REG(0x04, 0x2B, 1, 2)
#define TV5725_PB_BYPASS                TV5725_REG(0x04, 0x2B, 3, 1)
#define TV5725_PB_DB_BUFFER_EN          TV5725_REG(0x04, 0x2B, 5, 1)
#define TV5725_PB_DB_FIELD_EN           TV5725_REG(0x04, 0x2B, 4, 1)
#define TV5725_PB_ENABLE                TV5725_REG(0x04, 0x2B, 7, 1)
#define TV5725_PB_MAST_FLAG_REG         TV5725_REG(0x04, 0x2C, 0, 8)
#define TV5725_PB_GENERAL_FLAG_REG      TV5725_REG(0x04, 0x2D, 0, 8)

#define TV5725_WFF_ENABLE               TV5725_REG(0x04, 0x42, 0, 1)
#define TV5725_WFF_FF_STA_INV           TV5725_REG(0x04, 0x42, 2, 1)
#define TV5725_WFF_SAFE_GUARD           TV5725_REG(0x04, 0x42, 3, 1)
#define TV5725_WFF_ADR_ADD_2            TV5725_REG(0x04, 0x42, 5, 1)

#define TV5725_RFF_ENABLE               TV5725_REG(0x04, 0x4D, 7, 1)
#define TV5725_RFF_REQ_SEL              TV5725_REG(0x04, 0x4D, 5, 2)
#define TV5725_RFF_ADR_ADD_2            TV5725_REG(0x04, 0x4D, 4, 1)
#define TV5725_RFF_LREQ_CUT             TV5725_REG(0x04, 0x50, 7, 1)

/* ================================================================
   Segment 0x05: ADC / Sync Processor (SP) / PLLAD / Decoder
   ================================================================ */
#define TV5725_ADC_CLK_PA               TV5725_REG(0x05, 0x00, 0, 2)
#define TV5725_ADC_CLK_PLLAD            TV5725_REG(0x05, 0x00, 2, 1)
#define TV5725_ADC_CLK_ICLK2X           TV5725_REG(0x05, 0x00, 3, 1)
#define TV5725_ADC_CLK_ICLK1X           TV5725_REG(0x05, 0x00, 4, 1)
#define TV5725_ADC_SOGEN                TV5725_REG(0x05, 0x02, 0, 1)
#define TV5725_ADC_SOGCTRL              TV5725_REG(0x05, 0x02, 1, 5)
#define TV5725_ADC_INPUT_SEL            TV5725_REG(0x05, 0x02, 6, 2)
#define TV5725_ADC_POWDZ                TV5725_REG(0x05, 0x03, 0, 1)
#define TV5725_ADC_RYSEL_R              TV5725_REG(0x05, 0x03, 1, 1)
#define TV5725_ADC_RYSEL_G              TV5725_REG(0x05, 0x03, 2, 1)
#define TV5725_ADC_RYSEL_B              TV5725_REG(0x05, 0x03, 3, 1)
#define TV5725_ADC_FLTR                 TV5725_REG(0x05, 0x03, 4, 2)

/* ADC offset / gain */
#define TV5725_ADC_ROFCTRL              TV5725_REG(0x05, 0x06, 0, 8)
#define TV5725_ADC_GOFCTRL              TV5725_REG(0x05, 0x07, 0, 8)
#define TV5725_ADC_BOFCTRL              TV5725_REG(0x05, 0x08, 0, 8)
#define TV5725_ADC_RGCTRL               TV5725_REG(0x05, 0x09, 0, 8)
#define TV5725_ADC_GGCTRL               TV5725_REG(0x05, 0x0A, 0, 8)
#define TV5725_ADC_BGCTRL               TV5725_REG(0x05, 0x0B, 0, 8)

/* ADC auto offset */
#define TV5725_ADC_AUTO_OFST_EN         TV5725_REG(0x05, 0x0E, 0, 1)
#define TV5725_ADC_AUTO_OFST_PRD        TV5725_REG(0x05, 0x0E, 1, 1)
#define TV5725_ADC_AUTO_OFST_DELAY      TV5725_REG(0x05, 0x0E, 2, 2)
#define TV5725_ADC_AUTO_OFST_STEP       TV5725_REG(0x05, 0x0E, 4, 2)

/* PLLAD */
#define TV5725_PLLAD_VCORST             TV5725_REG(0x05, 0x11, 0, 1)
#define TV5725_PLLAD_LEN                TV5725_REG(0x05, 0x11, 1, 1)
#define TV5725_PLLAD_PDZ                TV5725_REG(0x05, 0x11, 4, 1)
#define TV5725_PLLAD_FS                 TV5725_REG(0x05, 0x11, 5, 1)
#define TV5725_PLLAD_BPS                TV5725_REG(0x05, 0x11, 6, 1)
#define TV5725_PLLAD_LAT                TV5725_REG(0x05, 0x11, 7, 1)
#define TV5725_PLLAD_MD                 TV5725_REG(0x05, 0x12, 0, 12)
#define TV5725_PLLAD_R                  TV5725_REG(0x05, 0x16, 0, 2)
#define TV5725_PLLAD_S                  TV5725_REG(0x05, 0x16, 2, 2)
#define TV5725_PLLAD_KS                 TV5725_REG(0x05, 0x16, 4, 2)
#define TV5725_PLLAD_ICP                TV5725_REG(0x05, 0x17, 0, 3)

/* PA ADC/SP */
#define TV5725_PA_ADC_BYPSZ             TV5725_REG(0x05, 0x18, 0, 1)
#define TV5725_PA_ADC_S                 TV5725_REG(0x05, 0x18, 1, 5)
#define TV5725_PA_ADC_LOCKOFF           TV5725_REG(0x05, 0x18, 6, 1)
#define TV5725_PA_ADC_LAT               TV5725_REG(0x05, 0x18, 7, 1)
#define TV5725_PA_SP_BYPSZ              TV5725_REG(0x05, 0x19, 0, 1)
#define TV5725_PA_SP_S                  TV5725_REG(0x05, 0x19, 1, 5)
#define TV5725_PA_SP_LOCKOFF            TV5725_REG(0x05, 0x19, 6, 1)
#define TV5725_PA_SP_LAT                TV5725_REG(0x05, 0x19, 7, 1)

/* Decoder */
#define TV5725_DEC_WEN_MODE             TV5725_REG(0x05, 0x1E, 7, 1)
#define TV5725_DEC1_BYPS                TV5725_REG(0x05, 0x1F, 0, 1)
#define TV5725_DEC2_BYPS                TV5725_REG(0x05, 0x1F, 1, 1)
#define TV5725_DEC_MATRIX_BYPS          TV5725_REG(0x05, 0x1F, 2, 1)
#define TV5725_DEC_IDREG_EN             TV5725_REG(0x05, 0x1F, 7, 1)

/* Sync processor */
#define TV5725_SP_SOG_SRC_SEL           TV5725_REG(0x05, 0x20, 0, 1)
#define TV5725_SP_SOG_P_ATO             TV5725_REG(0x05, 0x20, 1, 1)
#define TV5725_SP_SOG_P_INV             TV5725_REG(0x05, 0x20, 2, 1)
#define TV5725_SP_EXT_SYNC_SEL          TV5725_REG(0x05, 0x20, 3, 1)
#define TV5725_SP_JITTER_SYNC           TV5725_REG(0x05, 0x20, 4, 1)
#define TV5725_SP_HS_POL_ATO            TV5725_REG(0x05, 0x55, 4, 1)
#define TV5725_SP_VS_POL_ATO            TV5725_REG(0x05, 0x55, 6, 1)
#define TV5725_SP_HCST_AUTO_EN          TV5725_REG(0x05, 0x55, 7, 1)
#define TV5725_SP_SOG_MODE              TV5725_REG(0x05, 0x56, 0, 1)
#define TV5725_SP_SYNC_BYPS             TV5725_REG(0x05, 0x56, 4, 1)
#define TV5725_SP_CLAMP_MANUAL          TV5725_REG(0x05, 0x56, 2, 1)

/* ================================================================
   Function prototypes
   ================================================================ */

int32_t tv5725_init(void);
int32_t tv5725_input_path_init(void);
int32_t tv5725_output_path_init(void);
void tv5725_chip_reset(void);
uint8_t tv5725_get_chip_id(void);

void tv5725_set_segment(uint8_t segment);
int32_t tv5725_read_byte(uint8_t segment, uint8_t offset, uint8_t *value);
int32_t tv5725_write_byte(uint8_t segment, uint8_t offset, uint8_t value);
int32_t tv5725_read_buf(uint8_t segment, uint8_t offset, uint8_t *buf, uint8_t len);
int32_t tv5725_write_buf(uint8_t segment, uint8_t offset, const uint8_t *buf, uint8_t len);

uint32_t tv5725_reg_read(tv5725_reg_t reg);
void tv5725_reg_write(tv5725_reg_t reg, uint32_t value);

int32_t tv5725_read_seg_byte(uint8_t segment, uint8_t offset, uint8_t *value);
int32_t tv5725_write_seg_byte(uint8_t segment, uint8_t offset, uint8_t value);

#endif /* TV5725_H */
