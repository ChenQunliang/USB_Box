#include "tv5725.h"
#include "v_i2c.h"
#include <string.h>
#include "si5351.h"
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
    uint8_t buf[8];
    uint8_t n = (uint8_t)((reg.bit_offset + reg.bit_width + 7) / 8);

    if (n > sizeof(buf) || reg.bit_width > 32)
        return 0;

    if (seg_read(reg.segment, reg.offset, buf, n) != LL_OK)
        return 0;

    uint64_t raw = 0;
    for (uint8_t i = 0; i < n; i++)
        raw |= (uint64_t)buf[i] << (i * 8);

    return (uint32_t)((raw >> reg.bit_offset) & ((1ULL << reg.bit_width) - 1ULL));
}

void tv5725_reg_write(tv5725_reg_t reg, uint32_t value)
{
    uint8_t buf[8];
    uint8_t n = (uint8_t)((reg.bit_offset + reg.bit_width + 7) / 8);

    if (n > sizeof(buf))
        return;

    /* Full-byte-aligned: skip read-modify */
    if (reg.bit_offset == 0 && reg.bit_width == n * 8)
    {
        for (uint8_t i = 0; i < n; i++)
            buf[i] = (uint8_t)(value >> (i * 8));
        seg_write(reg.segment, reg.offset, buf, n);
        return;
    }

    /* Bitfield (single or multi-byte): read-modify-write */
    if (seg_read(reg.segment, reg.offset, buf, n) != LL_OK)
        return;

    uint64_t raw = 0;
    for (uint8_t i = 0; i < n; i++)
        raw |= (uint64_t)buf[i] << (i * 8);

    uint64_t mask = (reg.bit_width >= 64) ? ~0ULL : ((1ULL << reg.bit_width) - 1ULL);
    mask <<= reg.bit_offset;

    raw = (raw & ~mask) | (((uint64_t)value << reg.bit_offset) & mask);

    for (uint8_t i = 0; i < n; i++)
        buf[i] = (uint8_t)(raw >> (i * 8));

    seg_write(reg.segment, reg.offset, buf, n);
}

/* ==================================================================
   Chip-level operations
   ================================================================== */

void tv5725_chip_reset(void)
{
    // tv5725_reg_write(TV5725_RW_CONTROL_RESET_00, 0x00);
    // tv5725_reg_write(TV5725_RW_CONTROL_RESET_01, 0x00);

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
        TV5725_RW_PIP_V_SP,
    };
    int n = (int)(sizeof(bits) / sizeof(bits[0]));
    for (int i = 0; i < n; i++)
        tv5725_reg_write(bits[i], 0);
    for (int i = 0; i < n; i++)
        tv5725_reg_write(bits[i], 1);
}

uint32_t tv5725_get_chip_id(void)
{
    uint8_t buf[3];
    if (tv5725_read_buf(0x00, 0x0B, buf, 3) != LL_OK)
        return 0;
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16);
}

static int32_t tv5725_PowerDetect(void)
{
    tv5725_reg_write(TV5725_RW_ADC_UNUSED_69, 0x69); /* Assert reset on all blocks except ADC */
    if (tv5725_reg_read(TV5725_RW_ADC_UNUSED_69) == 0x69)
    {
        tv5725_reg_write(TV5725_RW_ADC_UNUSED_69, 00);
        return 1;
    }
    tv5725_reg_write(TV5725_RW_ADC_UNUSED_69, 00);
    printf("No power\n");
    return 0;
}

