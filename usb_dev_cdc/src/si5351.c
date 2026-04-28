#include "si5351.h"
#include "v_i2c.h"

/* ==================================================================
   I2C bus wrappers
   Uses V_I2C (I2C1, same bus as TV5725, PA4-SCL / PA5-SDA @ 400 kHz)

   Write:  START + addrW + reg + data[0..n] + STOP
   Read:   START + addrW + reg + STOP
           START + addrR + data[0..n] + STOP   (two transactions)
   ================================================================== */

int32_t si5351_write_burst(uint8_t reg, const uint8_t *data, uint8_t len)
{
    uint8_t buf[17];  /* register + up to 16 data bytes */

    if (len > 16)
        return LL_ERR_INVD_PARAM;

    buf[0] = reg;
    for (uint8_t i = 0; i < len; i++)
        buf[1 + i] = data[i];

    return I2C_Master_Transmit(SI5351_ADDR_WR, buf, (uint32_t)(len + 1), V_TIMEOUT);
}

int32_t si5351_write(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    return I2C_Master_Transmit(SI5351_ADDR_WR, buf, 2, V_TIMEOUT);
}

int32_t si5351_read(uint8_t reg, uint8_t *value)
{
    int32_t ret;

    ret = I2C_Master_Transmit(SI5351_ADDR_WR, &reg, 1, V_TIMEOUT);
    if (ret != LL_OK)
        return ret;

    return I2C_Master_Receive(SI5351_ADDR_RD, value, 1, V_TIMEOUT);
}

/* ==================================================================
   Detection — probe Si5351 at I2C address 0x60, check SYS_INIT bit
   ================================================================== */

int32_t si5351_detect(si5351_t *dev)
{
    uint8_t val = 0;
    int32_t ret;

    if (dev == NULL)
        return LL_ERR_INVD_PARAM;

    dev->detected = 0;

    /* Attempt to read register 0 (Device Status) — device ACK proves presence */
    ret = si5351_read(SI5351_REG_DEVICE_STATUS, &val);
    if (ret != LL_OK)
        return ret;

    /* Bit 7 (SYS_INIT): 0 = powered on and ready */
    if ((val & 0x80) == 0)
    {
        dev->detected = 1;
        return LL_OK;
    }

    return LL_ERR;
}

/* ==================================================================
   Initialisation
   ================================================================== */

int32_t si5351_init(si5351_t *dev, uint32_t xtal_freq)
{
    uint8_t i;

    if (dev == NULL || xtal_freq == 0)
        return LL_ERR_INVD_PARAM;

    dev->xtal_freq = xtal_freq;

    /* Initialise per-channel state */
    for (i = 0; i < SI5351_NUM_OUTPUTS; i++)
    {
        dev->clk_power[i] = SI5351_DRIVE_2mA;
        dev->omsynth[i]   = 0;
        dev->o_Rdiv[i]    = 0;
        dev->clk_on[i]    = 0;
    }

    /* Disable spread spectrum (register 149, bit 7 = 0) */
    {
        uint8_t regval;
        if (si5351_read(SI5351_REG_SSEN_SPREAD, &regval) == LL_OK)
        {
            regval &= ~0x80U;
            si5351_write(SI5351_REG_SSEN_SPREAD, regval);
        }
    }

    /* Power off all outputs */
    si5351_disable_all(dev);

    return LL_OK;
}

/* ==================================================================
   Set output frequency (integer mode, VCO 600-900 MHz)
   CLK0 → PLLA, CLK1/2 → PLLB
   ================================================================== */

