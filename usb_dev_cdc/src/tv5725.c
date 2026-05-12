#include "tv5725.h"
#include "v_i2c.h"
#include <string.h>
#include "si5351.h"

tv5725_config_t g_tv5725_cfg = {
    false, false, false, false, /* asw_01..04 */
    0, true,                    /* asw_sweep_combo, asw_sweep_first */
    -1, -1,                     /* sog_tune_level, sog_tune_best */
    TV5725_INPUT_AUTO,          /* input_mode */
    NULL,                       /* cur_preset */
    0xFF                        /* i2c_cur_seg */
};

/* ==================================================================
   I2C address mapping (7-bit 0x17, unshift mode)
   W: 0x17 << 1 = 0x2E    R: (0x17 << 1) | 1 = 0x2F
   ================================================================== */
#define TV_WR_ADDR (TV5725_I2C_ADDR_8BIT)      /* 0x2E */
#define TV_RD_ADDR (TV5725_I2C_ADDR_8BIT | 1U) /* 0x2F */

/* ==================================================================
   Segment selection — write [0xF0, segment] then STOP (cached)
   ================================================================== */
void tv5725_set_segment(uint8_t segment)
{
    if (g_tv5725_cfg.i2c_cur_seg == segment)
        return;
    uint8_t buf[2] = {TV5725_SEG_REG, segment};
    I2C_Master_Transmit(TV_WR_ADDR, buf, 2, V_TIMEOUT);
    g_tv5725_cfg.i2c_cur_seg = segment;
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
   gbs-control ofw_RGBS S5 段预设 (S5_00~S5_6F, 112 bytes)
   参考 gbs-control/ofw_RGBS.h，S5_20 改为 0xD8 (EXT_SYNC=1)
   ================================================================== */
static const uint8_t s5_rgbs_preset[112] = {
    0xD8, 0x00, 0x57, 0xF1, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x7F, 0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x90, 0xB3, 0xC6, 0x00, 0x00, 0x20, 0xCE, 0x85, 0x82, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04,
    0xD8, 0x20, 0x0F, 0x00, 0x40, 0x00, 0x05, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x04, 0x00, 0x04,
    0x00, 0x2F, 0x00, 0x28, 0x03, 0x15, 0x00, 0x60, 0x06, 0x07, 0x0A, 0x00, 0x00, 0x00, 0xC0, 0x03,
    0x0B, 0x27, 0x06, 0x7E, 0x06, 0x00, 0xC0, 0x05, 0xC0, 0x04, 0xC0, 0x34, 0xC0, 0x67, 0xC0, 0x67,
    0xC0, 0x00, 0xC0, 0x05, 0xC0, 0xC0, 0x21, 0xC0, 0x05, 0xC0, 0x01, 0xC8, 0x06, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* ==================================================================
   Input mode config
   ================================================================== */

static void input_adc_common(void)
{
    /* ADC clock: 参考 gbs-control ofw_RGBS S5_00=0xD8 (PA=3, ICLK1X=1) */
    tv5725_reg_write(TV5725_RW_CONTROL_ADC_CLK_00, 0xD8);

    /* ADC input: enable SOG (required for all modes per gbs-control) */
    tv5725_reg_write(TV5725_RW_ADC_SOGEN, 1);

    /* ADC power up, Red Y-select=1 (sync from R), FLTR=01 — 参考 gbs S5_03=0xF1 */
    tv5725_reg_write(TV5725_RW_ADC_POWDZ, 1);
    tv5725_reg_write(TV5725_RW_ADC_RYSEL_R, 1);
    tv5725_reg_write(TV5725_RW_ADC_RYSEL_G, 0);
    tv5725_reg_write(TV5725_RW_ADC_RYSEL_B, 0);
    tv5725_reg_write(TV5725_RW_ADC_FLTR, 0x01);

    /* PLLAD: power up, bypass on (PLL configured by preset), normal frequency */
    tv5725_reg_write(TV5725_RW_PLLAD_PDZ, 1);
    tv5725_reg_write(TV5725_RW_PLLAD_BPS, 1);
    tv5725_reg_write(TV5725_RW_PLLAD_FS, 0);
}

int32_t tv5725_input_config_vga(void)
{
    /* ASW: VGA */
    tv5725_asw_set_vga();

    input_adc_common();

    /* 复位同步处理器状态机（移植自 gbs-control resetSyncProcessor） */
    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 0);
    for (volatile uint32_t i = 0; i < 200; i++)
        continue; /* ~10µs @ 200MHz */
    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 1);

    /* VGA: ADC channel 1, separate H/V sync, auto polarity */
    tv5725_reg_write(TV5725_RW_ADC_INPUT_SEL, 1);
    tv5725_reg_write(TV5725_RW_ADC_SOGEN, 1);
    tv5725_reg_write(TV5725_RW_SP_SOG_SRC_SEL, 0);
    tv5725_reg_write(TV5725_RW_SP_SOG_MODE, 0);
    tv5725_reg_write(TV5725_RW_SP_EXT_SYNC_SEL, 0); /* HS_HS: H from H pin, V from V pin */
    tv5725_reg_write(TV5725_RW_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_VS_POL_ATO, 1);

    /* VGA: 无海岸线，同步保护关闭 */
    tv5725_reg_write(TV5725_RW_SP_NO_COAST_REG, 1);
    tv5725_reg_write(TV5725_RW_SP_PRE_COAST, 0);
    tv5725_reg_write(TV5725_RW_SP_POST_COAST, 0);
    tv5725_reg_write(TV5725_RW_SP_H_PROTECT, 0);
    tv5725_reg_write(TV5725_RW_SP_H_PULSE_IGNOR, 0xFF);
    tv5725_reg_write(TV5725_RW_SP_CLAMP_MANUAL, 1);
    tv5725_reg_write(TV5725_RW_SP_DIS_SUB_COAST, 1);

    /* 告知 TV5725 输入为 RGBHV 逐行，禁用去隔行 */
    tv5725_reg_write(TV5725_RW_GBS_OPTION_SCALING_RGBHV, 1);

    /* IF: CCIR601, 8-bit源, 24-bit数据路径, 旁路IF矩阵, 旁路数据寄存器 */
    tv5725_reg_write(TV5725_RW_IF_SEL_656, 0);
    tv5725_reg_write(TV5725_RW_IF_SEL16BIT, 0);
    tv5725_reg_write(TV5725_RW_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_RW_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_RW_IF_IN_DREG_BYPS, 1);

    /* DEC: RGB输入旁路解码器YUV→RGB转换 */
    tv5725_reg_write(TV5725_RW_DEC_MATRIX_BYPS, 1);

    g_tv5725_cfg.input_mode = TV5725_INPUT_VGA;
    return LL_OK;
}

