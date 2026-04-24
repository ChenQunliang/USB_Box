#include "tv5725.h"
#include "i2c.h"
#include <string.h>

/* Current segment cache to avoid unnecessary segment register writes */
static uint8_t g_tv5725_cur_seg = 0xFF;

/* -------------------------------------------------------------------
   Low-level I2C helpers (use V_I2C hardware)
   ------------------------------------------------------------------- */

/*
 * Write a register offset + value(s) to the TV5725.
 * Buf contains [offset, data0, data1, ...]
 * This issues: START, addr(W), offset+data..., STOP
 */
static int32_t tv5725_i2c_write(const uint8_t *buf, uint8_t len)
{
    return I2C_Master_Transmit(TV5725_I2C_ADDR_8BIT, buf, len, V_TIMEOUT);
}

/*
 * Write 1-byte register offset, then restart and read len bytes.
 * Issues: START, addr(W), reg_offset, RESTART, addr(R), data[len], STOP
 */
static int32_t tv5725_i2c_read(uint8_t reg_offset, uint8_t *rxbuf, uint8_t len)
{
    int32_t i32Ret;

    I2C_Cmd(V_I2C_UNIT, ENABLE);
    I2C_SWResetCmd(V_I2C_UNIT, ENABLE);
    I2C_SWResetCmd(V_I2C_UNIT, DISABLE);

    i32Ret = I2C_Start(V_I2C_UNIT, V_TIMEOUT);
    if (LL_OK != i32Ret)
        goto err;

    i32Ret = I2C_TransAddr(V_I2C_UNIT, TV5725_I2C_ADDR, I2C_DIR_TX, V_TIMEOUT);
    if (LL_OK != i32Ret)
        goto err;

    i32Ret = I2C_TransData(V_I2C_UNIT, &reg_offset, 1U, V_TIMEOUT);
    if (LL_OK != i32Ret)
        goto err;

    i32Ret = I2C_Restart(V_I2C_UNIT, V_TIMEOUT);
    if (LL_OK != i32Ret)
        goto err;

    if (1UL == len)
    {
        I2C_AckConfig(V_I2C_UNIT, I2C_NACK);
    }

    i32Ret = I2C_TransAddr(V_I2C_UNIT, TV5725_I2C_ADDR, I2C_DIR_RX, V_TIMEOUT);
    if (LL_OK != i32Ret)
    {
        I2C_AckConfig(V_I2C_UNIT, I2C_ACK);
        goto err;
    }

    i32Ret = I2C_MasterReceiveDataAndStop(V_I2C_UNIT, rxbuf, len, V_TIMEOUT);
    I2C_AckConfig(V_I2C_UNIT, I2C_ACK);

    if (LL_OK != i32Ret)
        goto err_done;

    I2C_Cmd(V_I2C_UNIT, DISABLE);
    return LL_OK;

err:
    (void)I2C_Stop(V_I2C_UNIT, V_TIMEOUT);
err_done:
    I2C_Cmd(V_I2C_UNIT, DISABLE);
    return i32Ret;
}

/* -------------------------------------------------------------------
   Segment management
   ------------------------------------------------------------------- */

/*
 * Set the segment register (0xF0).  Cached: no I2C traffic if the
 * segment hasn't changed since the last call.
 */
void tv5725_set_segment(uint8_t segment)
{
    if (g_tv5725_cur_seg != segment)
    {
        uint8_t buf[2];
        buf[0] = TV5725_SEG_REG;
        buf[1] = segment;
        tv5725_i2c_write(buf, 2);
        g_tv5725_cur_seg = segment;
    }
}

/* -------------------------------------------------------------------
   Byte-level read / write (within a segment)
   ------------------------------------------------------------------- */

int32_t tv5725_read_byte(uint8_t segment, uint8_t offset, uint8_t *value)
{
    tv5725_set_segment(segment);
    return tv5725_i2c_read(offset, value, 1);
}

