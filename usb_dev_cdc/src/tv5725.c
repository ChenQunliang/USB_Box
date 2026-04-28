#include "tv5725.h"
#include "v_i2c.h"
#include <string.h>

/* ==================================================================
   I2C address mapping (7-bit 0x17, unshift mode)
   W: 0x17 << 1 = 0x2E    R: (0x17 << 1) | 1 = 0x2F
   ================================================================== */
#define TV_WR_ADDR (TV5725_I2C_ADDR_8BIT)      /* 0x2E */
#define TV_RD_ADDR (TV5725_I2C_ADDR_8BIT | 1U) /* 0x2F */

/* ==================================================================
   Segment selection — write [0xF0, segment] then STOP (cached)
   ================================================================== */
static uint8_t s_cur_seg = 0xFF;

void tv5725_set_segment(uint8_t segment)
{
    if (s_cur_seg == segment)
        return;
    uint8_t buf[2] = {TV5725_SEG_REG, segment};
    I2C_Master_Transmit(TV_WR_ADDR, buf, 2, V_TIMEOUT);
    s_cur_seg = segment;
}

/* ==================================================================
   Register read  — [offset] then read data
   ================================================================== */

/* Read len bytes starting at register offset within a segment */
static int32_t seg_read(uint8_t seg, uint8_t offset, uint8_t *buf, uint8_t len)
{
    tv5725_set_segment(seg);
    int32_t ret = I2C_Master_Transmit(TV_WR_ADDR, &offset, 1, V_TIMEOUT);
    if (ret != LL_OK)
        return ret;
    return I2C_Master_Receive(TV_RD_ADDR, buf, len, V_TIMEOUT);
}

/* ==================================================================
   Register write — [offset, data...] combined in one transaction
   ================================================================== */

/* Write len bytes starting at register offset within a segment.
   Offset + data is sent in a single I2C write for len <= 16.
   Larger writes are chunked byte-by-byte. */
static int32_t seg_write(uint8_t seg, uint8_t offset, const uint8_t *buf, uint8_t len)
{
    tv5725_set_segment(seg);

    if (len > 16)
    {
        int32_t ret = LL_OK;
        for (uint8_t i = 0; i < len; i++)
        {
            uint8_t pkt[2] = {(uint8_t)(offset + i), buf[i]};
            ret = I2C_Master_Transmit(TV_WR_ADDR, pkt, 2, V_TIMEOUT);
            if (ret != LL_OK)
                break;
        }
        return ret;
    }

    /* [offset, data[0..len-1]] in one transaction */
    uint8_t pkt[17];
    pkt[0] = offset;
    (void)memcpy(&pkt[1], buf, len);
    return I2C_Master_Transmit(TV_WR_ADDR, pkt, (uint32_t)(len + 1), V_TIMEOUT);
}

/* ==================================================================
   Public API
   ================================================================== */

int32_t tv5725_read_byte(uint8_t seg, uint8_t offset, uint8_t *value)
{
    return seg_read(seg, offset, value, 1);
}

int32_t tv5725_write_byte(uint8_t seg, uint8_t offset, uint8_t value)
{
    return seg_write(seg, offset, &value, 1);
}

int32_t tv5725_read_buf(uint8_t seg, uint8_t offset, uint8_t *buf, uint8_t len)
{
    return seg_read(seg, offset, buf, len);
}

int32_t tv5725_write_buf(uint8_t seg, uint8_t offset, const uint8_t *buf, uint8_t len)
{
    return seg_write(seg, offset, buf, len);
}

/* ==================================================================
   Register descriptor API — byte or bitfield within a byte
   ================================================================== */

uint32_t tv5725_reg_read(tv5725_reg_t reg)
{
    uint8_t byte;
    if (reg.bit_width > 8)
        return 0;
    if (seg_read(reg.segment, reg.offset, &byte, 1) != LL_OK)
        return 0;
    return (uint32_t)((byte >> reg.bit_offset) & ((1U << reg.bit_width) - 1U));
}

void tv5725_reg_write(tv5725_reg_t reg, uint32_t value)
{
    if (reg.bit_width > 8)
        return;

    uint8_t byte;

    if (reg.bit_offset == 0 && reg.bit_width == 8)
    {
        /* Full byte: direct write */
        byte = (uint8_t)value;
        seg_write(reg.segment, reg.offset, &byte, 1);
    }
    else
    {
        /* Bitfield: read-modify-write */
        if (seg_read(reg.segment, reg.offset, &byte, 1) != LL_OK)
            return;
        uint8_t mask = (uint8_t)(((1U << reg.bit_width) - 1U) << reg.bit_offset);
        byte = (byte & ~mask) | (uint8_t)((value << reg.bit_offset) & mask);
        seg_write(reg.segment, reg.offset, &byte, 1);
    }
}