int32_t tv5725_input_config_rgbs(void)
{
    tv5725_asw_set_rgbs();

    /* 全量写入 gbs-control ofw_RGBS S5 段预设 (S5_00~S5_6F) */
    for (uint8_t i = 0; i < 7; i++)
        tv5725_write_buf(0x05, (uint8_t)(i * 16), s5_rgbs_preset + i * 16, 16);

    /* 恢复自动调谐找到的最佳 SOG 阈值 */
    if (g_tv5725_cfg.sog_tune_best >= 0)
        tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, g_tv5725_cfg.sog_tune_best);

    /* PLLAD (不在 S5 段) */
    tv5725_reg_write(TV5725_RW_PLLAD_PDZ, 1);
    tv5725_reg_write(TV5725_RW_PLLAD_BPS, 1);
    tv5725_reg_write(TV5725_RW_PLLAD_FS, 0);

    /* 复位同步处理器状态机 */
    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 0);
    for (volatile uint32_t i = 0; i < 200; i++)
        continue;
    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 1);

    /* IF: 24-bit, 旁路矩阵和数据寄存器 */
    tv5725_reg_write(TV5725_RW_IF_SEL_656, 0);
    tv5725_reg_write(TV5725_RW_IF_SEL16BIT, 0);
    tv5725_reg_write(TV5725_RW_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_RW_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_RW_IF_IN_DREG_BYPS, 1);

    /* DEC: RGB输入旁路解码器 */
    tv5725_reg_write(TV5725_RW_DEC_MATRIX_BYPS, 1);

    g_tv5725_cfg.input_mode = TV5725_INPUT_RGBS;
    return LL_OK;
}

int32_t tv5725_input_config_rgsb(void)
{
    /* ASW: RGsB */
    tv5725_asw_set_rgsb();

    input_adc_common();

    /* 复位同步处理器状态机 */
    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 0);
    for (volatile uint32_t i = 0; i < 200; i++)
        continue; /* ~10µs @ 200MHz */
    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 1);

    /* RGsB: ADC channel 0 (SOG0), SOG mode, auto polarity */
    tv5725_reg_write(TV5725_RW_ADC_INPUT_SEL, 0);
    tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, 5);
    tv5725_reg_write(TV5725_RW_ADC_SOGEN, 1);
    tv5725_reg_write(TV5725_RW_SP_SOG_SRC_SEL, 0);
    tv5725_reg_write(TV5725_RW_SP_EXT_SYNC_SEL, 1);
    tv5725_reg_write(TV5725_RW_SP_SOG_MODE, 1);
    tv5725_reg_write(TV5725_RW_SP_SOG_P_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_RW_SP_VS_POL_ATO, 1);

    /* RGsB: SOG 同步，海岸线使能 */
    tv5725_reg_write(TV5725_RW_SP_NO_COAST_REG, 0);
    tv5725_reg_write(TV5725_RW_SP_PRE_COAST, 4);
    tv5725_reg_write(TV5725_RW_SP_POST_COAST, 7);
    tv5725_reg_write(TV5725_RW_SP_H_PROTECT, 1);
    tv5725_reg_write(TV5725_RW_SP_H_PULSE_IGNOR, 0xFF);
    tv5725_reg_write(TV5725_RW_SP_CLAMP_MANUAL, 1);
    tv5725_reg_write(TV5725_RW_SP_DIS_SUB_COAST, 1);

    /* IF: CCIR601, 8-bit源, 24-bit数据路径, 旁路IF矩阵, 旁路数据寄存器 */
    tv5725_reg_write(TV5725_RW_IF_SEL_656, 0);
    tv5725_reg_write(TV5725_RW_IF_SEL16BIT, 0);
    tv5725_reg_write(TV5725_RW_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_RW_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_RW_IF_IN_DREG_BYPS, 1);

    /* DEC: RGB输入旁路解码器YUV→RGB转换 */
    tv5725_reg_write(TV5725_RW_DEC_MATRIX_BYPS, 1);

    return LL_OK;
}