int32_t tv5725_init(void)
{
    uint32_t id = tv5725_get_chip_id();
    if (id == 0x00 || id == 0xFFFFFFFF)
        return LL_ERR;
    printf("TV5725 Chip ID: 0x%06lX\n", id);
    tv5725_reg_write(TV5725_RW_CONTROL_RESET_00, 0x00); /* Reset all blocks except ADC */
    tv5725_reg_write(TV5725_RW_CONTROL_RESET_01, 0x00); /* Reset all blocks except ADC */
    tv5725_reg_write(TV5725_RW_PLLAD_VCORST, 1);        /* PLLAD VCO reset */
    tv5725_reg_write(TV5725_RW_PLLAD_PDZ, 0);           /* PLLAD power down */

    tv5725_reg_write(TV5725_RW_PAD_CKIN_ENZ, 1); /* Enable external clock input  */

    si5351_external_clock_init();

    tv5725_PowerDetect();
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
   Input mode config
   ================================================================== */

static tv5725_input_mode_t g_input_mode = TV5725_INPUT_AUTO;

static void input_adc_common(void)
{
    /* ADC clock: PA=0, ICLK2X=0, ICLK1X=0, PLLAD clock disabled */
    tv5725_reg_write(TV5725_RW_CONTROL_ADC_CLK_00, 0x00);

    /* ADC input: disable SOG, analog input select = 0 */
    tv5725_reg_write(TV5725_RW_ADC_SOGEN, 0);
    tv5725_reg_write(TV5725_RW_ADC_INPUT_SEL, 0x00);

    /* ADC power up, R/G/B clamp enabled, filter = 01 */
    tv5725_reg_write(TV5725_RW_ADC_POWDZ, 1);
    tv5725_reg_write(TV5725_RW_ADC_RYSEL_R, 1);
    tv5725_reg_write(TV5725_RW_ADC_RYSEL_G, 1);
    tv5725_reg_write(TV5725_RW_ADC_RYSEL_B, 1);
    tv5725_reg_write(TV5725_RW_ADC_FLTR, 0x01);

    /* ADC gain/offset defaults */
    tv5725_write_byte(0x05, 0x06, 0x00);
    tv5725_write_byte(0x05, 0x07, 0x00);
    tv5725_write_byte(0x05, 0x08, 0x00);
    tv5725_write_byte(0x05, 0x09, 0x80);
    tv5725_write_byte(0x05, 0x0A, 0x80);
    tv5725_write_byte(0x05, 0x0B, 0x80);

    /* PLLAD: power up, bypass off, normal frequency */
    tv5725_reg_write(TV5725_RW_PLLAD_PDZ, 1);
    tv5725_reg_write(TV5725_RW_PLLAD_BPS, 1);
    tv5725_reg_write(TV5725_RW_PLLAD_FS, 0);
}

int32_t tv5725_input_config_rgbs(void)
{
    input_adc_common();

    /* RGB mode: disable SOG, external H/V sync, auto polarity */
    tv5725_reg_write(TV5725_RW_ADC_SOGEN, 0);
    tv5725_reg_write(TV5725_RW_SP_SOG_SRC_SEL, 0);
    tv5725_reg_write(TV5725_RW_SP_EXT_SYNC_SEL, 0);
    tv5725_reg_write(TV5725_RW_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_VS_POL_ATO, 1);

    /* IF: 24-bit input, bypass color matrix, bypass data register */
    tv5725_reg_write(TV5725_RW_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_RW_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_RW_IF_IN_DREG_BYPS, 1);

    g_input_mode = TV5725_INPUT_RGBS;
    return LL_OK;
}

int32_t tv5725_input_config_rgsb(void)
{
    input_adc_common();

    /* RGsB mode: enable SOG on green channel, auto polarity */
    tv5725_reg_write(TV5725_RW_ADC_SOGEN, 1);
    tv5725_reg_write(TV5725_RW_SP_SOG_SRC_SEL, 0);
    tv5725_reg_write(TV5725_RW_SP_SOG_P_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_VS_POL_ATO, 1);

    /* IF: 24-bit input, bypass color matrix (RGB), bypass data register */
    tv5725_reg_write(TV5725_RW_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_RW_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_RW_IF_IN_DREG_BYPS, 1);

    g_input_mode = TV5725_INPUT_RGSB;
    return LL_OK;
}

int32_t tv5725_input_config_yuv(void)
{
    input_adc_common();

    /* YUV mode: enable SOG, SOG source = Y, auto polarity */
    tv5725_reg_write(TV5725_RW_ADC_SOGEN, 1);
    tv5725_reg_write(TV5725_RW_SP_SOG_SRC_SEL, 1);
    tv5725_reg_write(TV5725_RW_SP_SOG_P_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_VS_POL_ATO, 1);

    /* IF: 24-bit input, enable color matrix, bypass data register */
    tv5725_reg_write(TV5725_RW_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_RW_IF_MATRIX_BYPS, 0);
    tv5725_reg_write(TV5725_RW_IF_IN_DREG_BYPS, 1);

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
    case TV5725_INPUT_RGSB:
        return tv5725_input_config_rgsb();
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
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW1, &cfg);
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW2, &cfg);
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW3, &cfg);
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW4, &cfg);
}