void si5351_set_freq(si5351_t *dev, uint8_t clk, uint32_t freq_hz)
{
    uint8_t  pll_stride = 0, msyn_stride = 0;
    uint32_t a, b, c, f, fvco, outdivider;

    if (dev == NULL || clk >= SI5351_NUM_OUTPUTS || freq_hz == 0)
        return;

    /* --- Calculate output divider for VCO in 600-900 MHz range --- */
    outdivider = 900000000UL / freq_hz;

    /* Apply additional R divider (power-of-2) if outdivider > 900 */
    {
        uint8_t  R = 1;
        while (outdivider > 900)
        {
            R *= 2;
            outdivider /= 2;
        }

        /* Round down to nearest even number */
        if (outdivider % 2)
            outdivider--;

        /* Actual VCO frequency */
        fvco = outdivider * R * freq_hz;

        /* --- PLL MSNA / MSNB registers (integer mode) --- */
        a = fvco / dev->xtal_freq;
        b = (fvco % dev->xtal_freq) >> 5;
        c = dev->xtal_freq >> 5;
        f = (128UL * b) / c;

        {
            uint32_t MSNx_P1 = 128UL * a + f - 512;
            uint32_t MSNx_P2 = 128UL * b - f * c;
            uint32_t MSNx_P3 = c;

            /* PLL register bank (8 bytes at 26 + stride) */
            uint8_t pll_bank[8];
            pll_bank[0] = (uint8_t)((MSNx_P3 >> 8) & 0xFFU);   /* reg 26 */
            pll_bank[1] = (uint8_t)(MSNx_P3 & 0xFFU);           /* reg 27 */
            pll_bank[2] = (uint8_t)((MSNx_P1 >> 16) & 0x03U);   /* reg 28 */
            pll_bank[3] = (uint8_t)((MSNx_P1 >> 8) & 0xFFU);   /* reg 29 */
            pll_bank[4] = (uint8_t)(MSNx_P1 & 0xFFU);           /* reg 30 */
            pll_bank[5] = (uint8_t)(((MSNx_P3 >> 12) & 0x0FU) |
                                    ((MSNx_P2 >> 16) & 0x0FU)); /* reg 31 */
            pll_bank[6] = (uint8_t)((MSNx_P2 >> 8) & 0xFFU);   /* reg 32 */
            pll_bank[7] = (uint8_t)(MSNx_P2 & 0xFFU);           /* reg 33 */

            /* CLK0 → PLLA (stride 0), CLK1/2 → PLLB (stride 8) */
            if (clk > 0)
                pll_stride = 8;

            /* --- Output MS divider registers (integer mode) --- */
            uint32_t MSx_P1 = 128UL * outdivider - 512;

            /* Convert R divider to register bits */
            uint8_t Rbits = 0;
            switch (R)
            {
                case 1:   Rbits = 0;   break;
                case 2:   Rbits = 16;  break;
                case 4:   Rbits = 32;  break;
                case 8:   Rbits = 48;  break;
                case 16:  Rbits = 64;  break;
                case 32:  Rbits = 80;  break;
                case 64:  Rbits = 96;  break;
                case 128: Rbits = 112; break;
            }

            /* Check if output divider changed (avoid unnecessary reset / click) */
            if (dev->omsynth[clk] != (uint16_t)outdivider ||
                dev->o_Rdiv[clk]  != Rbits)
            {
                dev->omsynth[clk] = (uint16_t)outdivider;
                dev->o_Rdiv[clk]  = Rbits;
                msyn_stride = clk * 8;

                /* Special case: MSynth divide-by-4 (see Si5351 datasheet §4.1.3) */
                if (outdivider == 4)
                    Rbits |= 0x0CU;

                /* Output MS register bank (8 bytes at 42 + stride) */
                uint8_t ms_bank[8];
                ms_bank[0] = 0;                                        /* reg 42 */
                ms_bank[1] = 1;                                        /* reg 43 */
                ms_bank[2] = (uint8_t)(((MSx_P1 >> 16) & 0x03U) | Rbits); /* reg 44 */
                ms_bank[3] = (uint8_t)((MSx_P1 >> 8) & 0xFFU);        /* reg 45 */
                ms_bank[4] = (uint8_t)(MSx_P1 & 0xFFU);                /* reg 46 */
                ms_bank[5] = 0;                                        /* reg 47 */
                ms_bank[6] = 0;                                        /* reg 48 */
                ms_bank[7] = 0;                                        /* reg 49 */

                /* Write both banks close together to minimise glitch */
                si5351_write_burst(SI5351_REG_PLLA_MSNA_26 + pll_stride,
                                   pll_bank, 8);
                si5351_write_burst(SI5351_REG_MS0_42 + msyn_stride,
                                   ms_bank, 8);

                /* Reset PLL to apply new divider */
                si5351_reset_pll();
            }
            else
            {
                /* Only PLL values changed — no reset needed */
                si5351_write_burst(SI5351_REG_PLLA_MSNA_26 + pll_stride,
                                   pll_bank, 8);
            }
        }
    }
}

/* ==================================================================
   Power / enable / disable
   ================================================================== */

void si5351_set_power(si5351_t *dev, uint8_t clk, uint8_t drive)
{
    if (dev == NULL || clk >= SI5351_NUM_OUTPUTS)
        return;

    dev->clk_power[clk] = (drive <= SI5351_DRIVE_8mA) ? drive : SI5351_DRIVE_2mA;

    /* Re-enable to apply new drive strength */
    si5351_enable(dev, clk);
}

void si5351_enable(si5351_t *dev, uint8_t clk)
{
    uint8_t ctrl_val;

    if (dev == NULL || clk >= SI5351_NUM_OUTPUTS)
        return;

    /* CLK0 base register = 0x4C, CLK1/2 base = 0x6C */
    if (clk == 0)
        ctrl_val = 0x4C;
    else
        ctrl_val = 0x6C;

    ctrl_val |= (dev->clk_power[clk] & 0x03U);

    si5351_write(SI5351_REG_CLK0_CTRL + clk, ctrl_val);
    dev->clk_on[clk] = 1;

    /* CLK1 and CLK2 are mutually exclusive (shared PLLB) */
    if (clk == 1)
    {
        si5351_disable(dev, 2);
    }
    else if (clk == 2)
    {
        si5351_disable(dev, 1);
    }
}

void si5351_disable(si5351_t *dev, uint8_t clk)
{
    if (dev == NULL || clk >= SI5351_NUM_OUTPUTS)
        return;

    si5351_write(SI5351_REG_CLK0_CTRL + clk, SI5351_CLK_DISABLE_MASK);
    dev->clk_on[clk] = 0;
}

void si5351_disable_all(si5351_t *dev)
{
    uint8_t i;

    if (dev == NULL)
        return;

    for (i = 0; i < SI5351_NUM_OUTPUTS; i++)
        si5351_disable(dev, i);
}

/* ==================================================================
   PLL reset
   ================================================================== */

void si5351_reset_pll(void)
{
    si5351_write(SI5351_REG_PLL_RESET, SI5351_PLL_RESET_BOTH);
}
