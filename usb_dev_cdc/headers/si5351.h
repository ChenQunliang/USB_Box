/**
 * @file si5351.h
 * @brief Si5351A 时钟发生器驱动，基于 V_I2C（I2C1，与 TV5725 共用总线）
 *
 * 将 Pavel Milanes 的 Si5351mcu Arduino 库移植为 C 语言，
 * 适配 HC32F460 + V_I2C 层。
 *
 * CLK0  -> PLLA
 * CLK1  -> PLLB
 * CLK2  -> PLLB
 *
 * VCO 范围: 600-900 MHz（仅整数模式，典型误差 < ±2 Hz）
 */

#ifndef SI5351_H
#define SI5351_H

#include "main.h"

/* ==================================================================
   I2C 地址（7-bit 0x60，左移 1 位适配 V_I2C API）
   ================================================================== */
#define SI5351_ADDR_7BIT    (0x60U)
#define SI5351_ADDR_WR      ((uint16_t)(SI5351_ADDR_7BIT << 1))  /* 0xC0 */
#define SI5351_ADDR_RD      ((uint16_t)(SI5351_ADDR_7BIT << 1) | 1U) /* 0xC1 */

/* 输出通道数（Si5351A 10-pin = 3） */
#define SI5351_NUM_OUTPUTS  (3U)

/* ==================================================================
   输出驱动强度（CLK 控制寄存器）
   ================================================================== */
#define SI5351_DRIVE_2mA    (0U)
#define SI5351_DRIVE_4mA    (1U)
#define SI5351_DRIVE_6mA    (2U)
#define SI5351_DRIVE_8mA    (3U)

/* ==================================================================
   关键寄存器地址
   ================================================================== */
#define SI5351_REG_DEVICE_STATUS        (0x00U)
#define SI5351_REG_CLK0_CTRL            (0x10U)  /* CLK0 控制（基址），+1 CLK1，+2 CLK2 */
#define SI5351_REG_CLK1_CTRL            (0x11U)
#define SI5351_REG_CLK2_CTRL            (0x12U)

/* PLLA MSNA 寄存器（步进 0） */
#define SI5351_REG_PLLA_MSNA_26         (0x1AU)
/* PLLB MSNB 寄存器（步进 8） */
#define SI5351_REG_PLLB_MSNB_34         (0x22U)

/* MultiSynth 分频器：基址 = 0x2A（CLK0），每通道步进 8 */
#define SI5351_REG_MS0_42               (0x2AU)
#define SI5351_REG_MS1_42               (0x32U)
#define SI5351_REG_MS2_42               (0x3AU)

#define SI5351_REG_SSEN_SPREAD          (0x95U)  /* bit 7 = 扩频使能 */
#define SI5351_REG_PLL_RESET            (0xB1U)  /* 写入 0xA0 复位 PLL A & B */
#define SI5351_REG_XTAL_LOAD            (0xB7U)  /* 晶振负载电容 */

/* CLK 控制寄存器：bit 7 = 输出禁用 */
#define SI5351_CLK_DISABLE_MASK         (0x80U)

/* PLL 复位值 */
#define SI5351_PLL_RESET_BOTH           (0xA0U)

/* 典型 25 MHz 晶振的负载电容值（0xD2） */
#define SI5351_XTAL_LOAD_DEFAULT        (0xD2U)

/* ==================================================================
   设备状态结构体
   ================================================================== */
typedef struct {
    uint32_t xtal_freq;                      /* 基频晶振频率（Hz） */
    uint8_t  clk_power[SI5351_NUM_OUTPUTS];  /* 每通道驱动强度 */
    uint16_t omsynth[SI5351_NUM_OUTPUTS];    /* 缓存的输出分频器值 */
    uint8_t  o_Rdiv[SI5351_NUM_OUTPUTS];     /* 缓存的 R 分频器值 */
    uint8_t  clk_on[SI5351_NUM_OUTPUTS];     /* 输出使能标志 */
    uint8_t  detected;                       /* 1 = 总线上检测到设备 */
} si5351_t;

/* ==================================================================
   函数原型
   ================================================================== */

/* --- 底层 I2C（基于 V_I2C，与 TV5725 共用总线） --- */
int32_t si5351_write_burst(uint8_t reg, const uint8_t *data, uint8_t len);
int32_t si5351_write(uint8_t reg, uint8_t value);
int32_t si5351_read(uint8_t reg, uint8_t *value);

/* --- 检测与初始化 --- */
int32_t si5351_detect(si5351_t *dev);
int32_t si5351_init(si5351_t *dev, uint32_t xtal_freq);
void    si5351_external_clock_init(void);

/* --- 频率控制 --- */
void    si5351_set_freq(si5351_t *dev, uint8_t clk, uint32_t freq_hz);
void    si5351_set_power(si5351_t *dev, uint8_t clk, uint8_t drive);
void    si5351_enable(si5351_t *dev, uint8_t clk);
void    si5351_disable(si5351_t *dev, uint8_t clk);
void    si5351_disable_all(si5351_t *dev);

/* --- 复位 --- */
void    si5351_reset_pll(void);

#endif /* SI5351_H */