/* ==================================================================
   Chip-level operations
   ================================================================== */

void tv5725_chip_reset(void)
{
    tv5725_reg_write(TV5725_RESET_CONTROL_46, 0x00);
    tv5725_reg_write(TV5725_RESET_CONTROL_47, 0x00);

    for (uint8_t seg = 0; seg < TV5725_SEG_COUNT; seg++)
    {
        tv5725_set_segment(seg);
        for (uint8_t bank = 0; bank < 16; bank++)
        {
            uint8_t zeros[16] = {0};
            tv5725_write_buf(seg, (uint8_t)(bank * 16), zeros, 16);
        }
    }

    /* Assert then release all reset bits */
    const tv5725_reg_t bits[] = {
        TV5725_SFTRST_IF_RSTZ,
        TV5725_SFTRST_DEINT_RSTZ,
        TV5725_SFTRST_MEM_FF_RSTZ,
        TV5725_SFTRST_VDS_RSTZ,
        TV5725_SFTRST_FIFO_RSTZ,
        TV5725_SFTRST_DEC_RSTZ,
        TV5725_SFTRST_MODE_RSTZ,
        TV5725_SFTRST_SYNC_RSTZ,
        TV5725_SFTRST_HDBYPS_RSTZ,
    };
    int n = (int)(sizeof(bits) / sizeof(bits[0]));
    for (int i = 0; i < n; i++)
        tv5725_reg_write(bits[i], 0);
    for (int i = 0; i < n; i++)
        tv5725_reg_write(bits[i], 1);
}

uint8_t tv5725_get_chip_id(void)
{
    return (uint8_t)tv5725_reg_read(TV5725_CHIP_ID_PRODUCT);
}

int32_t tv5725_init(void)
{
    uint8_t id = tv5725_get_chip_id();
    if (id == 0x00 || id == 0xFF)
        return LL_ERR;
    printf("TV5725 Chip ID: 0x%02X\n", id);
    return LL_OK;
}

/* ==================================================================
   Preset loader — GBSCpro register preset format (432 bytes total)
   ================================================================== */

void tv5725_load_preset(const uint8_t *preset)
{
    uint16_t idx = 0;

    /* Seg 0: 0x40-0x5F, 0x90-0x9F */
    tv5725_write_buf(0x00, 0x40, preset + idx, 16);
    idx += 16;
    tv5725_write_buf(0x00, 0x50, preset + idx, 16);
    idx += 16;
    tv5725_write_buf(0x00, 0x90, preset + idx, 16);
    idx += 16;

    /* Seg 1: 0x00-0x2F */
    tv5725_write_buf(0x01, 0x00, preset + idx, 16);
    idx += 16;
    tv5725_write_buf(0x01, 0x10, preset + idx, 16);
    idx += 16;
    tv5725_write_buf(0x01, 0x20, preset + idx, 16);
    idx += 16;

    /* Seg 1: mode-detect section 0x60-0x83 */
    {
        extern const uint8_t preset_md_section[];
        tv5725_write_buf(0x01, 0x60, preset_md_section, 16);
        tv5725_write_buf(0x01, 0x70, preset_md_section + 16, 16);
        tv5725_write_buf(0x01, 0x80, preset_md_section + 32, 4);
    }

    /* Seg 2: deinterlacer preset (64 bytes) */
    {
        extern const uint8_t preset_deinterlacer[];
        tv5725_write_buf(0x02, 0x00, preset_deinterlacer, 16);
        tv5725_write_buf(0x02, 0x10, preset_deinterlacer + 16, 16);
        tv5725_write_buf(0x02, 0x20, preset_deinterlacer + 32, 16);
        tv5725_write_buf(0x02, 0x30, preset_deinterlacer + 48, 16);
    }

    /* Seg 3: 0x00-0x7F (128 bytes) */
    for (uint8_t i = 0; i < 8; i++, idx += 16)
        tv5725_write_buf(0x03, (uint8_t)(i * 16), preset + idx, 16);
    {
        uint8_t zeros[16] = {0};
        tv5725_write_buf(0x03, 0x80, zeros, 16);
    }

    /* Seg 4: 0x00-0x5F (96 bytes) */
    for (uint8_t i = 0; i < 6; i++, idx += 16)
        tv5725_write_buf(0x04, (uint8_t)(i * 16), preset + idx, 16);

    /* Seg 5: 0x00-0x6F (112 bytes) */
    for (uint8_t i = 0; i < 7; i++, idx += 16)
        tv5725_write_buf(0x05, (uint8_t)(i * 16), preset + idx, 16);
}

/* ==================================================================
   Frame buffer pipeline
   ================================================================== */