void tv5725_input_auto_detect(void)
{
    tv5725_detect_active_input();
}

/* ==================================================================
   ASW 步进切换 — 每次调用切换到下一个组合，观察同步状态变化
   On=0(LOW), Off=1(HIGH)
   ================================================================== */
void tv5725_asw_sweep_diag(void)
{
    if (g_tv5725_cfg.asw_sweep_first)
    {
        printf("=== ASW Step (sw1 sw2 sw3 sw4) ===\n");
        printf("  On=0(LOW) Off=1(HIGH)\n");
        g_tv5725_cfg.asw_sweep_first = false;
        g_tv5725_cfg.asw_sweep_combo = 0;
    }

    bool s1 = (g_tv5725_cfg.asw_sweep_combo >> 3) & 1;
    bool s2 = (g_tv5725_cfg.asw_sweep_combo >> 2) & 1;
    bool s3 = (g_tv5725_cfg.asw_sweep_combo >> 1) & 1;
    bool s4 = g_tv5725_cfg.asw_sweep_combo & 1;

    tv5725_asw_ctrl(s1, s2, s3, s4);

    /* 复位同步处理器 */
    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 0);
    for (volatile uint32_t i = 0; i < 200; i++)
        continue;
    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 1);

    /* 等待稳定 (~50ms) */
    for (volatile uint32_t d = 0; d < 1000000; d++)
        continue;

    uint8_t st05, st09, st0F;
    tv5725_read_byte(0x00, 0x05, &st05);
    tv5725_read_byte(0x00, 0x09, &st09);
    tv5725_read_byte(0x00, 0x0F, &st0F);

    uint8_t no_sync = (st05 >> 1) & 1;
    uint8_t ad_lock = (st09 >> 7) & 1;
    uint8_t sog_ok = (st0F >> 2) & 1;
    uint8_t sog_sw = (st0F >> 1) & 1;
    uint8_t sog_bad = st0F & 1;

    printf("  #%2d: %d %d %d %d  NO_SYNC=%d AD_LOCK=%d SOG:B=%d S=%d OK=%d (05=0x%02X 09=0x%02X 0F=0x%02X)",
           g_tv5725_cfg.asw_sweep_combo, s1, s2, s3, s4, no_sync, ad_lock,
           sog_bad, sog_sw, sog_ok, st05, st09, st0F);

    if (!no_sync || sog_ok || ad_lock)
        printf(" ***");

    printf("\n");

    g_tv5725_cfg.asw_sweep_combo++;
    if (g_tv5725_cfg.asw_sweep_combo >= 16)
    {
        printf("ASW loop done, restarting from #0\n");
        g_tv5725_cfg.asw_sweep_combo = 0;
        g_tv5725_cfg.asw_sweep_first = true;
    }
}

void tv5725_asw_sweep_reset(void)
{
    g_tv5725_cfg.asw_sweep_combo = 0;
    g_tv5725_cfg.asw_sweep_first = true;
    printf("ASW step reset to #0\n");
}

/* ==================================================================
   SOG 阈值校准 — 扫描 SOGCTRL 0~31，寻找能使 SOG 锁定的阈值
   ================================================================== */
void tv5725_sog_calibrate(void)
{
    uint8_t sog_save;
    tv5725_read_byte(0x05, 0x02, &sog_save);

    printf("=== SOG Calibrate (SOGCTRL 0~31) ===\n");
    int ok_count = 0;
    for (int level = 0; level < 32; level++)
    {
        tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, level);

        /* 复位同步处理器，确保 SOG 状态机从干净状态启动 */
        tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 0);
        for (volatile uint32_t i = 0; i < 200; i++)
            continue;
        tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 1);

        /* 等待 SOG 稳定 (~50ms) */
        for (volatile uint32_t d = 0; d < 1000000; d++)
            continue;

        uint8_t st0F;
        tv5725_read_byte(0x00, 0x0F, &st0F);
        uint8_t sog_ok = (st0F >> 2) & 1;
        uint8_t sog_sw = (st0F >> 1) & 1;
        uint8_t sog_bad = st0F & 1;

        if (sog_ok)
        {
            printf("  SOGCTRL=%2d: OK\n", level);
            ok_count++;
        }
        else if (level == 0 || level == 5 || level == 10 || level == 13 || level == 20 || level == 31)
        {
            printf("  SOGCTRL=%2d: BAD=%d SW=%d OK=%d (0x0F=0x%02X)\n",
                   level, sog_bad, sog_sw, sog_ok, st0F);
        }
    }

    /* 恢复原值 */
    tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, (sog_save >> 1) & 0x1F);
    printf("SOG calibrate done: %d/32 levels locked\n", ok_count);
}

/* ==================================================================
   SOG 自动调谐 — 主循环中调用，信号未锁时逐步扫 SOGCTRL
   ================================================================== */