int32_t tv5725_write_byte(uint8_t segment, uint8_t offset, uint8_t value)
{
    uint8_t buf[2];
    tv5725_set_segment(segment);
    buf[0] = offset;
    buf[1] = value;
    return tv5725_i2c_write(buf, 2);
}

int32_t tv5725_read_buf(uint8_t segment, uint8_t offset, uint8_t *buf, uint8_t len)
{
    tv5725_set_segment(segment);
    return tv5725_i2c_read(offset, buf, len);
}

int32_t tv5725_write_buf(uint8_t segment, uint8_t offset, const uint8_t *buf, uint8_t len)
{
    uint8_t tmp[17]; /* Up to 16 bytes data + 1 byte register offset */

    tv5725_set_segment(segment);

    if (len > (uint8_t)(sizeof(tmp) - 1U))
    {
        /* Chunked: first write the register offset only, then send data
           as a separate I2C write (assuming auto-increment on the chip) */
        uint8_t i;
        int32_t ret = LL_OK;
        for (i = 0; i < len; i++)
        {
            ret = tv5725_write_byte(segment, (uint8_t)(offset + i), buf[i]);
            if (ret != LL_OK)
                break;
        }
        return ret;
    }
    tmp[0] = offset;
    (void)memcpy(tmp + 1, buf, len);
    return tv5725_i2c_write(tmp, (uint8_t)(len + 1));
}

/* -------------------------------------------------------------------
   Raw register field read / write
   ------------------------------------------------------------------- */

/* Return the byte-size covered by a register */
static uint8_t reg_byte_size(uint8_t bit_offset, uint8_t bit_width)
{
    return (uint8_t)((bit_offset + bit_width + 7U) / 8U);
}

uint32_t tv5725_reg_read(tv5725_reg_t reg)
{
    uint8_t bs;
    uint8_t data[4];
    uint32_t value;
    uint8_t i;

    if (reg.bit_width > 32U)
        return 0;

    bs = reg_byte_size(reg.bit_offset, reg.bit_width);
    (void)tv5725_read_buf(reg.segment, reg.offset, data, bs);

    /* Decode: data[0] >> bit_offset forms the LSBs */
    value = (uint32_t)(data[0] >> reg.bit_offset);
    for (i = 1; i < bs; i++)
    {
        value |= (uint32_t)data[i] << (8U * i - reg.bit_offset);
    }
    /* Mask off excess bits */
    if (reg.bit_width < 32U)
    {
        value &= ((1UL << reg.bit_width) - 1UL);
    }
    return value;
}

void tv5725_reg_write(tv5725_reg_t reg, uint32_t value)
{
    uint8_t bs;
    uint8_t data[4];
    uint8_t i;

    if (reg.bit_width > 32U)
        return;

    bs = reg_byte_size(reg.bit_offset, reg.bit_width);

    /* Read-modify-write unless the field is byte-aligned */
    if (reg.bit_offset == 0 && (reg.bit_width & 0x7) == 0)
    {
        (void)memset(data, 0, sizeof(data));
    }
    else
    {
        (void)tv5725_read_buf(reg.segment, reg.offset, data, bs);
    }

    /* Encode: single-byte case */
    if (bs == 1)
    {
        uint8_t mask = (uint8_t)(((1U << reg.bit_width) - 1U) << reg.bit_offset);
        data[0] = (data[0] & ~mask) | (uint8_t)((value << reg.bit_offset) & mask);
    }
    else
    {
        /* Least significant byte: mask from bit_offset upward */
        uint8_t mask0 = (uint8_t)(0xFFU << reg.bit_offset);
        data[0] = (data[0] & ~mask0) | (uint8_t)((value << reg.bit_offset) & mask0);

        /* Full interior bytes */
        for (i = 1; i < bs - 1; i++)
        {
            data[i] = (uint8_t)(value >> (8U * i - reg.bit_offset));
        }

        /* Most significant byte: partial mask */
        uint8_t msb_bits = (uint8_t)(reg.bit_width + reg.bit_offset - (bs - 1) * 8);
        uint8_t mask_msb = (uint8_t)((1U << msb_bits) - 1U);
        uint8_t shift = (uint8_t)(8U * (bs - 1) - reg.bit_offset);
        data[bs - 1] = (data[bs - 1] & ~mask_msb) | (uint8_t)((value >> shift) & mask_msb);
    }

    (void)tv5725_write_buf(reg.segment, reg.offset, data, bs);
}