void tv5725_sdram_init(void)
{
    tv5725_reg_write(TV5725_SDRAM_RESET_CONTROL, 0x02);
    tv5725_reg_write(TV5725_SDRAM_RESET_SIGNAL, 1);
    tv5725_reg_write(TV5725_SDRAM_RESET_SIGNAL, 0);
    tv5725_reg_write(TV5725_SDRAM_RESET_CONTROL, 0x82);
}

void tv5725_capture_start(void)
{
    tv5725_reg_write(TV5725_WFF_ENABLE, 1);
    tv5725_reg_write(TV5725_RFF_ENABLE, 1);
    tv5725_reg_write(TV5725_CAPTURE_ENABLE, 1);
}

void tv5725_capture_stop(void)
{
    tv5725_reg_write(TV5725_CAPTURE_ENABLE, 0);
    tv5725_reg_write(TV5725_WFF_ENABLE, 0);
    tv5725_reg_write(TV5725_RFF_ENABLE, 0);
}

/* ==================================================================
   Color matrix
   ================================================================== */

void tv5725_apply_rgb_patches(void)
{
    tv5725_reg_write(TV5725_ADC_RYSEL_R, 0);
    tv5725_reg_write(TV5725_ADC_RYSEL_G, 0);
    tv5725_reg_write(TV5725_ADC_RYSEL_B, 0);
    tv5725_reg_write(TV5725_DEC_MATRIX_BYPS, 0);
    tv5725_reg_write(TV5725_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_VDS_Y_GAIN, 0x80);
    tv5725_reg_write(TV5725_VDS_UCOS_GAIN, 0x1C);
    tv5725_reg_write(TV5725_VDS_VCOS_GAIN, 0x29);
    tv5725_reg_write(TV5725_VDS_Y_OFST, 0x00);
    tv5725_reg_write(TV5725_VDS_U_OFST, 0x00);
    tv5725_reg_write(TV5725_VDS_V_OFST, 0x00);
}

void tv5725_apply_yuv_patches(void)
{
    tv5725_reg_write(TV5725_ADC_RYSEL_R, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_G, 0);
    tv5725_reg_write(TV5725_ADC_RYSEL_B, 1);
    tv5725_reg_write(TV5725_DEC_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_VDS_Y_GAIN, 128);
    tv5725_reg_write(TV5725_VDS_UCOS_GAIN, 28);
    tv5725_reg_write(TV5725_VDS_VCOS_GAIN, 41);
    tv5725_reg_write(TV5725_VDS_Y_OFST, 0x0E);
    tv5725_reg_write(TV5725_VDS_U_OFST, 0x03);
    tv5725_reg_write(TV5725_VDS_V_OFST, 0x04);
}

/* ==================================================================
   Output path init
   ================================================================== */

int32_t tv5725_output_path_init(const uint8_t *preset, uint8_t input_is_yuv)
{
    tv5725_load_preset(preset);
    tv5725_sdram_init();

    tv5725_reg_write(TV5725_DAC_RGBS_PWDNZ, 1);
    tv5725_reg_write(TV5725_DAC_RGBS_R0ENZ, 0); /* 0 = enable Red DAC */
    tv5725_reg_write(TV5725_DAC_RGBS_G0ENZ, 0); /* 0 = enable Green DAC */
    tv5725_reg_write(TV5725_DAC_RGBS_B0ENZ, 0); /* 0 = enable Blue DAC */
    tv5725_reg_write(TV5725_DAC_RGBS_SPD, 0);
    tv5725_reg_write(TV5725_DAC_RGBS_S0ENZ, 0);
    tv5725_reg_write(TV5725_DAC_RGBS_S1EN, 1);
    tv5725_reg_write(TV5725_PAD_SYNC_OUT_ENZ, 0);
    tv5725_reg_write(TV5725_OUT_SYNC_CNTRL, 1);

    if (input_is_yuv)
        tv5725_apply_yuv_patches();
    else
        tv5725_apply_rgb_patches();

    tv5725_reg_write(TV5725_ADC_AUTO_OFST_PRD, 1);
    tv5725_reg_write(TV5725_ADC_AUTO_OFST_DELAY, 0);
    tv5725_reg_write(TV5725_ADC_AUTO_OFST_STEP, 0);
    tv5725_reg_write(TV5725_ADC_AUTO_OFST_TEST, 1);
    tv5725_reg_write(TV5725_ADC_AUTO_OFST_RANGE_REG, 0x00);

    tv5725_capture_start();
    return LL_OK;
}

/* ==================================================================
   Input mode config
   ================================================================== */

static tv5725_input_mode_t g_input_mode = TV5725_INPUT_AUTO;