void tv5725_sog_auto_tune(void)
{
    if (g_tv5725_cfg.input_mode == TV5725_INPUT_AUTO)
        return;

    uint8_t st05, st0F;
    tv5725_read_byte(0x00, 0x05, &st05);
    tv5725_read_byte(0x00, 0x0F, &st0F);
    uint8_t no_sync = (st05 >> 1) & 1;
    uint8_t sog_ok = (st0F >> 2) & 1;

    if (no_sync == 0 || sog_ok)
    {
        if (g_tv5725_cfg.sog_tune_level >= 0)
        {
            uint8_t s5_02;
            tv5725_read_byte(0x05, 0x02, &s5_02);
            g_tv5725_cfg.sog_tune_best = (s5_02 >> 1) & 0x1F;
            printf("SOG auto: LOCKED at SOGCTRL=%d (0x%02X)\n",
                   g_tv5725_cfg.sog_tune_best, s5_02);
        }
        g_tv5725_cfg.sog_tune_level = -1;
        return;
    }

    if (g_tv5725_cfg.sog_tune_level < 0)
    {
        g_tv5725_cfg.sog_tune_level = 0;
        printf("SOG auto: start scan\n");
    }
    else
    {
        g_tv5725_cfg.sog_tune_level++;
        if (g_tv5725_cfg.sog_tune_level > 31)
        {
            printf("SOG auto: full scan no lock, restart\n");
            g_tv5725_cfg.sog_tune_level = 0;
        }
    }

    tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, g_tv5725_cfg.sog_tune_level);

    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 0);
    for (volatile uint32_t i = 0; i < 200; i++)
        continue;
    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 1);
}

void tv5725_diag(void)
{
    uint8_t st05, st09, st0A, st0F, st10, st11, st13, st16;

    tv5725_read_byte(0x00, 0x05, &st05);
    tv5725_read_byte(0x00, 0x09, &st09);
    tv5725_read_byte(0x00, 0x0A, &st0A);
    tv5725_read_byte(0x00, 0x0F, &st0F);
    tv5725_read_byte(0x00, 0x10, &st10);
    tv5725_read_byte(0x00, 0x11, &st11);
    tv5725_read_byte(0x00, 0x13, &st13);
    tv5725_read_byte(0x00, 0x16, &st16);

    printf("=== TV5725 Diag ===\n");
    printf("ASW:    sw1=%d sw2=%d sw3=%d sw4=%d\n",
           g_tv5725_cfg.asw_01, g_tv5725_cfg.asw_02,
           g_tv5725_cfg.asw_03, g_tv5725_cfg.asw_04);
    printf("INPUT:  NO_SYNC=%d HT_BAD=%d VT_BAD=%d (0x05=0x%02X)\n",
           (st05 >> 1) & 1, (st05 >> 2) & 1, (st05 >> 3) & 1, st05);
    uint8_t sog_reg;
    tv5725_read_byte(0x05, 0x02, &sog_reg);
    printf("SOG:    BAD=%d SW=%d OK=%d SOGCTRL=%d (0x0F=0x%02X, S5_02=0x%02X)\n",
           st0F & 1, (st0F >> 1) & 1, (st0F >> 2) & 1,
           (sog_reg >> 1) & 0x1F, st0F, sog_reg);
    printf("PLL:    AD_LOCK=%d P648_LOCK=%d (0x09=0x%02X)\n",
           (st09 >> 7) & 1, (st09 >> 6) & 1, st09);
    printf("SYNC:   HSYNC=%d VSYNC=%d VBLK=%d HBLK=%d (0x0A=0x%02X)\n",
           (st0A >> 7) & 1, (st0A >> 6) & 1, (st0A >> 4) & 1, (st0A >> 5) & 1, st0A);
    printf("VDS:    OUT_HS=%d OUT_VS=%d FIELD=%d BLANK=%d (0x10=0x%02X, 0x11=0x%02X)\n",
           (st10 >> 5) & 1, (st10 >> 4) & 1, st11 & 1, (st11 >> 1) & 1, st10, st11);
    printf("FIFO:   WFF_EMPTY=%d RFF_EMPTY=%d (0x13=0x%02X)\n",
           (st13 >> 1) & 1, (st13 >> 3) & 1, st13);
    printf("POL:    HS_POL=%d HS_ACT=%d VS_POL=%d VS_ACT=%d (0x16=0x%02X)\n",
           st16 & 1, (st16 >> 1) & 1, (st16 >> 2) & 1, (st16 >> 3) & 1, st16);

    uint8_t ht_l, ht_h, vt_l, vt_h;
    tv5725_read_byte(0x00, 0x17, &ht_l);
    tv5725_read_byte(0x00, 0x18, &ht_h);
    tv5725_read_byte(0x00, 0x1B, &vt_l);
    tv5725_read_byte(0x00, 0x1C, &vt_h);

    uint16_t htotal = ht_l | ((ht_h & 0x0F) << 8);
    uint16_t vtotal = vt_l | ((vt_h & 0x07) << 8);
    printf("MEAS:   H-total=%u V-total=%u\n", htotal, vtotal);
}

/* ==================================================================
   SDRAM / 帧缓存诊断

   检查 SDRAM 是否正常工作:
   - PLL648 锁定状态 (SDRAM 时钟)
   - 写 FIFO / 读 FIFO 使能和空满状态
   - SDRAM 控制器复位和初始化状态
   - 判断: SDRAM 未就绪时 VDS 输出黑屏
   ================================================================== */
