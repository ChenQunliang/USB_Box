#include "tv5725.h"
#include "v_i2c.h"
#include <string.h>
#include "si5351.h"

/* ASW analog switch state */
tv5725_config_t g_tv5725_cfg = {false, false, false, false};

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

/* ==================================================================
   Sync processor initialisation (segment 5)

   Mirrors gbs-control's prepareSyncProcessor().
   Configures sync polarity, jitter filtering, clamp/coast behaviour,
   and HS/VS output timing.
   ================================================================== */
static void tv5725_sync_processor_init(void)
{
    tv5725_reg_write(TV5725_RW_SP_SOG_P_ATO, 0);
    tv5725_reg_write(TV5725_RW_SP_JITTER_SYNC, 0);

    /* Sync timing / filter registers (raw byte writes) */
    tv5725_write_byte(5, 0x21, 0x18);
    tv5725_write_byte(5, 0x22, 0x0F);
    tv5725_write_byte(5, 0x23, 0x00);
    tv5725_write_byte(5, 0x24, 0x40);
    tv5725_write_byte(5, 0x25, 0x00);
    tv5725_reg_write(TV5725_RW_SP_SYNC_PD_THD, 0x04);
    tv5725_write_byte(5, 0x27, 0x00);
    tv5725_write_byte(5, 0x2a, 0x0F);
    tv5725_write_byte(5, 0x2d, 0x03);
    tv5725_write_byte(5, 0x2e, 0x00);
    tv5725_write_byte(5, 0x2f, 0x02);
    tv5725_write_byte(5, 0x31, 0x2f);

    tv5725_reg_write(TV5725_RW_SYNC_PROC_14, 0x3a);
    tv5725_write_byte(5, 0x34, 0x06);

    /* Delay-line threshold (default for progressive inputs) */
    tv5725_reg_write(TV5725_RW_SP_DLT_REG, 0x70);
    /* H-pulse ignore (default for non-SD inputs) */
    tv5725_reg_write(TV5725_RW_SYNC_PROC_18, 0x02);

    tv5725_reg_write(TV5725_RW_SYNC_PROC_21, 3);

    /* SDCS vsync start/stop */
    tv5725_reg_write(TV5725_RW_SP_SDCS_VSST_REG_H, 0);
    tv5725_reg_write(TV5725_RW_SP_SDCS_VSSP_REG_H, 0);
    tv5725_reg_write(TV5725_RW_SYNC_PROC_24, 4);
    tv5725_reg_write(TV5725_RW_SYNC_PROC_25, 1);

    /* CS HS start/stop */
    tv5725_reg_write(TV5725_RW_SP_CS_HS_ST, 0x10);
    tv5725_reg_write(TV5725_RW_SP_CS_HS_SP, 0x00);

    /* RT HS start/stop */
    tv5725_reg_write(TV5725_RW_SP_RT_HS_ST, 0);
    tv5725_reg_write(TV5725_RW_SP_RT_HS_SP, 0x44);

    tv5725_write_byte(5, 0x51, 0x02);
    tv5725_write_byte(5, 0x52, 0x00);
    tv5725_write_byte(5, 0x53, 0x00);
    tv5725_write_byte(5, 0x54, 0x00);

    /* Clamp / coast / SOG */
    tv5725_reg_write(TV5725_RW_SP_CLAMP_MANUAL, 0);
    tv5725_reg_write(TV5725_RW_SP_CLP_SRC_SEL, 0);
    tv5725_reg_write(TV5725_RW_SP_NO_CLAMP_REG, 1);
    tv5725_reg_write(TV5725_RW_SP_SOG_MODE, 1);
    tv5725_reg_write(TV5725_RW_SP_H_CST_ST, 0x10);
    tv5725_reg_write(TV5725_RW_SP_H_CST_SP, 0x100);
    tv5725_reg_write(TV5725_RW_SP_DIS_SUB_COAST, 0);
    tv5725_reg_write(TV5725_RW_SP_H_PROTECT, 1);
    tv5725_reg_write(TV5725_RW_SP_HCST_AUTO_EN, 0);
    tv5725_reg_write(TV5725_RW_SP_NO_COAST_REG, 0);

    /* HS / VS processing */
    tv5725_reg_write(TV5725_RW_SP_HS_REG, 1);
    tv5725_reg_write(TV5725_RW_SP_HS_PROC_INV_REG, 0);
    tv5725_reg_write(TV5725_RW_SP_VS_PROC_INV_REG, 0);

    tv5725_write_byte(5, 0x58, 0x05);
    tv5725_write_byte(5, 0x59, 0x00);
    tv5725_write_byte(5, 0x5a, 0x01);
    tv5725_write_byte(5, 0x5b, 0x00);
    tv5725_write_byte(5, 0x5c, 0x03);
    tv5725_write_byte(5, 0x5d, 0x02);
}