static void input_adc_common(void)
{
    tv5725_reg_write(TV5725_ADC_CLK_PA, 0x00);
    tv5725_reg_write(TV5725_ADC_CLK_PLLAD, 1);
    tv5725_reg_write(TV5725_ADC_CLK_ICLK2X, 0);
    tv5725_reg_write(TV5725_ADC_CLK_ICLK1X, 0);
    tv5725_reg_write(TV5725_ADC_INPUT_SEL, 0x00);
    tv5725_reg_write(TV5725_ADC_POWDZ, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_R, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_G, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_B, 1);
    tv5725_reg_write(TV5725_ADC_FLTR, 0x01);

    tv5725_write_byte(0x05, 0x06, 0x00);
    tv5725_write_byte(0x05, 0x07, 0x00);
    tv5725_write_byte(0x05, 0x08, 0x00);
    tv5725_write_byte(0x05, 0x09, 0x80);
    tv5725_write_byte(0x05, 0x0A, 0x80);
    tv5725_write_byte(0x05, 0x0B, 0x80);

    tv5725_reg_write(TV5725_PLLAD_PDZ, 1);
    tv5725_reg_write(TV5725_PLLAD_BPS, 1);
    tv5725_reg_write(TV5725_PLLAD_FS, 0);
}

int32_t tv5725_input_config_rgbs(void)
{
    input_adc_common();

    tv5725_reg_write(TV5725_ADC_SOGEN, 0);
    tv5725_reg_write(TV5725_SP_SOG_SRC_SEL, 0);
    tv5725_reg_write(TV5725_SP_EXT_SYNC_SEL, 0);
    tv5725_reg_write(TV5725_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_SP_VS_POL_ATO, 1);
    tv5725_reg_write(TV5725_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_IF_IN_DREG_BYPS, 1);

//    tv5725_asw_set(TV5725_ASW01, 1);
//    tv5725_asw_set(TV5725_ASW02, 0);
//    tv5725_asw_set(TV5725_ASW03, 0);
//    tv5725_asw_set(TV5725_ASW04, 0);

    g_input_mode = TV5725_INPUT_RGBS;
    return LL_OK;
}

int32_t tv5725_input_config_yuv(void)
{
    input_adc_common();

    tv5725_reg_write(TV5725_ADC_SOGEN, 1);
    tv5725_reg_write(TV5725_SP_SOG_SRC_SEL, 1);
    tv5725_reg_write(TV5725_SP_SOG_P_ATO, 1);
    tv5725_reg_write(TV5725_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_SP_VS_POL_ATO, 1);
    tv5725_reg_write(TV5725_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_IF_MATRIX_BYPS, 0);
    tv5725_reg_write(TV5725_IF_IN_DREG_BYPS, 1);

//    tv5725_asw_set(TV5725_ASW01, 0);
//    tv5725_asw_set(TV5725_ASW02, 0);
//    tv5725_asw_set(TV5725_ASW03, 0);
//    tv5725_asw_set(TV5725_ASW04, 0);

    g_input_mode = TV5725_INPUT_YUV;
    return LL_OK;
}

void tv5725_input_auto_detect(void)
{
    uint8_t status;
    tv5725_read_byte(0x00, 0x05, &status);

    if (status == 0x00 || status == 0xFF)
    {
        tv5725_input_config_rgbs();
        return;
    }
    if (!(status & 0x02))
        tv5725_input_config_rgbs();
    else
    {
        tv5725_input_config_yuv();
        g_input_mode = TV5725_INPUT_AUTO;
    }
}

int32_t tv5725_input_set_mode(tv5725_input_mode_t mode)
{
    switch (mode)
    {
    case TV5725_INPUT_YUV:
        return tv5725_input_config_yuv();
    case TV5725_INPUT_RGBS:
        return tv5725_input_config_rgbs();
    default:
        tv5725_input_auto_detect();
        return LL_OK;
    }
}

/* ==================================================================
   ASW GPIO (PB12-PB15)
   ================================================================== */

void tv5725_asw_init(void)
{
    stc_gpio_init_t cfg;
    GPIO_StructInit(&cfg);
    cfg.u16PinDir = PIN_DIR_OUT;
    GPIO_Init(TV5725_ASW_PORT, TV5725_ASW01, &cfg);
    GPIO_Init(TV5725_ASW_PORT, TV5725_ASW02, &cfg);
    GPIO_Init(TV5725_ASW_PORT, TV5725_ASW03, &cfg);
    GPIO_Init(TV5725_ASW_PORT, TV5725_ASW04, &cfg);
}

// void tv5725_asw_set(uint16_t pin, uint8_t high)
// {
//     if (high)
//         GPIO_SetPins(TV5725_ASW_PORT, pin);
//     else
//         GPIO_ResetPins(TV5725_ASW_PORT, pin);
// }
