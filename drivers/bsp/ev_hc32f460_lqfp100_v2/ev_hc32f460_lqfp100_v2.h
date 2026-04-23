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
#define EXAMPLE_PERIPH_WE               (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_INTC |\
                                         LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM)
#define EXAMPLE_PERIPH_WP               (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_INTC |\
                                         LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM)



/* BSP XTAL Configure definition */
#define BSP_XTAL_PORT                   (GPIO_PORT_H)
#define BSP_XTAL_IN_PIN                 (GPIO_PIN_00)
#define BSP_XTAL_OUT_PIN                (GPIO_PIN_01)

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


#define MS8006F_MCULIB_VERSION_FIRST      1
#define MS8006F_MCULIB_VERSION_SECOND     0
#define MS8006F_MCULIB_VERSION_REVISE     8

#define MS8006F_MCULIB_VERSION_PHASE      "Beta"


/**********/

#define MS8006F_MCULIB_VERISION_SRT       "MS8006F mculib Rev: %d.%d.%d_%s_%s\r\n\r\n", \
                                            MS8006F_MCULIB_VERSION_FIRST, \
                                            MS8006F_MCULIB_VERSION_SECOND, \
                                            MS8006F_MCULIB_VERSION_REVISE, \
                                            __DATE__, \
                                            MS8006F_MCULIB_VERSION_PHASE




typedef enum _KEY_MAP_
{
    KEY_MAP_0 = 0,
    KEY_MAP_1,
    KEY_MAP_2,
    KEY_MAP_3,
    KEY_MAP_4,
    KEY_MAP_5,
    KEY_MAP_6,
    KEY_MAP_7,
    KEY_MAP_LONG0,
    KEY_MAP_LONG1,
    KEY_MAP_LONG2,
    KEY_MAP_LONG3,
    KEY_MAP_LONG4,
    KEY_MAP_LONG5,
    KEY_MAP_LONG6,
    KEY_MAP_LONG7,
    KEY_NONE = 0xFF
} KEY_MAP_E;


#define IR_KEY_PORT   (GPIO_PORT_B)
#define IR_KEY_PIN	  (GPIO_PIN_09)

#define IR_KEY_EXTINT_CH         (EXTINT_CH09)
#define IR_KEY_INT_SRC           (INT_SRC_PORT_EIRQ9)
#define IR_KEY_INT_IRQn          (INT003_IRQn)
#define IR_KEY_INT_PRIO          (DDL_IRQ_PRIO_DEFAULT)


#define IR_KEY_UP             	((uint8_t)0x07)//UP
#define IR_KEY_DWON           	((uint8_t)0x05)//DWON
#define IR_KEY_LEFT             ((uint8_t)0x00)//LEFT 
#define IR_KEY_RIGHT            ((uint8_t)0x01)//RIGHT 
#define IR_KEY_OK      	        ((uint8_t)0x02)//OK
#define IR_KEY_MENU          	  ((uint8_t)0x03)//EXIT/MENU

#define IR_KEY_POWER            ((uint8_t)0x3F)
#define IR_KEY_OTHER            ((uint8_t)0xFF)
#define IR_KEY_REPEAT			      ((uint8_t)0x80)

#define LOG DDL_Printf
#define LOG1 DDL_Printf



/**
 * @}
 */

/**
 *  LED hardware 
 */
#define LED_RED                 (0x01U)
#define LED_GREEN               (0x02U)
#define LED_BLUE                (0x04U)
#define LED_ALL                 (LED_RED | LED_GREEN| LED_BLUE)



#define BSP_LED_RED_PORT        (GPIO_PORT_A)
#define BSP_LED_RED_PIN         (GPIO_PIN_00)
#define BSP_LED_GREEN_PORT      (GPIO_PORT_A)
#define BSP_LED_GREEN_PIN       (GPIO_PIN_01)

#define BSP_LED_BLUE_PORT     	(GPIO_PORT_B) //A2
#define BSP_LED_BLUE_PIN      	(GPIO_PIN_03) //更改


/**
 *  USER Button hardware 
 */