void tv5725_sdram_diag(void)
{
    uint8_t st09, st13, st14, s4_00, s4_1b, s4_42, s4_4d;

    tv5725_read_byte(0x00, 0x09, &st09);
    tv5725_read_byte(0x00, 0x13, &st13);
    tv5725_read_byte(0x00, 0x14, &st14);
    tv5725_read_byte(0x04, 0x00, &s4_00);
    tv5725_read_byte(0x04, 0x1B, &s4_1b);
    tv5725_read_byte(0x04, 0x42, &s4_42);
    tv5725_read_byte(0x04, 0x4D, &s4_4d);

    uint8_t pll648_lock = (st09 >> 6) & 1;
    uint8_t wff_full    = st13 & 1;
    uint8_t wff_empty   = (st13 >> 1) & 1;
    uint8_t rff_full    = (st13 >> 2) & 1;
    uint8_t rff_empty   = (st13 >> 3) & 1;
    uint8_t cap_full    = (st13 >> 4) & 1;
    uint8_t cap_empty   = (st13 >> 5) & 1;
    uint8_t wff_en      = s4_42 & 1;
    uint8_t wff_sta_inv = (s4_42 >> 2) & 1;
    uint8_t rff_en      = (s4_4d >> 7) & 1;
    uint8_t sdram_rst   = (s4_00 >> 4) & 1;
    uint8_t sdram_init  = (s4_00 >> 7) & 1;

    /* WFF 状态位在 WFF_FF_STA_INV=1 时取反 */
    uint8_t wff_full_real  = wff_sta_inv ? !wff_full  : wff_full;
    uint8_t wff_empty_real = wff_sta_inv ? !wff_empty : wff_empty;

    printf("=== SDRAM Diag ===\n");
    printf("CLK:    PLL648_LOCK=%d %s\n", pll648_lock,
           pll648_lock ? "(OK)" : "(NO CLK!)");
    printf("FIFO-R: RFF_EN=%d EMPTY=%d FULL=%d (S4_4D=0x%02X)\n",
           rff_en, rff_empty, rff_full, s4_4d);
    printf("FIFO-W: WFF_EN=%d EMPTY(r)=%d FULL(r)=%d (raw=%d/%d inv=%d, S4_42=0x%02X)\n",
           wff_en, wff_empty_real, wff_full_real,
           wff_empty, wff_full, wff_sta_inv, s4_42);
    printf("FIFO-C: CAP_EMPTY=%d CAP_FULL=%d (0x13=0x%02X, 0x14=0x%02X)\n",
           cap_empty, cap_full, st13, st14);
    printf("CTRL:   RESET=%d INIT_CYCLE=%d TIMING=0x%02X (S4_00=0x%02X)\n",
           sdram_rst, sdram_init, s4_1b, s4_00);

    /* verdict */
    if (!pll648_lock)
        printf("=> FAIL: PLL648 not locked, SDRAM has no clock\n");
    else if (!wff_en)
        printf("=> FAIL: Write FIFO not enabled, data cannot enter SDRAM\n");
    else if (!rff_en)
        printf("=> FAIL: Read FIFO not enabled, VDS cannot read data\n");
    else if (rff_empty)
        printf("=> FAIL: Read FIFO empty, SDRAM has no valid data output -> black screen\n");
    else if (wff_full_real)
        printf("=> WARN: Write FIFO full, input rate > output rate\n");
    else if (wff_empty_real)
        printf("=> WARN: Write FIFO empty, no data from capture side\n");
    else
        printf("=> OK: SDRAM data flow normal\n");
}

int32_t tv5725_input_set_mode(tv5725_input_mode_t mode)
{
    g_tv5725_cfg.sog_tune_level = -1; /* 输入模式切换后重新扫描 */
    g_tv5725_cfg.sog_tune_best = -1;  /* 最佳档位失效，重新寻找 */

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

    /* 通用 Clamp 配置 */
    tv5725_reg_write(TV5725_RW_SP_NO_CLAMP_REG, 1);
    tv5725_reg_write(TV5725_RW_SP_COAST_INV_REG, 0);
    tv5725_reg_write(TV5725_RW_SP_HCST_AUTO_EN, 0);
    tv5725_reg_write(TV5725_RW_SP_HS_REG, 1);
}

/* ==================================================================
   YPbPr 输出配置（独立函数封装）

   与预设加载分离，专门配置 VDS、DAC 和色彩矩阵
   使芯片输出 YPbPr 分量信号。修改输出不影响输入侧代码。
   ================================================================== */