/* -------------------------------------------------------------------
   Convenience: read/write single byte by segment + offset (no bitfield)
   ------------------------------------------------------------------- */

int32_t tv5725_read_seg_byte(uint8_t segment, uint8_t offset, uint8_t *value)
{
    return tv5725_read_byte(segment, offset, value);
}

int32_t tv5725_write_seg_byte(uint8_t segment, uint8_t offset, uint8_t value)
{
    return tv5725_write_byte(segment, offset, value);
}

/* -------------------------------------------------------------------
   Chip-level operations
   ------------------------------------------------------------------- */

/*
 * Full chip reset: release all soft-reset bits, then assert+release
 * the key blocks (IF, DEINT, MEM, VDS, DEC, MODE, SYNC, HDBYPS).
 */
void tv5725_chip_reset(void)
{
    uint8_t i, j;

    /* Release all soft reset bits first */
    tv5725_reg_write(TV5725_RESET_CONTROL_46, 0x00);
    tv5725_reg_write(TV5725_RESET_CONTROL_47, 0x00);

    /* Clear all 6 segments (0-5) in 16-byte banks */
    for (i = 0; i < TV5725_SEG_COUNT; i++)
    {
        tv5725_set_segment(i);
        for (j = 0; j < 16; j++)
        {
            uint8_t bank[16];
            (void)memset(bank, 0, sizeof(bank));
            (void)tv5725_write_buf(i, (uint8_t)(j * 16), bank, 16);
        }
    }

    /* Re-assert reset on key blocks, then release */
    tv5725_reg_write(TV5725_SFTRST_IF_RSTZ, 0);
    tv5725_reg_write(TV5725_SFTRST_DEINT_RSTZ, 0);
    tv5725_reg_write(TV5725_SFTRST_MEM_FF_RSTZ, 0);
    tv5725_reg_write(TV5725_SFTRST_VDS_RSTZ, 0);
    tv5725_reg_write(TV5725_SFTRST_FIFO_RSTZ, 0);
    tv5725_reg_write(TV5725_SFTRST_DEC_RSTZ, 0);
    tv5725_reg_write(TV5725_SFTRST_MODE_RSTZ, 0);
    tv5725_reg_write(TV5725_SFTRST_SYNC_RSTZ, 0);
    tv5725_reg_write(TV5725_SFTRST_HDBYPS_RSTZ, 0);

    tv5725_reg_write(TV5725_SFTRST_IF_RSTZ, 1);
    tv5725_reg_write(TV5725_SFTRST_DEINT_RSTZ, 1);
    tv5725_reg_write(TV5725_SFTRST_MEM_FF_RSTZ, 1);
    tv5725_reg_write(TV5725_SFTRST_VDS_RSTZ, 1);
    tv5725_reg_write(TV5725_SFTRST_FIFO_RSTZ, 1);
    tv5725_reg_write(TV5725_SFTRST_DEC_RSTZ, 1);
    tv5725_reg_write(TV5725_SFTRST_MODE_RSTZ, 1);
    tv5725_reg_write(TV5725_SFTRST_SYNC_RSTZ, 1);
    tv5725_reg_write(TV5725_SFTRST_HDBYPS_RSTZ, 1);
}

/*
 * Read the chip product ID.  Should return 0x57 for TV5725.
 */
uint8_t tv5725_get_chip_id(void)
{
    return (uint8_t)tv5725_reg_read(TV5725_CHIP_ID_PRODUCT);
}

/*
 * Initialize the TV5725 chip.
 * Returns LL_OK on success, LL_ERR on I2C failure.
 */
