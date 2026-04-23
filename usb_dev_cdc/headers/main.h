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
#include "tim0.h"


//#include "flash.h"

#include "menu.h"

#ifdef  DEMO
#include "cm_backtrace.h"
#endif
#define DEFAULT 0
#define USER    1
#define FLASH_LEAF_ADDR(x)          (uint32_t)(0x7a000U+(0x2000U * (x)))

#define HARDWARE_VERSION               "V1.0.0"
#define SOFTWARE_VERSION               "V0.1.0"

#define LED_TIME 0
#define LED_ERR 0



/* 定时器计数*/
extern __IO  uint8_t m_u8SpeedUpd ;




#define EXAMPLE_PERIPH_WE_IIC               (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | \
                                         LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM)
#define EXAMPLE_PERIPH_WP_IIC               (LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_SRAM)

/* Define I2C unit used for the example */
#define I2C_UNIT                        (CM_I2C2)
#define I2C_FCG_USE                     (FCG1_PERIPH_I2C2)
/* Define slave device address for example */
#define DEVICE_ADDR                     (0x3C)
/* I2C address mode */
#define I2C_ADDR_MD_7BIT                (0U)
#define I2C_ADDR_MD_10BIT               (1U)
/* Config I2C address mode: I2C_ADDR_MD_7BIT or I2C_ADDR_MD_10BIT */
#define I2C_ADDR_MD                     (I2C_ADDR_MD_7BIT)

/* Define port and pin for SDA and SCL */
#define I2C_SCL_PORT                    (GPIO_PORT_A)
#define I2C_SCL_PIN                     (GPIO_PIN_04)

#define I2C_SDA_PORT                    (GPIO_PORT_A)
#define I2C_SDA_PIN                     (GPIO_PIN_05)

#define I2C_GPIO_SDA_FUNC               (GPIO_FUNC_50)
#define I2C_GPIO_SCL_FUNC               (GPIO_FUNC_51)


#define TIMEOUT                         (0x40000)

/* Define Write and read data length for the example */
#define TEST_DATA_LEN                   (256U)
/* Define i2c baudrate */

#define I2C_CLK_DIVX                    I2C_CLK_DIV2
#define I2C_BAUDRATE                    (400000)





#endif /* __MAIN_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