static void tv5725_output_config_ypbpr(void)
{
    /* 保留预设的 PAD 寄存器值，仅按位修改同步控制 */
    /* OUT_SYNC_CNTRL=1: 输出同步路由 (gbs-control doPostPresetLoadSteps 第1236行) */
    tv5725_reg_write(TV5725_RW_OUT_SYNC_CNTRL, 1);

    /* DAC 数据路径: 经 VDS 处理 (RGB→YUV 转换) */
    tv5725_reg_write(TV5725_RW_DAC_RGBS_BYPS2DAC, 0); /* VDS→DAC */
    tv5725_reg_write(TV5725_RW_DAC_RGBS_ADC2DAC, 0);  /* ADC 不直通 DAC */

    /* VDS: 使能 RGB→YUV 转换, 嵌入同步到 Y */
    tv5725_reg_write(TV5725_RW_VDS_CONVT_BYPS, 0); /* 不禁用 RGB→YUV */
    tv5725_reg_write(TV5725_RW_VDS_DYN_BYPS, 0);
    tv5725_reg_write(TV5725_RW_PIP_CONVT_BYPS, 0);

    /* 色差矩阵系数 (ITU-R BT.601, 移植自 gbs-control applyComponentColorMixing) */
    tv5725_reg_write(TV5725_RW_VDS_Y_GAIN, 0x64);
    tv5725_reg_write(TV5725_RW_VDS_UCOS_GAIN, 0x19);
    tv5725_reg_write(TV5725_RW_VDS_VCOS_GAIN, 0x19);
    tv5725_reg_write(TV5725_RW_VDS_Y_OFST, 0xFE);
    tv5725_reg_write(TV5725_RW_VDS_U_OFST, 0x01);
    tv5725_reg_write(TV5725_RW_VDS_V_OFST, 0x00);
    tv5725_reg_write(TV5725_RW_VDS_SYNC_LEV, 0x0E0);

    /* DAC 掉电解除 (预设已配好 R0ENZ/G0ENZ/B0ENZ, 不覆盖) */
    tv5725_reg_write(TV5725_RW_DAC_RGBS_PWDNZ, 1);
}

/* ==================================================================
   输出路径初始化 — 加载预设 + 调用 YPbPr 输出配置

   负责 SDRAM 复位、加载预设、恢复 SDRAM 时序、配置同步，
   然后调用 tv5725_output_config_ypbpr() 设置 YPbPr 输出。

   @param preset      预设数据指针（432 字节 GBSCpro 格式）
   ================================================================== */

int32_t tv5725_output_path_init(const uint8_t *preset)
{
    g_tv5725_cfg.cur_preset = preset;

    uint8_t s4_1b_saved;
    tv5725_read_byte(0x04, 0x1B, &s4_1b_saved);

    /* SDRAM 复位序列 (移植自 gbs-control ResetSDRAM) */
    tv5725_write_byte(0x04, 0x00, 0x02);                        /* S4_00=0x02, 清除 INIT_CYCLE */
    tv5725_reg_write(TV5725_RW_SDRAM_RESET_SIGNAL, 1);          /* 复位 SDRAM 控制器 */
    tv5725_reg_write(TV5725_RW_SDRAM_RESET_SIGNAL, 0);          /* 释放复位 */
    SysTick_Delay(1);

    tv5725_load_preset(preset);                                 /* 预设写入所有 S4 寄存器 */

    /* 预设先写 S4_00=0x82 再写 S4_01~S4_5F, INIT_CYCLE 在时序参数就绪前就启动了。
       重新触发 INIT_CYCLE，确保所有 SDRAM 参数已就位再启动初始化 */
    tv5725_write_byte(0x04, 0x00, 0x02);                        /* 清除 INIT_CYCLE */
    SysTick_Delay(1);
    tv5725_write_byte(0x04, 0x00, 0x82);                        /* 重新启动 INIT_CYCLE */

    /* 等待 INIT_CYCLE 硬件自动完成 */
    {
        uint8_t s4_00;
        int timeout = 500;
        do {
            tv5725_read_byte(0x04, 0x00, &s4_00);
        } while ((s4_00 & 0x80) && --timeout > 0);
        if (timeout == 0)
            printf("WARN: SDRAM INIT_CYCLE timeout, S4_00=0x%02X\n", s4_00);
    }

    if (s4_1b_saved != 0)
        tv5725_write_byte(0x04, 0x1B, s4_1b_saved);            /* 恢复 SDRAM 时序校准值 */

    /* 预设写入时 S4_42/S4_4D 的 FIFO 使能位为 0，
       INIT_CYCLE 完成后需显式使能读写 FIFO */
    tv5725_reg_write(TV5725_RW_WFF_ENABLE, 1);
    tv5725_reg_write(TV5725_RW_RFF_ENABLE, 1);

    tv5725_sync_config();
    tv5725_output_config_ypbpr();

    /* 预设加载覆盖了 S5 段 ADC 寄存器（输入通道/SOG等），
       立即恢复当前输入模式以保持输入配置一致 */
    tv5725_input_set_mode(g_tv5725_cfg.input_mode);

    return LL_OK;
}

/* ==================================================================
   ASW analog switch — PB12-PB15
   ================================================================== */

void tv5725_asw_init(void)
{
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDrv = PIN_HIGH_DRV;

    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW1, &stcGpioInit);
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW2, &stcGpioInit);
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW3, &stcGpioInit);
    GPIO_Init(TV5725_SYNC_ASW_PORT, TV5725_SYNC_ASW4, &stcGpioInit);

    /* Default: all off (reset state) */
    ASW1_Off();
    ASW2_Off();
    ASW3_Off();
    ASW4_Off();
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
    tv5725_asw_ctrl(1, 1, 1, 1);
}