int32_t tv5725_init(void)
{
    uint8_t chip_id;

    chip_id = tv5725_get_chip_id();
    if (chip_id == 0x00 || chip_id == 0xFF)
    {
        /* Chip not responding */
        return LL_ERR;
    }
    printf("TV5725 Chip ID: 0x%02X\n", chip_id);
    tv5725_chip_reset();

    return LL_OK;
}

/* ====================================================================
   Preset loader — writes a complete register preset array to the chip.
   Preset format (matches GBSCpro layout):
     Segment 0: 48 bytes (0x40-0x5F bank0+bank1, 0x90-0x9F bank2)
     Segment 1: 48 bytes (0x00-0x2F bank0-2)
     Segment 3: 128 bytes (0x00-0x7F bank0-7)
     Segment 4: 96 bytes (0x00-0x5F bank0-5)
     Segment 5: 112 bytes (0x00-0x6F bank0-6)
   Total: 432 bytes
   ==================================================================== */

void tv5725_load_preset(const uint8_t *preset)
{
    uint16_t idx = 0;
    uint8_t bank[16];
    uint8_t i;

    /* ---- Segment 0: 2 banks at 0x40, 1 bank at 0x90 ---- */
    tv5725_set_segment(0x00);
    for (i = 0; i < 2; i++)
    {
        (void)memcpy(bank, &preset[idx], 16);
        (void)tv5725_write_buf(0x00, (uint8_t)(0x40 + i * 16), bank, 16);
        idx += 16;
    }
    /* OSD bank at 0x90 */
    (void)memcpy(bank, &preset[idx], 16);
    (void)tv5725_write_buf(0x00, 0x90, bank, 16);
    idx += 16;

    /* ---- Segment 1: 3 banks at 0x00-0x2F ---- */
    for (i = 0; i < 3; i++)
    {
        (void)memcpy(bank, &preset[idx], 16);
        (void)tv5725_write_buf(0x01, (uint8_t)(i * 16), bank, 16);
        idx += 16;
    }

    /* ---- Load mode-detect section (segment 1, 0x60-0x83) ---- */
    {
        extern const uint8_t preset_md_section[];
        tv5725_set_segment(0x01);
        for (i = 0; i < 2; i++)
        {
            (void)memcpy(bank, &preset_md_section[i * 16], 16);
            (void)tv5725_write_buf(0x01, (uint8_t)(0x60 + i * 16), bank, 16);
        }
        /* Last 4 bytes at 0x80 */
        (void)memcpy(bank, &preset_md_section[32], 4);
        (void)tv5725_write_buf(0x01, 0x80, bank, 4);
    }

    /* ---- Segment 2: load deinterlacer preset ---- */
    {
        extern const uint8_t preset_deinterlacer[];
        tv5725_set_segment(0x02);
        for (i = 0; i < 4; i++)
        {
            (void)memcpy(bank, &preset_deinterlacer[i * 16], 16);
            (void)tv5725_write_buf(0x02, (uint8_t)(i * 16), bank, 16);
        }
    }

    /* ---- Segment 3: 8 banks at 0x00-0x7F ---- */
    for (i = 0; i < 8; i++)
    {
        (void)memcpy(bank, &preset[idx], 16);
        (void)tv5725_write_buf(0x03, (uint8_t)(i * 16), bank, 16);
        idx += 16;
    }
    /* Clear PIP area (0x80-0x8F) */
    (void)memset(bank, 0, sizeof(bank));
    tv5725_set_segment(0x03);
    (void)tv5725_write_buf(0x03, 0x80, bank, 16);

    /* ---- Segment 4: 6 banks at 0x00-0x5F ---- */
    for (i = 0; i < 6; i++)
    {
        (void)memcpy(bank, &preset[idx], 16);
        (void)tv5725_write_buf(0x04, (uint8_t)(i * 16), bank, 16);
        idx += 16;
    }

    /* ---- Segment 5: 7 banks at 0x00-0x6F ---- */
    for (i = 0; i < 7; i++)
    {
        (void)memcpy(bank, &preset[idx], 16);
        (void)tv5725_write_buf(0x05, (uint8_t)(i * 16), bank, 16);
        idx += 16;
    }
}