/* ==================================================================
   ADC 偏移自动校准（G/R/B）

   移植自 gbs-control 的 calibrateAdcOffset()。
   将每个 ADC 通道通过测试总线引出，调整偏移寄存器
   使输出接近零（< 7 counts）。
   ================================================================== */
static void tv5725_calibrate_adc_offset(void)
{
    uint8_t r_off, g_off, b_off;
    uint8_t readout = 0;
    uint8_t miss_target;
    uint16_t hit_target;
    uint16_t readout16;

    /* 进入测试模式 */
    tv5725_reg_write(TV5725_RW_PAD_BOUT_EN, 0);
    tv5725_reg_write(TV5725_RW_CONTROL_PLL648_01, 0xA5);
    tv5725_reg_write(TV5725_RW_ADC_INPUT_SEL, 2);
    tv5725_reg_write(TV5725_RW_DEC_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_RW_DEC_TEST_ENABLE, 1);
    tv5725_write_byte(5, 0x03, 0x31);
    tv5725_write_byte(5, 0x04, 0x00);
    tv5725_reg_write(TV5725_RW_SP_CS_CLP_ST, 0x00);
    tv5725_reg_write(TV5725_RW_SP_CS_CLP_SP, 0x00);
    tv5725_reg_write(TV5725_RW_SYNC_PROC_48, 0x05);
    tv5725_reg_write(TV5725_RW_SYNC_PROC_49, 0x80);
    tv5725_write_byte(5, 0x00, 0x02);
    tv5725_reg_write(TV5725_RW_TEST_BUS_SEL, 0x0B);
    tv5725_reg_write(TV5725_RW_TEST_BUS_EN, 1);

    /* 软复位数字模块 */
    tv5725_reg_write(TV5725_RW_CONTROL_RESET_01, 0x17);
    tv5725_reg_write(TV5725_RW_CONTROL_RESET_00, 0x41);
    tv5725_reg_write(TV5725_RW_CONTROL_RESET_00, 0x7F);

    /* 增益设为中值 */
    tv5725_write_byte(5, 0x09, 0x7F);
    tv5725_write_byte(5, 0x0A, 0x7F);
    tv5725_write_byte(5, 0x0B, 0x7F);

    /* 初始偏移 */
    tv5725_write_byte(5, 0x06, 0x7F);
    tv5725_write_byte(5, 0x07, 0x3D);
    tv5725_write_byte(5, 0x08, 0x7F);
    tv5725_reg_write(TV5725_RW_DEC_TEST_SEL, 1);

    /* 依次校准 G (ch=0), R (ch=1), B (ch=2) */
    for (uint8_t ch = 0; ch < 3; ch++)
    {
        miss_target = 0;
        hit_target = 0;

        volatile uint32_t timeout = 800000;

        while (timeout--)
        {
            readout16 = (uint16_t)tv5725_reg_read(TV5725_RO_TEST_BUS) & 0x7FFFU;

            if (readout16 < 7)
            {
                hit_target++;
                miss_target = 0;
            }
            else if (miss_target++ > 2)
            {
                if (ch == 0)
                {
                    tv5725_read_byte(5, 0x07, &readout);
                    readout++;
                    tv5725_write_byte(5, 0x07, readout);
                }
                else if (ch == 1)
                {
                    tv5725_read_byte(5, 0x06, &readout);
                    readout++;
                    tv5725_write_byte(5, 0x06, readout);
                }
                else
                {
                    tv5725_read_byte(5, 0x08, &readout);
                    readout++;
                    tv5725_write_byte(5, 0x08, readout);
                }

                if (readout >= 0x52)
                    break;

                /* 短暂延时等待稳定 */
                for (volatile uint32_t d = 0; d < 5000; d++)
                    continue;

                hit_target = 0;
                miss_target = 0;
                timeout = 800000;
            }

            if (hit_target > 1500)
                break;
        }

        /* 保存当前通道结果，切换到下一通道 */
        if (ch == 0)
        {
            tv5725_read_byte(5, 0x07, &g_off);
            tv5725_write_byte(5, 0x07, 0x7F);
            tv5725_write_byte(5, 0x06, 0x3D);
            tv5725_reg_write(TV5725_RW_DEC_TEST_SEL, 2);
        }
        else if (ch == 1)
        {
            tv5725_read_byte(5, 0x06, &r_off);
            tv5725_write_byte(5, 0x06, 0x7F);
            tv5725_write_byte(5, 0x08, 0x3D);
            tv5725_reg_write(TV5725_RW_DEC_TEST_SEL, 3);
        }
        else
        {
            tv5725_read_byte(5, 0x08, &b_off);
        }
    }

    /* 校准失败保护 — 强制设为中值 */
    if (readout >= 0x52)
        r_off = g_off = b_off = 0x40;

    /* 写入最终校准后的偏移值 */
    tv5725_write_byte(5, 0x07, g_off);
    tv5725_write_byte(5, 0x06, r_off);
    tv5725_write_byte(5, 0x08, b_off);
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

    tv5725_asw_init();

    tv5725_sync_processor_init();

    tv5725_calibrate_adc_offset();
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

    /* ADC input: disable SOG, select RGB input channel (1=RGB, 0=YUV) */
    tv5725_reg_write(TV5725_RW_ADC_SOGEN, 0);
    tv5725_reg_write(TV5725_RW_ADC_INPUT_SEL, 1);

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

int32_t tv5725_input_config_vga(void)
{
    input_adc_common();

    /* VGA (RGBHV) mode: disable SOG, separate H/V sync, auto polarity */
    tv5725_reg_write(TV5725_RW_ADC_SOGEN, 0);
    tv5725_reg_write(TV5725_RW_SP_SOG_SRC_SEL, 0);
    tv5725_reg_write(TV5725_RW_SP_EXT_SYNC_SEL, 0);   /* HS_HS: H from H pin, V from V pin */
    tv5725_reg_write(TV5725_RW_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_VS_POL_ATO, 1);

    /* IF: 24-bit input, bypass color matrix, bypass data register */
    tv5725_reg_write(TV5725_RW_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_RW_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_RW_IF_IN_DREG_BYPS, 1);

    /* ASW: VGA */
    tv5725_asw_set_vga();

    g_input_mode = TV5725_INPUT_VGA;
    return LL_OK;
}

int32_t tv5725_input_config_rgbs(void)
{
    input_adc_common();

    /* RGBS mode: disable SOG, composite sync on CS pin, auto polarity */
    tv5725_reg_write(TV5725_RW_ADC_SOGEN, 0);
    tv5725_reg_write(TV5725_RW_SP_SOG_SRC_SEL, 0);
    tv5725_reg_write(TV5725_RW_SP_EXT_SYNC_SEL, 1);   /* CS_HS: composite sync from CS pin */
    tv5725_reg_write(TV5725_RW_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_VS_POL_ATO, 1);

    /* IF: 24-bit input, bypass color matrix, bypass data register */
    tv5725_reg_write(TV5725_RW_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_RW_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_RW_IF_IN_DREG_BYPS, 1);

    /* ASW: RGBS */
    tv5725_asw_set_rgbs();

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

    /* ASW: RGsB */
    tv5725_asw_set_rgsb();
    return LL_OK;
}

void tv5725_input_auto_detect(void)
{
    uint8_t status;
    tv5725_read_byte(0x00, 0x05, &status);

    if (status == 0x00 || status == 0xFF)
    {
        tv5725_input_config_vga();
        return;
    }
    if (!(status & 0x02))
        tv5725_input_config_vga();
    else
    {
        tv5725_input_config_rgbs();
        g_input_mode = TV5725_INPUT_AUTO;
    }
}

int32_t tv5725_input_set_mode(tv5725_input_mode_t mode)
{
    switch (mode)
    {
    case TV5725_INPUT_VGA:
        return tv5725_input_config_vga();
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
   同步通道选择 — 所有输入模式共用

   配置同步源选择（外部 H/V）、SOG 源、极性自动检测、
   及 coast/clamp 相关寄存器。
   ================================================================== */

void tv5725_sync_config(void)
{
    /* 外部同步源选择：0 = HS_HS（H 从 H 引脚，V 从 V 引脚） */
    tv5725_reg_write(TV5725_RW_SP_EXT_SYNC_SEL, 0);

    /* SOG 源选择：0 = 绿色通道 */
    tv5725_reg_write(TV5725_RW_SP_SOG_SRC_SEL, 0);

    /* 极性自动检测 */
    tv5725_reg_write(TV5725_RW_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_VS_POL_ATO, 1);

    /* 同步处理 */
    tv5725_reg_write(TV5725_RW_SP_HS_LOOP_SEL, 1);
    tv5725_reg_write(TV5725_RW_SP_SYNC_BYPS, 0);
    tv5725_reg_write(TV5725_RW_SP_HS_PROC_INV_REG, 0);
    tv5725_reg_write(TV5725_RW_SP_VS_PROC_INV_REG, 0);

    /* Coast / Clamp 配置 */
    tv5725_reg_write(TV5725_RW_SP_PRE_COAST, 4);
    tv5725_reg_write(TV5725_RW_SP_POST_COAST, 7);
    tv5725_reg_write(TV5725_RW_SP_H_PULSE_IGNOR, 0xFF);
    tv5725_reg_write(TV5725_RW_SP_NO_COAST_REG, 0);
    tv5725_reg_write(TV5725_RW_SP_NO_CLAMP_REG, 1);
    tv5725_reg_write(TV5725_RW_SP_CLAMP_MANUAL, 0);
    tv5725_reg_write(TV5725_RW_SP_COAST_INV_REG, 0);
    tv5725_reg_write(TV5725_RW_SP_DIS_SUB_COAST, 0);
    tv5725_reg_write(TV5725_RW_SP_H_PROTECT, 1);
    tv5725_reg_write(TV5725_RW_SP_HCST_AUTO_EN, 0);
}

/* ==================================================================
   输出路径初始化 — YPbPr 分量输出

   移植自 gbs-control 的 applyComponentColorMixing() 和
   doPostPresetLoadSteps()。
   加载预设、配置 VDS 色彩空间转换（RGB→YPbPr）、
   DAC 和同步输出。

   @param preset      预设数据指针（432 字节 GBSCpro 格式）
   @param input_is_yuv 保留参数，已忽略（输出固定为 YPbPr）
   ================================================================== */

int32_t tv5725_output_path_init(const uint8_t *preset, uint8_t input_is_yuv)
{
    (void)input_is_yuv; /* 输出固定为 YPbPr，忽略输入类型 */

    /* === 加载预设 === */
    tv5725_load_preset(preset);

    /* === 同步通道配置 === */
    tv5725_sync_config();

    /* === YPbPr 输出配置（VDS 色彩空间转换） === */

    /* 使能 VDS 色彩转换（RGB→YPbPr），不旁路 */
    tv5725_reg_write(TV5725_RW_VDS_CONVT_BYPS, 0);
    tv5725_reg_write(TV5725_RW_PIP_CONVT_BYPS, 0);

    /* Y/C 增益 — YPbPr 电平（移植自 gbs-control applyComponentColorMixing） */
    tv5725_reg_write(TV5725_RW_VDS_Y_GAIN, 0x64);      /* Y 增益: 100 */
    tv5725_reg_write(TV5725_RW_VDS_UCOS_GAIN, 0x19);   /* U/Cb 增益: 25 */
    tv5725_reg_write(TV5725_RW_VDS_VCOS_GAIN, 0x19);   /* V/Cr 增益: 25 */
    tv5725_reg_write(TV5725_RW_VDS_Y_OFST, 0xFE);      /* Y 偏移: 254 */
    tv5725_reg_write(TV5725_RW_VDS_U_OFST, 0x01);      /* U 偏移: 1 */
    tv5725_reg_write(TV5725_RW_VDS_V_OFST, 0x00);      /* V 偏移: 0 */

    /* === 输出 DAC 配置（YPbPr 输出） === */

    /* 使能同步输入引脚 */
    tv5725_reg_write(TV5725_RW_PAD_SYNC1_IN_ENZ, 0);
    tv5725_reg_write(TV5725_RW_PAD_SYNC2_IN_ENZ, 0);

    /* DAC 输出使能 */
    tv5725_reg_write(TV5725_RW_DAC_RGBS_R0ENZ, 1);     /* R → Pr */
    tv5725_reg_write(TV5725_RW_DAC_RGBS_G0ENZ, 1);     /* G → Y  */
    tv5725_reg_write(TV5725_RW_DAC_RGBS_B0ENZ, 1);     /* B → Pb */

    /* 同步 DAC 配置（同步叠加在 Y 通道上） */
    tv5725_reg_write(TV5725_RW_DAC_RGBS_SPD, 0);
    tv5725_reg_write(TV5725_RW_DAC_RGBS_S0ENZ, 0);     /* 使能同步 DAC 输出 */
    tv5725_reg_write(TV5725_RW_DAC_RGBS_S1EN, 1);      /* 使能同步输出 1 */

    /* 输出同步选择与控制 */
    tv5725_reg_write(TV5725_RW_OUT_SYNC_SEL, 1);
    tv5725_reg_write(TV5725_RW_OUT_SYNC_CNTRL, 1);

    /* DAC 上电 */
    tv5725_reg_write(TV5725_RW_DAC_RGBS_PWDNZ, 1);

    /* 矩阵配置 — RGB 输入旁路，YPbPr 靠 VDS 转换 */
    tv5725_reg_write(TV5725_RW_DEC_MATRIX_BYPS, 1);    /* 旁路解码器 YUV→RGB（输入已是 RGB） */
    tv5725_reg_write(TV5725_RW_IF_MATRIX_BYPS, 1);     /* 旁路 IF 矩阵 */
    tv5725_reg_write(TV5725_RW_HD_MATRIX_BYPS, 1);     /* 旁路 HD 矩阵 */
    tv5725_reg_write(TV5725_RW_HD_DYN_BYPS, 1);        /* 旁路 HD 动态 */

    /* === ADC 自动偏移 === */
    tv5725_reg_write(TV5725_RW_ADC_AUTO_OFST_PRD, 1);
    tv5725_reg_write(TV5725_RW_ADC_AUTO_OFST_DELAY, 0);
    tv5725_reg_write(TV5725_RW_ADC_AUTO_OFST_STEP, 0);
    tv5725_reg_write(TV5725_RW_ADC_AUTO_OFST_TEST, 1);
    tv5725_reg_write(TV5725_RW_ADC_AUTO_OFST_01, 0x00);

    return LL_OK;
}

/* ==================================================================
   ASW analog switch — PB12-PB15

   Pin mapping (matching hardware schematic):
     PB15 = ASW1    PB14 = ASW2    PB13 = ASW3    PB12 = ASW4

   Each ASW controls an analog switch that routes the sync signal
   from a specific input connector to the TV5725.

   Input mode → ASW table (移植自 usart_uart_dma 工程):
     VGA:   asw_01=1, asw_02=var, asw_03=1, asw_04=1
     RGBS:  asw_01=0, asw_02=0,   asw_03=0, asw_04=1
     RGsB:  asw_01=0, asw_02=0,   asw_03=1, asw_04=0
     YPbPr: asw_01=0, asw_02=0,   asw_03=1, asw_04=0
   ================================================================== */

void tv5725_asw_init(void)
{
    stc_gpio_init_t cfg;
    GPIO_StructInit(&cfg);
    cfg.u16PinDir = PIN_DIR_OUT;

    /* Init all 4 pins as outputs */
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW1, &cfg);
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW2, &cfg);
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW3, &cfg);
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW4, &cfg);

    /* Default: all off (reset state) */
    ASW1_Off(); ASW2_Off(); ASW3_Off(); ASW4_Off();
}

void tv5725_asw_ctrl(bool sw1, bool sw2, bool sw3, bool sw4)
{
    if (sw1)
        ASW1_On();
    else
        ASW1_Off();

    if (sw2)
        ASW2_On();
    else
        ASW2_Off();

    if (sw3)
        ASW3_On();
    else
        ASW3_Off();

    if (sw4)
        ASW4_On();
    else
        ASW4_Off();

    g_tv5725_cfg.asw_01 = sw1;
    g_tv5725_cfg.asw_02 = sw2;
    g_tv5725_cfg.asw_03 = sw3;
    g_tv5725_cfg.asw_04 = sw4;
}

/* 封装：各输入模式专用 ASW 设置 */
void tv5725_asw_set_vga(void)
{
    tv5725_asw_ctrl(1, g_tv5725_cfg.asw_02, 1, 1);
}

void tv5725_asw_set_rgbs(void)
{
    tv5725_asw_ctrl(0, 0, 0, 1);
}

void tv5725_asw_set_rgsb(void)
{
    tv5725_asw_ctrl(0, 0, 1, 0);
}