void tv5725_asw_set_rgbs(void)
{
    tv5725_asw_ctrl(0, 0, 0, 1);
}

void tv5725_asw_set_rgsb(void)
{
    tv5725_asw_ctrl(0, 0, 1, 0);
}

/* ==================================================================
   同步检测 — 移植自 gbs-control detectAndSwitchToActiveInput()

   核心思路：
   - 读 ADC_INPUT_SEL 确定当前同步通道（0=SOG0 1=SOG1）
   - 等待 HSync 稳定 (getStatus16SpHsStable)
   - 通道1: 查 VS_ACT 区分 VGA(独立H/V) vs RGBS(复合同步)
           有VSync → VGA,  无VSync → CSync(SOG扫描)
   - 通道0: RGsB/YPbPr → SOG扫描
   ================================================================== */

/* 检查同步处理器 HSync 是否稳定
   对应 gbs-control getStatus16SpHsStable() */
static bool tv5725_get_sync_stable(void)
{
    uint8_t st16, st05;
    tv5725_read_byte(0x00, 0x16, &st16);
    tv5725_read_byte(0x00, 0x05, &st05);

    if ((st05 >> 1) & 1) /* NO_SYNC = 1 */
        return false;
    if ((st16 >> 1) & 1) /* HS_ACT = 1 */
        return true;
    return false;
}

/* 获取当前检测到的视频模式
   对应 gbs-control getVideoMode() — 简化版
   返回 0=未检测到, 非0=已检测到有效模式 */
static uint8_t tv5725_get_video_mode(void)
{
    uint8_t st00, st01, st04, st05;
    tv5725_read_byte(0x00, 0x00, &st00);
    tv5725_read_byte(0x00, 0x01, &st01);
    tv5725_read_byte(0x00, 0x04, &st04);
    tv5725_read_byte(0x00, 0x05, &st05);

    if ((st05 >> 1) & 1) /* NO_SYNC still asserted? */
        return 0;

    /* 检查 IF 状态: IF_HVT_OK + IF_HT_OK + IF_VT_OK */
    if ((st00 & 0x07) == 0x07)
    {
        /* VGA/SVGA/XGA 等 PC 模式 */
        if (st01 & 0x08)
            return 15; /* IF_INP_VGA */
        if (st01 & 0x80)
            return 15; /* IF_INP_SVGA */
        if (st04 & 0x08)
            return 15; /* IF_INP_2640_1125P etc */
        /* SD/HD 视频模式 */
        if (st00 & 0x80)
            return 1; /* SD 480i/576i */
        if (st04 & 0x20)
            return 6; /* 480p */
        if (st04 & 0x40)
            return 8; /* 1080i */
        return 7;     /* 720p 等 */
    }

    /* 部分检测 */
    if ((st00 & 0x2F) == 0x07)
        return 15;

    return 0;
}

/* 检测并切换到活动输入源
   对应 gbs-control detectAndSwitchToActiveInput()
   返回: true=检测到活动输入并完成配置, false=未检测到 */