/* -------------------------------------------------------------------
   Frame buffer pipeline control
   ------------------------------------------------------------------- */

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

/* -------------------------------------------------------------------
   Color matrix patches
   ------------------------------------------------------------------- */

void tv5725_apply_rgb_patches(void)
{
    /* ADC: R/G/B direct (not YUV remap) */
    tv5725_reg_write(TV5725_ADC_RYSEL_R, 0);
    tv5725_reg_write(TV5725_ADC_RYSEL_G, 0);
    tv5725_reg_write(TV5725_ADC_RYSEL_B, 0);

    /* Bypass decoder CSC */
    tv5725_reg_write(TV5725_DEC_MATRIX_BYPS, 0);
    tv5725_reg_write(TV5725_IF_MATRIX_BYPS, 1);

    /* VDS color matrix: identity gains, zero offsets */
    tv5725_reg_write(TV5725_VDS_Y_GAIN,   0x80);
    tv5725_reg_write(TV5725_VDS_UCOS_GAIN, 0x1C);
    tv5725_reg_write(TV5725_VDS_VCOS_GAIN, 0x29);
    tv5725_reg_write(TV5725_VDS_Y_OFST,   0x00);
    tv5725_reg_write(TV5725_VDS_U_OFST,   0x00);
    tv5725_reg_write(TV5725_VDS_V_OFST,   0x00);
}

void tv5725_apply_yuv_patches(void)
{
    /* ADC: remap for YUV (G=Y, R=Pr, B=Pb) */
    tv5725_reg_write(TV5725_ADC_RYSEL_R, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_G, 0);
    tv5725_reg_write(TV5725_ADC_RYSEL_B, 1);

    /* Enable decoder CSC for YUV→RGB */
    tv5725_reg_write(TV5725_DEC_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_IF_MATRIX_BYPS, 1);

    /* VDS color matrix */
    tv5725_reg_write(TV5725_VDS_Y_GAIN,   128);
    tv5725_reg_write(TV5725_VDS_UCOS_GAIN, 28);
    tv5725_reg_write(TV5725_VDS_VCOS_GAIN, 41);
    tv5725_reg_write(TV5725_VDS_Y_OFST,   0x0E);
    tv5725_reg_write(TV5725_VDS_U_OFST,   0x03);
    tv5725_reg_write(TV5725_VDS_V_OFST,   0x04);
}

/* -------------------------------------------------------------------
   Output path initialization (uses preset + post-init)
   ------------------------------------------------------------------- */

int32_t tv5725_output_path_init(const uint8_t *preset, uint8_t input_is_yuv)
{
    /* 1. Load full register preset for the target resolution */
    tv5725_load_preset(preset);

    /* 2. Initialize SDRAM */
    tv5725_sdram_init();

    /* 3. Post-preset steps (from GBSCpro doPostPresetLoadSteps) */

    /* Enable DAC outputs */
    tv5725_reg_write(TV5725_DAC_RGBS_PWDNZ, 1);
    tv5725_reg_write(TV5725_DAC_RGBS_SPD, 0);
    tv5725_reg_write(TV5725_DAC_RGBS_S0ENZ, 0);
    tv5725_reg_write(TV5725_DAC_RGBS_S1EN, 1);

    /* Enable sync output */
    tv5725_reg_write(TV5725_PAD_SYNC_OUT_ENZ, 0);
    tv5725_reg_write(TV5725_OUT_SYNC_CNTRL, 1);

    /* Apply color matrix based on input type */
    if (input_is_yuv)
        tv5725_apply_yuv_patches();
    else
        tv5725_apply_rgb_patches();

    /* ADC auto-offset: periodic, no delay, step=0 */
    tv5725_reg_write(TV5725_ADC_AUTO_OFST_PRD, 1);
    tv5725_reg_write(TV5725_ADC_AUTO_OFST_DELAY, 0);
    tv5725_reg_write(TV5725_ADC_AUTO_OFST_STEP, 0);
    tv5725_reg_write(TV5725_ADC_AUTO_OFST_TEST, 1);
    tv5725_reg_write(TV5725_ADC_AUTO_OFST_RANGE_REG, 0x00);

    /* GPIO control (s0_52, s0_53 from preset already set) */

    /* 4. Start frame buffer pipeline */
    tv5725_capture_start();

    return LL_OK;
}

