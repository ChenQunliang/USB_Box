/**
 *******************************************************************************
 * @file usb/usb_dev_cdc/source/main.h
 * @brief This file contains the including files of main routine.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2022-03-31       CDT             First version
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */
#ifndef __MAIN_H__
#define __MAIN_H__

#include "hc32_ll.h"
#include "ev_hc32f460_lqfp100_v2.h"
#include "EncodTimA.h"
#include "i2c.h"
//#include "tim0.h"

// #include "flash.h"

#include "menu.h"

#ifdef DEMO
#include "cm_backtrace.h"
#endif
#define DEFAULT 0
#define USER 1
#define FLASH_LEAF_ADDR(x) (uint32_t)(0x7a000U + (0x2000U * (x)))

#define LED_TIME 35

#define LED_ERR_RED     0x80
#define LED_ERR_GREEN   0x40
#define LED_ERR_BLUE    0x20
#define LED_OK          0x10



extern __IO uint8_t m_u8SpeedUpd;

#define EXAMPLE_PERIPH_WE_IIC (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | \
                               LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM)
#define EXAMPLE_PERIPH_WP_IIC (LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_SRAM)

/* Define I2C unit used for the example */
#define U8G2_I2C_UNIT (CM_I2C2)
#define U8G2_I2C_FCG_USE (FCG1_PERIPH_I2C2)
/* Define slave device address for example */
#define U8G2_DEVICE_ADDR (0x3C)
/* I2C address mode */
#define U8G2_I2C_ADDR_MD_7BIT (0U)
#define U8G2_I2C_ADDR_MD_10BIT (1U)
/* Config I2C address mode: I2C_ADDR_MD_7BIT or I2C_ADDR_MD_10BIT */
#define U8G2_I2C_ADDR_MD (U8G2_I2C_ADDR_MD_7BIT)

/* Define port and pin for SDA and SCL */
#define U8G2_I2C_SCL_PORT (GPIO_PORT_A)
#define U8G2_I2C_SCL_PIN (GPIO_PIN_06)

#define U8G2_I2C_SDA_PORT (GPIO_PORT_A)
#define U8G2_I2C_SDA_PIN (GPIO_PIN_07)

#define U8G2_I2C_GPIO_SDA_FUNC (GPIO_FUNC_50)
#define U8G2_I2C_GPIO_SCL_FUNC (GPIO_FUNC_51)

#define U8G2_TIMEOUT (0x40000)

/* Define i2c baudrate */

#define U8G2_I2C_CLK_DIVX I2C_CLK_DIV2
#define U8G2_I2C_BAUDRATE (400000)

#endif /* __MAIN_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