bool tv5725_detect_active_input(void)
{
    uint8_t s5_02, st16;

    /* 读取当前 ADC 输入选择 (S5_02[7:6]) */
    tv5725_read_byte(0x05, 0x02, &s5_02);
    uint8_t adc_sel = (s5_02 >> 6) & 0x03;

    printf("ADC_INPUT_SEL=%d\n", adc_sel);

    /* 等待同步稳定 (超时 ~450ms) */
    {
        int32_t timeout = 450;
        bool stable = false;
        while (timeout > 0)
        {
            if (tv5725_get_sync_stable())
            {
                stable = true;
                break;
            }
            SysTick_Delay(1);
            timeout--;
        }
        if (!stable)
            printf("  HS unstable\n");
    }

    if (adc_sel == 1) /* RGBS 或 RGBHV */
    {
        /* 初始 SOG 阈值 (gbs 默认 13) */
        tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, 13);

        /* 检查 VSync 活动 — 区分 VGA(独立同步) 和 RGBS(复合同步) */
        tv5725_read_byte(0x00, 0x16, &st16);
        uint8_t vs_act = (st16 >> 3) & 1;

        if (vs_act)
        {
            /* === VSync 存在 → VGA (分离 H/V 同步) === */
            tv5725_reg_write(TV5725_RW_MD_SEL_VGA60, 1);

            /* 等待 HSync 活动 */
            int32_t timeout = 400;
            uint8_t hs_act = 0;
            while (timeout > 0)
            {
                tv5725_read_byte(0x00, 0x16, &st16);
                hs_act = (st16 >> 1) & 1;
                if (hs_act)
                    break;
                SysTick_Delay(1);
                timeout--;
            }

            if (hs_act)
            {
                tv5725_reg_write(TV5725_RW_SP_H_PROTECT, 1);
                SysTick_Delay(120);

                /* 尝试模式检测 — 迭代 MD_HD1250P_CNTRL */
                for (uint8_t i = 0; i < 16; i++)
                {
                    uint8_t mode = tv5725_get_video_mode();
                    if (mode == 8) /* med-res found */
                    {
                        printf("  VGA mode detected (med-res)\n");
                        tv5725_input_config_vga();
                        return true;
                    }
                    tv5725_reg_write(TV5725_RW_MD_HD1250P_CNTRL,
                                     (tv5725_reg_read(TV5725_RW_MD_HD1250P_CNTRL) + 1) & 0x7F);
                    SysTick_Delay(30);
                }

                printf("  VGA mode detected\n");
                tv5725_input_config_vga();
                return true;
            }
        }
        else
        {
            /* === VSync 不存在 → CSync (复合同步) === */
            tv5725_reg_write(TV5725_RW_MD_SEL_VGA60, 0);

            printf("  CSync detect, sweeping SOG...\n");

            /* SOG 扫描 (1→2→4→6...→15, 每 150ms 一档)
               对应 gbs 的 SOG sweep 逻辑 */
            int32_t timeout_6s = 6000;
            uint16_t cycle = 0;
            int8_t sog = 1;

            while (timeout_6s > 0)
            {
                SysTick_Delay(2);
                timeout_6s -= 2;

                uint8_t mode = tv5725_get_video_mode();
                if (mode > 0 && mode != 8)
                {
                    printf("  CSync mode=%d at SOGCTRL=%d\n", mode, sog);
                    tv5725_input_config_rgbs();
                    /* 设置找到的最佳 SOG 阈值 */
                    tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, sog);
                    g_tv5725_cfg.sog_tune_best = sog;
                    g_tv5725_cfg.sog_tune_level = -1;
                    return true;
                }

                cycle++;
                if ((cycle % 150) == 0)
                {
                    sog = (sog == 1) ? 2 : sog + 2;
                    if (sog >= 15)
                        sog = 1;
                    tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, sog);

                    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 0);
                    for (volatile uint32_t k = 0; k < 200; k++)
                        continue;
                    tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 1);
                }

                /* 中分辨率检测 8 = 1080i */
                if (mode == 8)
                {
                    tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, 13);
                    g_tv5725_cfg.sog_tune_best = 13;
                    printf("  CSync mode=8 (1080i), SOGCTRL=13\n");
                    tv5725_input_config_rgbs();
                    return true;
                }

                /* 调整 MD_HD1250P_CNTRL */
                uint8_t cntrl = tv5725_reg_read(TV5725_RW_MD_HD1250P_CNTRL) & 0x7F;
                if (cntrl < 0x3C)
                    tv5725_reg_write(TV5725_RW_MD_HD1250P_CNTRL, cntrl + 1);
                else
                    tv5725_reg_write(TV5725_RW_MD_HD1250P_CNTRL, 0x33);
            }

            printf("  CSync timeout, default RGBS\n");
            tv5725_input_config_rgbs();
            return true;
        }
    }
    else if (adc_sel == 0) /* RGsB 或 YPbPr */
    {
        tv5725_reg_write(TV5725_RW_MD_SEL_VGA60, 0);

        printf("  RGsB/YPbPr detect, sweeping SOG...\n");

        int32_t timeout_6s = 6000;
        uint16_t cycle = 0;
        int8_t sog = 1;

        while (timeout_6s > 0)
        {
            SysTick_Delay(2);
            timeout_6s -= 2;

            uint8_t mode = tv5725_get_video_mode();
            if (mode > 0)
            {
                printf("  RGsB mode=%d at SOGCTRL=%d\n", mode, sog);
                tv5725_input_config_rgsb();
                tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, sog);
                g_tv5725_cfg.sog_tune_best = sog;
                g_tv5725_cfg.sog_tune_level = -1;
                return true;
            }

            cycle++;
            if ((cycle % 180) == 0)
            {
                sog = (sog == 1) ? 2 : sog + 2;
                if (sog >= 16)
                    sog = 1;
                tv5725_reg_write(TV5725_RW_ADC_SOGCTRL, sog);

                tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 0);
                for (volatile uint32_t k = 0; k < 200; k++)
                    continue;
                tv5725_reg_write(TV5725_RW_SFTRST_SYNC_RSTZ, 1);
            }
        }

        printf("  RGsB timeout\n");
        return false;
    }

    printf("  No input detected\n");
    return false;
}

/*
RGB模拟输入，模拟输出

S0_48'0 = 0 禁用VB_[7:0]（test_out_[7:0]）输出
S0_48'1 = 1 禁用 VB_[7:0] 输入
S0_48'2 = 0 禁用VR_[7:0]（test_out_[15:8]）输出
S0_48'3 = 1 禁用VR_[7:0]输入
S0_48'4 = 0 禁用VG_[7:0]（test_out_[23:16]）输出
S0_48'5 = 1 禁用VG_[7:0]输入

S0_49'1 = 1 禁用 CLKOUT 输出
S0_49'2 = 0 启用 HSOUT / VSOUT 输出
S0_49'3 = 0 启用 HBOUT / VBOUT 输出
S0_50'0 = 0 输出 VBOUT 垂直空白
S3_50'7 = 0 数字输出为24位
S1_28'2 = 1 选择ADC同步至数据路径
S1_00'3 = 0 输入为 CCIR 601模式。选择CCIR601模式计时
S1_00'4 = 0 源数据8位656/601输入
S1_01'7 = 1 选择24位数据路径

S5_02'7:6 = 01 ADC以R1/G1/B1/SOG1作为输入

*/