/* ====================================================================
   Input mode configuration
   ==================================================================== */

/* Current input mode */
static tv5725_input_mode_t g_tv5725_input_mode = TV5725_INPUT_AUTO;

/*
 * Common ADC and input clock setup, shared by all input modes.
 */
static void tv5725_input_config_adc_common(void)
{
    /* ADC clock: PLLAD from OSC, PA_ADC from PLLAD CLKO2 */
    tv5725_reg_write(TV5725_ADC_CLK_PA, 0x00);
    tv5725_reg_write(TV5725_ADC_CLK_PLLAD, 1);
    tv5725_reg_write(TV5725_ADC_CLK_ICLK2X, 0);
    tv5725_reg_write(TV5725_ADC_CLK_ICLK1X, 0);

    /* Select R0/G0/B0 input */
    tv5725_reg_write(TV5725_ADC_INPUT_SEL, 0x00);

    /* Power up ADC, clamp to midscale, 110MHz filter */
    tv5725_reg_write(TV5725_ADC_POWDZ, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_R, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_G, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_B, 1);
    tv5725_reg_write(TV5725_ADC_FLTR, 0x01);

    /* Offset = 0, gain = 0x80 for all channels */
    tv5725_write_seg_byte(0x05, 0x06, 0x00);
    tv5725_write_seg_byte(0x05, 0x07, 0x00);
    tv5725_write_seg_byte(0x05, 0x08, 0x00);
    tv5725_write_seg_byte(0x05, 0x09, 0x80);
    tv5725_write_seg_byte(0x05, 0x0A, 0x80);
    tv5725_write_seg_byte(0x05, 0x0B, 0x80);

    /* PLLAD: power up, bypass mode */
    tv5725_reg_write(TV5725_PLLAD_PDZ, 1);
    tv5725_reg_write(TV5725_PLLAD_BPS, 1);
    tv5725_reg_write(TV5725_PLLAD_FS, 0);
}

/*
 * Configure input path for RGBS (RGB + composite sync, e.g. arcade / BNC).
 * R/G/B → ADC R/G/B, sync → HSIN1 external sync.
 * IF matrix bypassed (signal is already RGB).
 */
int32_t tv5725_input_config_rgbs(void)
{
    tv5725_input_config_adc_common();

    /* Disable SOG */
    tv5725_reg_write(TV5725_ADC_SOGEN, 0);

    /* Sync Processor: external sync from HSIN1/VSIN1, auto polarity */
    tv5725_reg_write(TV5725_SP_SOG_SRC_SEL, 0);
    tv5725_reg_write(TV5725_SP_EXT_SYNC_SEL, 0);
    tv5725_reg_write(TV5725_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_SP_VS_POL_ATO, 1);

    /* Input Formatter: 24-bit, bypass matrix (already RGB), bypass data reg */
    tv5725_reg_write(TV5725_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_IF_IN_DREG_BYPS, 1);

    /* ASW01-04: ASW01=H, ASW02=L, ASW03=L, ASW04=L for RGBS */
    tv5725_asw_set(TV5725_ASW01, 1);
    tv5725_asw_set(TV5725_ASW02, 0);
    tv5725_asw_set(TV5725_ASW03, 0);
    tv5725_asw_set(TV5725_ASW04, 0);

    g_tv5725_input_mode = TV5725_INPUT_RGBS;
    return LL_OK;
}

