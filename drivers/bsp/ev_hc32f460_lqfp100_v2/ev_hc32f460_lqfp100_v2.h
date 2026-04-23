/**
 *******************************************************************************
 * @file  ev_hc32f460_lqfp100_v2.h
 * @brief This file contains all the functions prototypes of the
 *        EV_HC32F460_LQFP100_V2 BSP driver library.
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
#ifndef __EV_HC32F460_LQFP100_V2_H__
#define __EV_HC32F460_LQFP100_V2_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

  /*******************************************************************************
   * Include files
   ******************************************************************************/
#include "hc32_ll.h"
#include "hc32_ll_aos.h"
#include "hc32_ll_clk.h"
#include "hc32_ll_dma.h"
#include "hc32_ll_efm.h"
#include "hc32_ll_fcg.h"
#include "hc32_ll_gpio.h"
#include "hc32_ll_i2c.h"
#include "hc32_ll_i2s.h"
#include "hc32_ll_interrupts.h"
#include "hc32_ll_keyscan.h"
#include "hc32_ll_pwc.h"
#include "hc32_ll_spi.h"
#include "hc32_ll_sram.h"
#include "hc32_ll_usart.h"
#include "hc32_ll_utility.h"
#include "hc32_ll_tmr0.h"

  /**
   * @addtogroup BSP
   * @{
   */

  /**
   * @addtogroup EV_HC32F460_LQFP100_V2
   * @{
   */

#if (BSP_EV_HC32F460_LQFP100_V2 == BSP_EV_HC32F4XX)

  typedef _Bool BOOL;
/* unlock/lock peripheral */
#define EXAMPLE_PERIPH_WE (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_INTC | \
                           LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM)
#define EXAMPLE_PERIPH_WP (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_INTC | \
                           LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM)

/* BSP XTAL Configure definition */
#define BSP_XTAL_PORT (GPIO_PORT_H)
#define BSP_XTAL_IN_PIN (GPIO_PIN_00)
#define BSP_XTAL_OUT_PIN (GPIO_PIN_01)

  /*******************************************************************************
   * Global type definitions ('typedef')
   ******************************************************************************/

  /*******************************************************************************
   * Global pre-processor symbols/macros ('#define')
   ******************************************************************************/
  /**
   * @defgroup EV_HC32F460_LQFP100_V2_Global_Macros EV_HC32F460_LQFP100_V2 Global Macros
   * @{
   */

/**
 *  LED hardware
 */
#define LED_RED (0x01U)
#define LED_GREEN (0x02U)
#define LED_BLUE (0x04U)
#define LED_ALL (LED_RED | LED_GREEN | LED_BLUE)

#define BSP_LED_RED_PORT (GPIO_PORT_A)
#define BSP_LED_RED_PIN (GPIO_PIN_00)
#define BSP_LED_GREEN_PORT (GPIO_PORT_A)
#define BSP_LED_GREEN_PIN (GPIO_PIN_01)

#define BSP_LED_BLUE_PORT (GPIO_PORT_B) // A2
#define BSP_LED_BLUE_PIN (GPIO_PIN_03)  // ����

/**
 * @defgroup EV_HC32F460_LQFP100_V2_PRINT_CONFIG EV_HC32F460_LQFP100_V2 PRINT Configure definition
 * @{
 */
#define BSP_PRINTF_DEVICE (CM_USART4)
#define BSP_PRINTF_DEVICE_FCG (FCG1_PERIPH_USART4)

#define BSP_PRINTF_BAUDRATE (115200)
#define BSP_PRINTF_BAUDRATE_ERR_MAX (0.025F)

#define BSP_PRINTF_PORT (GPIO_PORT_H)
#define BSP_PRINTF_PIN (GPIO_PIN_02)
#define BSP_PRINTF_PORT_FUNC (GPIO_FUNC_36)

#if (LL_TMR0_ENABLE == DDL_ON)
/* TMR0 unit and channel definition */
#define TMR0_UNIT (CM_TMR0_1)
#define TMR02_UNIT (CM_TMR0_2)
#define TMR0_CLK (FCG2_PERIPH_TMR0_1)
#define TMR0_CHB (TMR0_CH_B)
#define TMR0_CHA (TMR0_CH_A)
#define TMR0_TRIG_CH (AOS_TMR0)
#define TMR0_CHB_INT (TMR0_INT_CMP_B)
#define TMR0_CHA_INT (TMR0_INT_CMP_A)
#define TMR0_CHB_FLAG (TMR0_FLAG_CMP_B)
#define TMR0_CHA_FLAG (TMR0_FLAG_CMP_A)
#define TMR0_INTB_SRC (INT_SRC_TMR0_1_CMP_B)
#define TMR0_INTA_SRC (INT_SRC_TMR0_2_CMP_A)
#define TMR0_IRQn (INT006_IRQn)
#define TMR0_CHA_IRQn (INT007_IRQn)
/* Period = 1 / (Clock freq / div) * (Compare value + 1) = 500ms */
#define TMR0_CMP_VALUE (XTAL32_VALUE / 16U / 2U - 1U)

#define TMR0_CMP_VALUE_1MS 6249
#define TMR0_CMP_VALUE_1US 99
#define TMR0_CMP_VALUE_10US 999

#define SYS_TIMEOUT_10MS (1) // TIMER_BASE_10MS * 1
#define SYS_TIMEOUT_50MS (5) // TIMER_BASE_10MS * 5
#define SYS_TIMEOUT_100MS (9)
#define SYS_TIMEOUT_500MS (50)
#define SYS_TIMEOUT_1SEC (100)
#define SYS_TIMEOUT_2SEC (200)

  void TMR0_CHB_Config(void);
  void TMR02_A_Config(void);
  void TMR02_B_Config(void);
  void TMR01_A_Config(void);
  void timer0_delay_us(uint32_t us);
  void TMR0_CHB_CompareIrqCallback(void);
  void TMR0_CHA_CompareIrqCallback(void);
  uint8_t Key_Read(uint8_t mode);
/**
 * @}
 */
#endif /*(LL_TMR0_ENABLE == DDL_ON)*/
  /*******************************************************************************
   * Global variable definitions ('extern')
   ******************************************************************************/

  /*******************************************************************************
    Global function prototypes (definition in C source)
   ******************************************************************************/
  /**
   * @addtogroup EV_HC32F460_LQFP100_V2_Global_Functions
   * @{
   */
  void BSP_CLK_Init(void);
  void BSP_CLK_DeInit(void);
  void XTAL32_Config(void);
  void BSP_KEY_Init(void);

  void BSP_LED_Init(void);
  void BSP_LED_Sw(uint8_t u8Led); // 0x01->red 0x02->green 0x04->blue
  void BSP_LED_On(uint8_t u8Led);
  void BSP_LED_Off(uint8_t u8Led);
  void BSP_LED_Toggle(uint8_t u8Led);

#if (LL_PRINT_ENABLE == DDL_ON)
  int32_t BSP_PRINTF_Preinit(void *vpDevice, uint32_t u32Baudrate);
#endif

#endif /* BSP_EV_HC32F460_LQFP100_V2 */

#ifdef __cplusplus
}
#endif

#endif /* __EV_HC32F460_LQFP100_V2_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
