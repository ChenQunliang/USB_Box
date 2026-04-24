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

/*
 * Initialize the input path: ADC, PLLAD, Sync Processor, Input Formatter.
 * Configures for analog RGB (R0/G0/B0) input via ADC.
 */
int32_t tv5725_input_path_init(void)
{
    /* ---- ADC clock control (S5_00) ----
       PLLAD input from OSC (27MHz), PA_ADC from PLLAD CLKO2 */
    tv5725_reg_write(TV5725_ADC_CLK_PA, 0x00);
    tv5725_reg_write(TV5725_ADC_CLK_PLLAD, 1);
    tv5725_reg_write(TV5725_ADC_CLK_ICLK2X, 0);
    tv5725_reg_write(TV5725_ADC_CLK_ICLK1X, 0);

    /* ---- ADC input select & SOG (S5_02) ----
       Select R0/G0/B0 input, disable SOG */
    tv5725_reg_write(TV5725_ADC_INPUT_SEL, 0x00);
    tv5725_reg_write(TV5725_ADC_SOGEN, 0);

    /* ---- ADC power & clamp (S5_03) ----
       Power up ADC, clamp to midscale, 110MHz filter */
    tv5725_reg_write(TV5725_ADC_POWDZ, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_R, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_G, 1);
    tv5725_reg_write(TV5725_ADC_RYSEL_B, 1);
    tv5725_reg_write(TV5725_ADC_FLTR, 0x01);

    /* ADC offset: 0 for R/G/B (S5_06~S5_08) */
    tv5725_write_seg_byte(0x05, 0x06, 0x00);
    tv5725_write_seg_byte(0x05, 0x07, 0x00);
    tv5725_write_seg_byte(0x05, 0x08, 0x00);

    /* ADC gain: default 0x80 for R/G/B (S5_09~S5_0B) */
    tv5725_write_seg_byte(0x05, 0x09, 0x80);
    tv5725_write_seg_byte(0x05, 0x0A, 0x80);
    tv5725_write_seg_byte(0x05, 0x0B, 0x80);

    /* ---- PLLAD configuration ----
       Power up, bypass, FS=0 for HSYNC-based clock recovery */
    tv5725_reg_write(TV5725_PLLAD_PDZ, 1);
    tv5725_reg_write(TV5725_PLLAD_BPS, 1);
    tv5725_reg_write(TV5725_PLLAD_FS, 0);

    /* ---- Sync Processor ----
       External sync from HSIN1/VSIN1 */
    tv5725_reg_write(TV5725_SP_SOG_SRC_SEL, 0);
    tv5725_reg_write(TV5725_SP_EXT_SYNC_SEL, 0);
    tv5725_reg_write(TV5725_SP_SOG_P_ATO, 1);
    tv5725_reg_write(TV5725_SP_HS_POL_ATO, 1);
    tv5725_reg_write(TV5725_SP_VS_POL_ATO, 1);

    /* ---- Input Formatter ----
       24-bit RGB mode, bypass matrix, bypass data registers */
    tv5725_reg_write(TV5725_IF_SEL24BIT, 1);
    tv5725_reg_write(TV5725_IF_MATRIX_BYPS, 1);
    tv5725_reg_write(TV5725_IF_IN_DREG_BYPS, 1);

    return LL_OK;
}

/*
 * Initialize the output path: PLL, VDS timing, DAC, sync outputs.
 * Configures 640x480 VGA output at ~27MHz pixel clock from a 27MHz crystal.
 */