/*
 * Configure input path for YUV (YPbPr component video).
 * Pr/Y/Pb → ADC R/G/B, sync extracted via SOG on Y channel.
 * IF matrix enabled for YUV→RGB conversion.
 */
int32_t tv5725_input_config_yuv(void)
{
    tv5725_input_config_adc_common();

    /* Enable SOG on Y channel (G ADC input) */
    tv5725_reg_write(TV5725_ADC_SOGEN, 1);

    /* Sync Processor: SOG source, auto polarity */
    tv5725_reg_write(TV5725_SP_SOG_SRC_SEL, 1);
    tv5725_reg_write(TV5725_SP_SOG_P_ATO, 1);
    tv5725_reg_write(TV5725_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_SP_VS_POL_ATO, 1);

    /* Input Formatter: 24-bit, enable matrix (YUV→RGB), bypass data reg */
    tv5725_reg_write(TV5725_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_IF_MATRIX_BYPS, 0);
    tv5725_reg_write(TV5725_IF_IN_DREG_BYPS, 1);

    /* ASW01-04: ASW01=L, ASW02=L, ASW03=L, ASW04=L for YUV */
    tv5725_asw_set(TV5725_ASW01, 0);
    tv5725_asw_set(TV5725_ASW02, 0);
    tv5725_asw_set(TV5725_ASW03, 0);
    tv5725_asw_set(TV5725_ASW04, 0);

    g_tv5725_input_mode = TV5725_INPUT_YUV;
    return LL_OK;
}

/*
 * Auto-detect input source by reading status registers.
 * Tries RGBS first (external sync), then YUV (SOG).
 */
void tv5725_input_auto_detect(void)
{
    uint8_t s0_05;

    /* Read STATUS_IF general status */
    tv5725_read_byte(0x00, 0x05, &s0_05);

    if (s0_05 == 0x00 || s0_05 == 0xFF)
    {
        /* No response, fall back to RGBS */
        tv5725_input_config_rgbs();
        return;
    }

    /* NO_SYNC bit (bit1): 0 = sync present, 1 = no sync */
    if (!(s0_05 & 0x02))
    {
        /* External sync detected → RGBS mode */
        tv5725_input_config_rgbs();
        return;
    }

    /* No external sync: try SOG mode for YUV */
    tv5725_input_config_yuv();

    /* TODO: verify SOG lock via PLLAD status or input detection bits */
    g_tv5725_input_mode = TV5725_INPUT_AUTO;
}

/*
 * Set input mode explicitly or use auto-detect.
 */
int32_t tv5725_input_set_mode(tv5725_input_mode_t mode)
{
    switch (mode)
    {
    case TV5725_INPUT_YUV:
        return tv5725_input_config_yuv();
    case TV5725_INPUT_RGBS:
        return tv5725_input_config_rgbs();
    case TV5725_INPUT_AUTO:
    default:
        tv5725_input_auto_detect();
        return LL_OK;
    }
}

/* ====================================================================
   Analog switch GPIO control (ASW01-ASW04: PB12-PB15)
   ==================================================================== */

void tv5725_asw_init(void)
{
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir = PIN_DIR_OUT;

    (void)GPIO_Init(TV5725_ASW_PORT, TV5725_ASW01, &stcGpioInit);
    (void)GPIO_Init(TV5725_ASW_PORT, TV5725_ASW02, &stcGpioInit);
    (void)GPIO_Init(TV5725_ASW_PORT, TV5725_ASW03, &stcGpioInit);
    (void)GPIO_Init(TV5725_ASW_PORT, TV5725_ASW04, &stcGpioInit);
}

/*
 * Set a single PB12-PB15 pin high or low.
 */
void tv5725_asw_set(uint16_t pin, uint8_t high)
{
    if (high)
        GPIO_SetPins(TV5725_ASW_PORT, pin);
    else
        GPIO_ResetPins(TV5725_ASW_PORT, pin);
}