#define USER_BUTTON_PORT              (GPIO_PORT_B)
#define USER_BUTTON_PIN               (GPIO_PIN_15)
#define USER_BUTTON_EXTINT_CH         (EXTINT_CH15)
#define USER_BUTTON_INT_SRC           (INT_SRC_PORT_EIRQ15)
#define USER_BUTTON_INT_IRQn          (INT001_IRQn)
#define USER_BUTTON_GRP_INT_IRQn      (INT033_IRQn)
#define USER_BUTTON_SHARE_INT_IRQn    (INT128_IRQn)
#define USER_BUTTON_INT_PRIO          (DDL_IRQ_PRIO_DEFAULT)



#define MSCHIP_RESET_PORT   (GPIO_PORT_B)  //A3
#define MSCHIP_RESET_PIN    (GPIO_PIN_04)  //更改

/**
 *  Resolution Switcher Hardware
 */
#define BSP_R_SWITCH_PORT      	 (GPIO_PORT_C)
#define BSP_R_SWITCH_PIN				 (GPIO_PIN_14)
#define BSP_OSD_SWITCH_PORT      (GPIO_PORT_A)
#define BSP_OSD_SWITCH_PIN			 (GPIO_PIN_15)
#define BSP_MODE_KEY_PORT				 (GPIO_PORT_A)
#define BSP_MODE_KEY_PIN         (GPIO_PIN_08)
#define  PIN_MODE_KEY_SWITCH_4X3    GPIO_ReadInputPins(BSP_MODE_KEY_PORT, BSP_MODE_KEY_PIN)
#define 	PIN_KEY_OSD_SWITCH				  GPIO_ReadInputPins(BSP_OSD_SWITCH_PORT, BSP_OSD_SWITCH_PIN)
#define  PIN_KEY_R_SWITCH            GPIO_ReadInputPins(BSP_R_SWITCH_PORT, BSP_R_SWITCH_PIN)

/**
 * @defgroup EV_HC32F460_LQFP100_V2_PRINT_CONFIG EV_HC32F460_LQFP100_V2 PRINT Configure definition
 * @{
 */
#define BSP_PRINTF_DEVICE               (CM_USART4)
#define BSP_PRINTF_DEVICE_FCG           (FCG1_PERIPH_USART4)

#define BSP_PRINTF_BAUDRATE             (115200)
#define BSP_PRINTF_BAUDRATE_ERR_MAX     (0.025F)

#define BSP_PRINTF_PORT                 (GPIO_PORT_H)
#define BSP_PRINTF_PIN                  (GPIO_PIN_02)
#define BSP_PRINTF_PORT_FUNC            (GPIO_FUNC_36)
/**
 * @}
 */
//#if (LL_I2C_ENABLE == DDL_ON)
///* Define port and pin for SDA and SCL */
////#define I2C_SCL_PORT                    (GPIO_PORT_C)
////#define I2C_SCL_PIN                     (GPIO_PIN_00)
////#define I2C_SDA_PORT                    (GPIO_PORT_C)
////#define I2C_SDA_PIN                     (GPIO_PIN_01)
////#define I2C_GPIO_SCL_FUNC               (GPIO_FUNC_49)
////#define I2C_GPIO_SDA_FUNC               (GPIO_FUNC_48)

//#define I2C_SCL_PORT                    (GPIO_PORT_A)
//#define I2C_SCL_PIN                     (GPIO_PIN_04)
//#define I2C_SDA_PORT                    (GPIO_PORT_A)
//#define I2C_SDA_PIN                     (GPIO_PIN_05)
//#define I2C_GPIO_SCL_FUNC               (GPIO_FUNC_51)
//#define I2C_GPIO_SDA_FUNC               (GPIO_FUNC_50)



//#define TIMEOUT                         (0x40000UL)

///* Define Write and read data length for the example */
//#define TEST_DATA_LEN                   (256U)
///* Define i2c baudrate */
//#define I2C_BAUDRATE                    (400000UL)

