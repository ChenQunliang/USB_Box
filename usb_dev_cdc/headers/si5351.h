/**
 * @file si5351.h
 * @brief Si5351A clock generator driver using V_I2C (I2C1, shared with TV5725)
 *
 * Thin C port of the Si5351mcu Arduino library by Pavel Milanes,
 * adapted for HC32F460 + V_I2C layer.
 *
 * CLK0  -> PLLA
 * CLK1  -> PLLB
 * CLK2  -> PLLB
 *
 * VCO range: 600-900 MHz (integer-mode only, error < ±2 Hz typical)
 */

#ifndef SI5351_H
#define SI5351_H

#include "main.h"

/* ==================================================================
   I2C address (7-bit 0x60, left-shifted 1 for V_I2C API)
   ================================================================== */
#define SI5351_ADDR_7BIT    (0x60U)
#define SI5351_ADDR_WR      ((uint16_t)(SI5351_ADDR_7BIT << 1))  /* 0xC0 */
#define SI5351_ADDR_RD      ((uint16_t)(SI5351_ADDR_7BIT << 1) | 1U) /* 0xC1 */

/* Number of outputs (Si5351A 10-pin = 3) */
#define SI5351_NUM_OUTPUTS  (3U)

/* ==================================================================
   Drive-strength settings for CLK control registers
   ================================================================== */
#define SI5351_DRIVE_2mA    (0U)
#define SI5351_DRIVE_4mA    (1U)
#define SI5351_DRIVE_6mA    (2U)
#define SI5351_DRIVE_8mA    (3U)

/* ==================================================================
   Key register addresses
   ================================================================== */
#define SI5351_REG_DEVICE_STATUS        (0x00U)
#define SI5351_REG_CLK0_CTRL            (0x10U)  /* CLK0 control (base), +1 CLK1, +2 CLK2 */
#define SI5351_REG_CLK1_CTRL            (0x11U)
#define SI5351_REG_CLK2_CTRL            (0x12U)

/* PLLA MSNA registers (stride 0) */
#define SI5351_REG_PLLA_MSNA_26         (0x1AU)
/* PLLB MSNB registers (stride 8) */
#define SI5351_REG_PLLB_MSNB_34         (0x22U)

/* MultiSynth dividers: base = 0x2A (CLK0), stride = 8 per channel */
#define SI5351_REG_MS0_42               (0x2AU)
#define SI5351_REG_MS1_42               (0x32U)
#define SI5351_REG_MS2_42               (0x3AU)

#define SI5351_REG_SSEN_SPREAD          (0x95U)  /* bit 7 = spread-spectrum enable */
#define SI5351_REG_PLL_RESET            (0xB1U)  /* write 0xA0 to reset PLL A & B */
#define SI5351_REG_XTAL_LOAD            (0xB7U)  /* crystal load capacitance */

/* CLK control register: bit 7 = output disable */
#define SI5351_CLK_DISABLE_MASK         (0x80U)

/* PLL reset value */
#define SI5351_PLL_RESET_BOTH           (0xA0U)

/* Load-capacitance value for typical 25 MHz crystal (0xD2) */
#define SI5351_XTAL_LOAD_DEFAULT        (0xD2U)

/* ==================================================================
   Device state
   ================================================================== */
typedef struct {
    uint32_t xtal_freq;               /* base crystal frequency (Hz) */
    uint8_t  clk_power[SI5351_NUM_OUTPUTS]; /* per-output drive strength */
    uint16_t omsynth[SI5351_NUM_OUTPUTS];  /* cached output divider */
    uint8_t  o_Rdiv[SI5351_NUM_OUTPUTS];   /* cached R divider */
    uint8_t  clk_on[SI5351_NUM_OUTPUTS];   /* output enabled flags */
    uint8_t  detected;                 /* 1 = device present on bus */
} si5351_t;

/* ==================================================================
   Function prototypes
   ================================================================== */

/* --- Low-level I2C (V_I2C based, shared bus with TV5725) --- */
int32_t si5351_write_burst(uint8_t reg, const uint8_t *data, uint8_t len);
int32_t si5351_write(uint8_t reg, uint8_t value);
int32_t si5351_read(uint8_t reg, uint8_t *value);

/* --- Detection & initialisation --- */
int32_t si5351_detect(si5351_t *dev);
int32_t si5351_init(si5351_t *dev, uint32_t xtal_freq);
void    si5351_external_clock_init(void);

/* --- Frequency control --- */
void    si5351_set_freq(si5351_t *dev, uint8_t clk, uint32_t freq_hz);
void    si5351_set_power(si5351_t *dev, uint8_t clk, uint8_t drive);
void    si5351_enable(si5351_t *dev, uint8_t clk);
void    si5351_disable(si5351_t *dev, uint8_t clk);
void    si5351_disable_all(si5351_t *dev);

/* --- Reset --- */
void    si5351_reset_pll(void);

#endif /* SI5351_H */