int32_t tv5725_output_path_init(void)
{
    /* ---- PLL648: VCLK = 27MHz (from 27MHz crystal) ----
       S0_40: source=OSC, div2=off, ICLK from PLL, mem=108MHz */
    tv5725_reg_write(TV5725_PLL_CKIS, 0);
    tv5725_reg_write(TV5725_PLL_DIVBY2Z, 0);
    tv5725_reg_write(TV5725_PLL_IS, 0);
    tv5725_reg_write(TV5725_PLL_ADS, 0);
    tv5725_reg_write(TV5725_PLL_MS, 0x00);

    /* S0_41: 4XV=0, 2XV=0, VS4=00, VS2=01, VS=01 → VCLK=27, V2CLK=27, V4CLK=27 */
    tv5725_reg_write(TV5725_PLL_4XV, 0);
    tv5725_reg_write(TV5725_PLL_2XV, 0);
    tv5725_reg_write(TV5725_PLL_VS4, 0);
    tv5725_reg_write(TV5725_PLL_VS2, 1);
    tv5725_reg_write(TV5725_PLL_VS, 1);

    /* S0_43: Lock enable */
    tv5725_reg_write(TV5725_PLL_LEN, 1);
    tv5725_reg_write(TV5725_PLL_VCORST, 0);

    /* ---- PAD control ----
       S0_48: enable sync1 input, disable digital video I/O */
    tv5725_write_seg_byte(0x00, 0x48, 0xEA);

    /* S0_49: enable HSOUT/VSOUT/HBOUT/VBOUT, enable PCLKIN, disable tri-state */
    tv5725_reg_write(TV5725_PAD_CKIN_ENZ, 0);
    tv5725_reg_write(TV5725_PAD_CKOUT_ENZ, 1);
    tv5725_reg_write(TV5725_PAD_SYNC_OUT_ENZ, 0);
    tv5725_reg_write(TV5725_PAD_BLK_OUT_ENZ, 0);
    tv5725_reg_write(TV5725_PAD_TRI_ENZ, 1);

    /* ---- DAC enable ----
       Power up DAC, enable R/G/B output to follow data */
    tv5725_reg_write(TV5725_DAC_RGBS_PWDNZ, 1);
    tv5725_reg_write(TV5725_DAC_RGBS_R0ENZ, 1);
    tv5725_reg_write(TV5725_DAC_RGBS_G0ENZ, 1);
    tv5725_reg_write(TV5725_DAC_RGBS_B0ENZ, 1);

    /* Enable sync DAC output */
    tv5725_reg_write(TV5725_DAC_RGBS_S0ENZ, 1);

    /* DAC_MUX (S0_4B): bypass input DFF */
    tv5725_write_seg_byte(0x00, 0x4B, 0x01);

    /* ---- CLK/SYNC control (S0_4F) ----
       Sync output from VDS, enable H/V sync output */
    tv5725_reg_write(TV5725_OUT_SYNC_SEL, 0x00);
    tv5725_reg_write(TV5725_OUT_SYNC_CNTRL, 1);

    /* ---- VDS control (S3_00) ----
       Free run, bypass H/V scaling */
    tv5725_reg_write(TV5725_VDS_SYNC_EN, 0);
    tv5725_reg_write(TV5725_VDS_HSCALE_BYPS, 1);
    tv5725_reg_write(TV5725_VDS_VSCALE_BYPS, 1);

    /* ---- VDS timing: 640x480 @ 27MHz (H-total=800, V-total=525) ----
       Frame rate = 27e6 / (800*525) ≈ 64.3Hz, tolerable for most VGA monitors */

    /* Horizontal: total=800, active=640, blank=160, sync pulse=96 */
    tv5725_reg_write(TV5725_VDS_HSYNC_RST, 799);       /* H-total - 1 */
    tv5725_reg_write(TV5725_VDS_HB_ST, 640);            /* H-blank start (active end) */
    tv5725_reg_write(TV5725_VDS_HB_SP, 800);            /* H-blank stop (total) */
    tv5725_reg_write(TV5725_VDS_HS_ST, 656);            /* H-sync start */
    tv5725_reg_write(TV5725_VDS_HS_SP, 752);            /* H-sync stop */
    tv5725_reg_write(TV5725_VDS_DIS_HB_ST, 640);        /* Display H-blank start */
    tv5725_reg_write(TV5725_VDS_DIS_HB_SP, 800);        /* Display H-blank stop */

    /* Vertical: total=525, active=480, blank=45, sync pulse=2 */
    tv5725_reg_write(TV5725_VDS_VSYNC_RST, 524);        /* V-total - 1 */
    tv5725_reg_write(TV5725_VDS_VB_ST, 480);            /* V-blank start (active end) */
    tv5725_reg_write(TV5725_VDS_VB_SP, 525);            /* V-blank stop (total) */
    tv5725_reg_write(TV5725_VDS_VS_ST, 490);            /* V-sync start */
    tv5725_reg_write(TV5725_VDS_VS_SP, 492);            /* V-sync stop */
    tv5725_reg_write(TV5725_VDS_DIS_VB_ST, 480);        /* Display V-blank start */
    tv5725_reg_write(TV5725_VDS_DIS_VB_SP, 525);        /* Display V-blank stop */

    /* ---- Deinterlacer bypass ----
       Bypass deinterlacer since we use progressive output */
    tv5725_reg_write(TV5725_DIAG_BOB_PLDY_RAM_BYPS, 1);

    /* ---- VDS data path bypass ----
       Bypass internal conversions for minimum-latency passthrough */
    tv5725_reg_write(TV5725_VDS_CONVT_BYPS, 1);
    tv5725_reg_write(TV5725_VDS_IN_DREG_BYPS, 1);

    return LL_OK;
}