///* Define I2C unit used for the example */
//#define I2C_UNIT                        (CM_I2C2)
//#define I2C_FCG_USE                     (FCG1_PERIPH_I2C2)
///* Define slave device address for example */
//#define DEVICE_ADDR                     (0x3C)

///* Define slave device address for example */
////#define DEVICE_ADDR                     (0x06U)
//int32_t I2C_Master_Transmit(uint16_t u16DevAddr, uint8_t const au8Data[], uint32_t u32Size, uint32_t u32Timeout);
//int32_t I2C_Master_Receive(uint16_t u16DevAddr, uint8_t au8Data[], uint32_t u32Size, uint32_t u32Timeout);
//int32_t I2C_Master_Initialize(void);
//void I2C_Master_DeInitialize(void);
//#endif  /*LL_I2C_ENABLE == DDL_ON*/

#if (LL_TMR0_ENABLE == DDL_ON)
/* TMR0 unit and channel definition */
#define TMR0_UNIT                       (CM_TMR0_1)
#define TMR02_UNIT                      (CM_TMR0_2)
#define TMR0_CLK                        (FCG2_PERIPH_TMR0_1)
#define TMR0_CHB                         (TMR0_CH_B)
#define TMR0_CHA                         (TMR0_CH_A)
#define TMR0_TRIG_CH                    (AOS_TMR0)
#define TMR0_CHB_INT                     (TMR0_INT_CMP_B)
#define TMR0_CHA_INT                      (TMR0_INT_CMP_A)
#define TMR0_CHB_FLAG                    (TMR0_FLAG_CMP_B)
#define TMR0_CHA_FLAG                    (TMR0_FLAG_CMP_A)
#define TMR0_INTB_SRC                    (INT_SRC_TMR0_1_CMP_B)
#define TMR0_INTA_SRC                    (INT_SRC_TMR0_2_CMP_A)
#define TMR0_IRQn                       (INT006_IRQn)
#define TMR0_CHA_IRQn                   (INT007_IRQn)
/* Period = 1 / (Clock freq / div) * (Compare value + 1) = 500ms */
#define TMR0_CMP_VALUE                  (XTAL32_VALUE / 16U / 2U - 1U)

#define TMR0_CMP_VALUE_1MS               6249
#define TMR0_CMP_VALUE_1US    					 99
#define TMR0_CMP_VALUE_10US    					 999

#define SYS_TIMEOUT_10MS    (1)   // TIMER_BASE_10MS * 1
#define SYS_TIMEOUT_50MS    (5)   // TIMER_BASE_10MS * 5
#define SYS_TIMEOUT_100MS   (9)   
#define SYS_TIMEOUT_500MS   (50)
#define SYS_TIMEOUT_1SEC    (100)  
#define SYS_TIMEOUT_2SEC    (200)


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
#endif  /*(LL_TMR0_ENABLE == DDL_ON)*/
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
en_flag_status_t BSP_KEY_GetStatus(uint32_t u32Key);

void BSP_LED_Init(void);
void BSP_LED_Sw(uint8_t u8Led);  //0x01->red 0x02->green 0x04->blue
void BSP_LED_On(uint8_t u8Led);
void BSP_LED_Off(uint8_t u8Led);
void BSP_LED_Toggle(uint8_t u8Led);
uint8_t mculib_ir_detect(void);
#if (LL_PRINT_ENABLE == DDL_ON)
int32_t BSP_PRINTF_Preinit(void* vpDevice, uint32_t u32Baudrate);
#endif


void mculib_chip_reset(void);
void mculib_key_exti_init(void);

/* It can't get the status of the KEYx by calling BSP_KEY_GetStatus when you re-implement BSP_KEY_KEYx_IrqHandler. */
//void BSP_KEY_KEY10_IrqHandler(void);




void  USER_BUTTON_Init(void);
void USER_SWITCHS_Init(void);


//void EXTINT_USER_BUTTON_IrqCallback(void);



/**
 * @}
 */

#endif /* BSP_EV_HC32F460_LQFP100_V2 */
/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __EV_HC32F460_LQFP100_V2_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
