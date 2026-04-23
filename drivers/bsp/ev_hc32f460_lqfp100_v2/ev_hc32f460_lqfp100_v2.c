/**
 *******************************************************************************
 * @file  ev_hc32f460_lqfp100_v2.c
 * @brief This file provides firmware functions for EV_HC32F460_LQFP100_V2 BSP
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

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "ev_hc32f460_lqfp100_v2.h"
#include <string.h>

#include "main.h"

/**
 * @addtogroup BSP
 * @{
 */

/**
 * @defgroup EV_HC32F460_LQFP100_V2 EV_HC32F460_LQFP100_V2
 * @{
 */
// modify by user
uint16_t g_u16_sys_timer;
uint16_t g_u16_key_timer;
uint16_t g_u16_mis_timer;
uint16_t g_u16_osd_timer;
uint16_t g_u16_rgbs_timer;

uint16_t g_u16_led_timer;
uint8_t led_state = 0;
uint8_t led_sw = 0;
uint8_t rgbs_dir = 0;



volatile unsigned long g_SystemTicks=0;
/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
typedef struct
{
    uint8_t port;
    uint16_t pin;
} BSP_Port_Pin;

typedef struct
{
    uint8_t port;
    uint16_t pin;
    uint32_t ch;
    en_int_src_t int_src;
    IRQn_Type irq;
    func_ptr_t callback;
} BSP_KeyIn_Config;

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/**
 * @brief  KEY10(K10) initialize
 * @param  None
 * @retval None
 */
void USER_SWITCHS_Init(void)
{
    stc_gpio_init_t stcGpioInit;

    /* GPIO config */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PullUp = PIN_PU_ON;
    stcGpioInit.u16PinDir = PIN_DIR_IN;
    stcGpioInit.u16PinAttr = PIN_ATTR_DIGITAL;

    GPIO_SetDebugPort(GPIO_PIN_TDI, DISABLE);
    GPIO_SubFuncCmd(BSP_OSD_SWITCH_PORT, BSP_OSD_SWITCH_PIN, DISABLE);
    GPIO_SubFuncCmd(BSP_MODE_KEY_PORT, BSP_MODE_KEY_PIN, DISABLE);

    (void)GPIO_Init(BSP_R_SWITCH_PORT, BSP_R_SWITCH_PIN, &stcGpioInit);
    (void)GPIO_Init(BSP_OSD_SWITCH_PORT, BSP_OSD_SWITCH_PIN, &stcGpioInit);

    (void)GPIO_Init(BSP_MODE_KEY_PORT, BSP_MODE_KEY_PIN, &stcGpioInit);
}

// reset for macrosilicon chip
void mculib_chip_reset(void)
{
    stc_gpio_init_t stcGpioInit;
    /* GPIO config */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PullUp = PIN_PU_ON;
    stcGpioInit.u16PinState = PIN_SET;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    (void)GPIO_Init(MSCHIP_RESET_PORT, MSCHIP_RESET_PIN, &stcGpioInit);

    GPIO_ResetPins(MSCHIP_RESET_PORT, MSCHIP_RESET_PIN);
    mculib_delay_ms(100);
    GPIO_SetPins(MSCHIP_RESET_PORT, MSCHIP_RESET_PIN);
}

/**
 * @brief  KEY10(K10) External interrupt Ch.1 callback function
 *         IRQ No.1 in Global IRQ entry No.0~31 is used for EXTINT1
 * @param  None
 * @retval None
 */
void EXTINT_USER_BUTTON_IrqCallback(void)
{

    if (SET == EXTINT_GetExtIntStatus(USER_BUTTON_EXTINT_CH))
    {

        while (PIN_RESET == GPIO_ReadInputPins(USER_BUTTON_PORT, USER_BUTTON_PIN))
        {
            ;
        }
        EXTINT_ClearExtIntStatus(USER_BUTTON_EXTINT_CH);
    }
}

/**
 * @brief  KEY10(K10) initialize
 * @param  None
 * @retval None
 */
void USER_BUTTON_Init(void)
{
    stc_extint_init_t stcExtIntInit;
    stc_irq_signin_config_t stcIrqSignConfig;
    stc_gpio_init_t stcGpioInit;

    /* GPIO config */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16ExtInt = PIN_EXTINT_ON;
    stcGpioInit.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(USER_BUTTON_PORT, USER_BUTTON_PIN, &stcGpioInit);

    /* ExtInt config */
    (void)EXTINT_StructInit(&stcExtIntInit);
    stcExtIntInit.u32Filter = EXTINT_FILTER_ON;
    stcExtIntInit.u32FilterClock = EXTINT_FCLK_DIV8;
    stcExtIntInit.u32Edge = EXTINT_TRIG_FALLING;
    (void)EXTINT_Init(USER_BUTTON_EXTINT_CH, &stcExtIntInit);

    /* IRQ sign-in */
    stcIrqSignConfig.enIntSrc = USER_BUTTON_INT_SRC;
    stcIrqSignConfig.enIRQn = USER_BUTTON_INT_IRQn;
    stcIrqSignConfig.pfnCallback = &EXTINT_USER_BUTTON_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* NVIC config */
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, USER_BUTTON_INT_PRIO);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);
}

#if (LL_TMR0_ENABLE == DDL_ON)
/**
 * @brief  TMR0 compare interrupt callback function.
 * @param  None
 * @retval None
 */
static uint32_t timer0_CHB_count = 0;
void TMR0_CHB_CompareIrqCallback(void) // 10US
{
    static uint32_t u32TmrCnt = 0U;
    if (timer0_CHB_count % 100 == 0) // 1ms
    {
        g_SystemTicks++;

        u32TmrCnt++;
        if (u32TmrCnt >= 10U)
        {   
            m_u8SpeedUpd = 1U;

            u32TmrCnt = 0U;
        }
    }
    
    if (timer0_CHB_count % 1000 == 0) // 10ms
    {
        g_u16_sys_timer++;
        g_u16_key_timer++;
        g_u16_mis_timer++;
        g_u16_osd_timer++;

        if ((led_sw & 0x01) && (led_sw & LED_ERR))
        {
            g_u16_led_timer++;
            BSP_LED_Sw(0x01);
            if (g_u16_led_timer >= LED_TIME)
            {
                g_u16_led_timer = 0;
                led_sw = 0;
            }
        }
        else if (led_sw & 0x01)
        {
            g_u16_led_timer++;
            BSP_LED_Sw(0x00);
            if (g_u16_led_timer >= 50)
            {
                g_u16_led_timer = 0;
                led_sw = 0;
            }
        }
        else
            BSP_LED_Sw(led_state);
    }
    timer0_CHB_count++;

    TMR0_ClearStatus(CM_TMR0_2, TMR0_FLAG_CMP_B);
}

void TMR01_CHA_CompareIrqCallback(void)
{
    // BSP_LED_Toggle(LED_BLUE);
    TMR0_ClearStatus(CM_TMR0_1, TMR0_FLAG_CMP_A);
}

/**
 * @brief  Configure TMR0.
 * @note   In asynchronous clock, If you want to write a TMR0 register, you need to wait for
 *         at least 3 asynchronous clock cycles after the last write operation!
 * @param  None
 * @retval None
 */

void TMR02_A_Config(void)
{
    stc_tmr0_init_t stcTmr0Init;

    /* Enable timer0 and AOS clock */
    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_2, ENABLE);
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

    /* TIMER0 configuration */
    (void)TMR0_StructInit(&stcTmr0Init);
    stcTmr0Init.u32ClockSrc = TMR0_CLK_SRC_INTERN_CLK; // 时钟源采用内部低速振荡器
    stcTmr0Init.u32ClockDiv = TMR0_CLK_DIV1;
    stcTmr0Init.u32Func = TMR0_FUNC_CMP;
    stcTmr0Init.u16CompareValue = (uint16_t)TMR0_CMP_VALUE_1US;

    (void)TMR0_Init(CM_TMR0_2, TMR0_CH_A, &stcTmr0Init);
    DDL_DelayMS(1U);
    TMR0_HWStopCondCmd(CM_TMR0_2, TMR0_CH_A, ENABLE);
    DDL_DelayMS(1U);
}

void TMR01_A_Config(void)
{
    stc_tmr0_init_t stcTmr0Init;
    stc_irq_signin_config_t stcIrqSignConfig;

    /* Enable timer0 and AOS clock */
    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_1, ENABLE);
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

    /* TIMER0 configuration */
    (void)TMR0_StructInit(&stcTmr0Init);
    stcTmr0Init.u32ClockSrc = TMR0_CLK_SRC_LRC; // 时钟源采用内部低速振荡器
    stcTmr0Init.u32ClockDiv = TMR0_CLK_DIV16;
    stcTmr0Init.u32Func = TMR0_FUNC_CMP;
    stcTmr0Init.u16CompareValue = (uint16_t)TMR0_CMP_VALUE;

    (void)TMR0_Init(CM_TMR0_1, TMR0_CH_A, &stcTmr0Init);
    DDL_DelayMS(1U);
    TMR0_HWStopCondCmd(CM_TMR0_1, TMR0_CH_A, ENABLE);
    DDL_DelayMS(1U);
    TMR0_IntCmd(CM_TMR0_1, TMR0_INT_CMP_A, ENABLE);
    DDL_DelayMS(1U);
    // AOS_SetTriggerEventSrc(TMR0_TRIG_CH, BSP_KEY_KEY10_EVT);
    /* Interrupt configuration */
    stcIrqSignConfig.enIntSrc = INT_SRC_TMR0_1_CMP_A;
    stcIrqSignConfig.enIRQn = INT008_IRQn;
    stcIrqSignConfig.pfnCallback = &TMR01_CHA_CompareIrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);
}

void TMR02_B_Config(void)
{
    stc_tmr0_init_t stcTmr0Init;
    stc_irq_signin_config_t stcIrqSignConfig;

    /* Enable timer0 and AOS clock */
    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_2, ENABLE);
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

    /* TIMER0 configuration */
    (void)TMR0_StructInit(&stcTmr0Init);
    stcTmr0Init.u32ClockSrc = TMR0_CLK_SRC_INTERN_CLK; // 时钟源采用内部低速振荡器
    stcTmr0Init.u32ClockDiv = TMR0_CLK_DIV1;
    stcTmr0Init.u32Func = TMR0_FUNC_CMP;
    stcTmr0Init.u16CompareValue = (uint16_t)TMR0_CMP_VALUE_10US;

    (void)TMR0_Init(CM_TMR0_2, TMR0_CH_B, &stcTmr0Init);
    DDL_DelayMS(1U);
    TMR0_HWStopCondCmd(CM_TMR0_2, TMR0_CH_B, ENABLE);
    DDL_DelayMS(1U);
    TMR0_IntCmd(CM_TMR0_2, TMR0_INT_CMP_B, ENABLE);
    DDL_DelayMS(1U);
    // AOS_SetTriggerEventSrc(TMR0_TRIG_CH, BSP_KEY_KEY10_EVT);
    /* Interrupt configuration */
    stcIrqSignConfig.enIntSrc = INT_SRC_TMR0_2_CMP_B;
    stcIrqSignConfig.enIRQn = INT007_IRQn;
    stcIrqSignConfig.pfnCallback = &TMR0_CHB_CompareIrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);
}

// 使用一个定时器延时1us, 该函数最大的精确时间是300us
void timer0_delay_us(uint32_t us)
{
    if (us > 300)
    {
        us = 300;
        printf("%s %d param us is overrun \r\n", __FILE__, __LINE__);
    }
    // 计数器清零
    TMR0_SetCountValue(CM_TMR0_2, TMR0_CH_A, 0);
    // 设置重载值，也就是需要计数的最大值
    TMR0_SetCompareValue(CM_TMR0_2, TMR0_CH_A, us * 97); // 将Timter0 2单元 A通道 作为延迟函数使用的定时器

    // 开始计数
    TMR0_Start(CM_TMR0_2, TMR0_CHA);
    // 等待计数事件到
    while (!TMR0_GetStatus(CM_TMR0_2, TMR0_FLAG_CMP_A))
        ;
    // 清除转态标志
    TMR0_ClearStatus(CM_TMR0_2, TMR0_FLAG_CMP_A);
    // 停止计数
    TMR0_Stop(CM_TMR0_2, TMR0_CHA);
}

#endif
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/**
 * @brief  Configure XTAL32.
 * @param  None
 * @retval None
 */
void XTAL32_Config(void)
{
    stc_clock_xtal32_init_t stcXtal32Init;

    (void)CLK_Xtal32StructInit(&stcXtal32Init);
    /* Configure Xtal32 */
    stcXtal32Init.u8State = CLK_XTAL32_ON;
    stcXtal32Init.u8Drv = CLK_XTAL32_DRV_MID;
    stcXtal32Init.u8Filter = CLK_XTAL32_FILTER_OFF;
    (void)CLK_Xtal32Init(&stcXtal32Init);
}

/**
 * @brief  BSP clock initialize.
 *         Set board system clock to MPLL@200MHz
 * @param  None
 * @retval None
 */
__WEAKDEF void BSP_CLK_Init(void)
{
    //    stc_clock_xtal_init_t     stcXtalInit;
    //    stc_clock_pll_init_t      stcMpllInit;

    //    (void)CLK_XtalStructInit(&stcXtalInit);
    //    (void)CLK_PLLStructInit(&stcMpllInit);

    //    /* Set bus clk div. */
    //    CLK_SetClockDiv(CLK_BUS_CLK_ALL, (CLK_HCLK_DIV1 | CLK_EXCLK_DIV2 | CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | \
//                                      CLK_PCLK2_DIV4 | CLK_PCLK3_DIV4 | CLK_PCLK4_DIV2));

    //    /* Config Xtal and enable Xtal */
    //    stcXtalInit.u8Mode = CLK_XTAL_MD_OSC;
    //    stcXtalInit.u8Drv = CLK_XTAL_DRV_ULOW;
    //    stcXtalInit.u8State = CLK_XTAL_ON;
    //    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    //    (void)CLK_XtalInit(&stcXtalInit);

    //    /* MPLL config (XTAL / pllmDiv * plln / PllpDiv = 200M). */
    //    stcMpllInit.PLLCFGR = 0UL;
    //    stcMpllInit.PLLCFGR_f.PLLM = 1UL - 1UL;
    //    stcMpllInit.PLLCFGR_f.PLLN = 50UL - 1UL;
    //    stcMpllInit.PLLCFGR_f.PLLP = 2UL - 1UL;
    //    stcMpllInit.PLLCFGR_f.PLLQ = 2UL - 1UL;
    //    stcMpllInit.PLLCFGR_f.PLLR = 2UL - 1UL;
    //    stcMpllInit.u8PLLState = CLK_PLL_ON;
    //    stcMpllInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    //    (void)CLK_PLLInit(&stcMpllInit);
    //    /* Wait MPLL ready. */
    //    while(SET != CLK_GetStableStatus(CLK_STB_FLAG_PLL))
    //    {
    //        ;
    //    }

    //    /* sram init include read/write wait cycle setting */
    //    SRAM_Init();
    //    SRAM_SetWaitCycle(SRAM_SRAM_ALL, SRAM_WAIT_CYCLE1, SRAM_WAIT_CYCLE1);
    //    SRAM_SetWaitCycle(SRAM_SRAMH, SRAM_WAIT_CYCLE0, SRAM_WAIT_CYCLE0);
    //
    //    /* flash read wait cycle setting */
    //    (void)EFM_SetWaitCycle(EFM_WAIT_CYCLE5);
    //    /* 3 cycles for 126MHz ~ 200MHz */
    //    GPIO_SetReadWaitCycle(GPIO_RD_WAIT3);
    //    /* Switch driver ability */
    //    (void)PWC_HighSpeedToHighPerformance();
    //    /* Switch system clock source to MPLL. */
    //    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);

    stc_clock_xtal_init_t stcXtalInit;
    stc_clock_pll_init_t stcMpllInit;

    /* Set bus clk div. */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL, (CLK_HCLK_DIV1 | CLK_EXCLK_DIV2 | CLK_PCLK0_DIV1 |
                                      CLK_PCLK1_DIV2 | CLK_PCLK2_DIV4 | CLK_PCLK3_DIV4 | CLK_PCLK4_DIV2));

    GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_IN_PIN | BSP_XTAL_OUT_PIN, ENABLE);
    (void)CLK_XtalStructInit(&stcXtalInit);
    /* Config Xtal and enable Xtal */
    stcXtalInit.u8Mode = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv = CLK_XTAL_DRV_LOW;
    stcXtalInit.u8State = CLK_XTAL_ON;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    (void)CLK_XtalInit(&stcXtalInit);

    (void)CLK_PLLStructInit(&stcMpllInit);
    /* MPLL config (XTAL / pllmDiv * plln / PllpDiv = 200M). */
    stcMpllInit.PLLCFGR = 0UL;
    stcMpllInit.PLLCFGR_f.PLLM = 1UL - 1UL;
    stcMpllInit.PLLCFGR_f.PLLN = 50UL - 1UL;
    stcMpllInit.PLLCFGR_f.PLLP = 2UL - 1UL;
    stcMpllInit.PLLCFGR_f.PLLQ = 2UL - 1UL;
    stcMpllInit.PLLCFGR_f.PLLR = 2UL - 1UL;
    stcMpllInit.u8PLLState = CLK_PLL_ON;
    stcMpllInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcMpllInit);
    /* Wait MPLL ready. */
    while (SET != CLK_GetStableStatus(CLK_STB_FLAG_PLL))
    {
    }

    /* sram init include read/write wait cycle setting */
    SRAM_Init();
    SRAM_SetWaitCycle(SRAM_SRAM_ALL, SRAM_WAIT_CYCLE1, SRAM_WAIT_CYCLE1);
    SRAM_SetWaitCycle(SRAM_SRAMH, SRAM_WAIT_CYCLE0, SRAM_WAIT_CYCLE0);
    /* 3 cycles for 126MHz ~ 200MHz */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT3);
    /* flash read wait cycle setting */
    EFM_SetWaitCycle(EFM_WAIT_CYCLE5);
    /* Switch driver ability */
    (void)PWC_HighSpeedToHighPerformance();
    /* Switch system clock source to MPLL. */
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);
}
/**
 * @brief  IAP clock De-Initialize.
 * @param  None
 * @retval None
 */
void BSP_CLK_DeInit(void)
{
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_MRC);
    /* Switch driver ability */
    (void)PWC_HighPerformanceToLowSpeed();
    /* Set bus clk div. */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL, (CLK_HCLK_DIV1 | CLK_EXCLK_DIV1 | CLK_PCLK0_DIV1 |
                                      CLK_PCLK1_DIV1 | CLK_PCLK2_DIV1 | CLK_PCLK3_DIV1 | CLK_PCLK4_DIV1));
    CLK_XtalCmd(DISABLE);
    CLK_PLLCmd(DISABLE);
    /* sram init include read/write wait cycle setting */
    SRAM_SetWaitCycle(SRAM_SRAM_ALL, SRAM_WAIT_CYCLE0, SRAM_WAIT_CYCLE0);
    SRAM_SetWaitCycle(SRAM_SRAMH, SRAM_WAIT_CYCLE0, SRAM_WAIT_CYCLE0);
    /* 0 cycles */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT0);
    /* flash read wait cycle setting */
    EFM_SetWaitCycle(EFM_WAIT_CYCLE0);
}

/**
 * @brief  LED initialize.
 * @param  None
 * @retval None
 */
void BSP_LED_Init(void)
{
    uint8_t i;
    stc_gpio_init_t stcGpioInit;

    /* configuration structure initialization */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    /* Initialize LED pin */

    (void)GPIO_Init(BSP_LED_RED_PORT, BSP_LED_RED_PIN, &stcGpioInit);
    (void)GPIO_Init(BSP_LED_GREEN_PORT, BSP_LED_GREEN_PIN, &stcGpioInit);
    (void)GPIO_Init(BSP_LED_BLUE_PORT, BSP_LED_BLUE_PIN, &stcGpioInit);
}

void BSP_LED_Sw(uint8_t u8Led) // 0x01->red 0x02->green 0x04->blue
{
    if (u8Led & 0x01)
        GPIO_ResetPins(BSP_LED_RED_PORT, BSP_LED_RED_PIN);
    else
        GPIO_SetPins(BSP_LED_RED_PORT, BSP_LED_RED_PIN);

    if (u8Led & 0x02)
        GPIO_ResetPins(BSP_LED_GREEN_PORT, BSP_LED_GREEN_PIN);
    else
        GPIO_SetPins(BSP_LED_GREEN_PORT, BSP_LED_GREEN_PIN);

    if (u8Led & 0x04)
        GPIO_ResetPins(BSP_LED_BLUE_PORT, BSP_LED_BLUE_PIN);
    else
        GPIO_SetPins(BSP_LED_BLUE_PORT, BSP_LED_BLUE_PIN);
}

void BSP_LED_Off(uint8_t u8Led)
{
    if (u8Led & 0x01)
        GPIO_SetPins(BSP_LED_RED_PORT, BSP_LED_RED_PIN);
    if (u8Led & 0x02)
        GPIO_SetPins(BSP_LED_GREEN_PORT, BSP_LED_GREEN_PIN);
    if (u8Led & 0x04)
        GPIO_SetPins(BSP_LED_BLUE_PORT, BSP_LED_BLUE_PIN);
}

void BSP_LED_On(uint8_t u8Led)
{
    if (u8Led & 0x01)
        GPIO_ResetPins(BSP_LED_RED_PORT, BSP_LED_RED_PIN);
    if (u8Led & 0x02)
        GPIO_ResetPins(BSP_LED_GREEN_PORT, BSP_LED_GREEN_PIN);
    if (u8Led & 0x04)
        GPIO_ResetPins(BSP_LED_BLUE_PORT, BSP_LED_BLUE_PIN);
}

void BSP_LED_Toggle(uint8_t u8Led)
{
    if (u8Led & 0x01)
        GPIO_TogglePins(BSP_LED_RED_PORT, BSP_LED_RED_PIN);
    if (u8Led & 0x02)
        GPIO_TogglePins(BSP_LED_GREEN_PORT, BSP_LED_GREEN_PIN);
    if (u8Led & 0x04)
        GPIO_TogglePins(BSP_LED_BLUE_PORT, BSP_LED_BLUE_PIN);
}

#if (LL_PRINT_ENABLE == DDL_ON)
/**
 * @brief  BSP printf device, clock and port pre-initialize.
 * @param  [in] vpDevice                Pointer to print device
 * @param  [in] u32Baudrate             Print device communication baudrate
 * @retval int32_t:
 *           - LL_OK:                   Initialize successfully.
 *           - LL_ERR:                  Initialize unsuccessfully.
 *           - LL_ERR_INVD_PARAM:       The u32Baudrate value is 0.
 */
int32_t BSP_PRINTF_Preinit(void *vpDevice, uint32_t u32Baudrate)
{
    uint32_t u32Div;
    float32_t f32Error;
    stc_usart_uart_init_t stcUartInit;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    (void)vpDevice;

    if (0UL != u32Baudrate)
    {
        /* Set TX port function */
        GPIO_SetFunc(BSP_PRINTF_PORT, BSP_PRINTF_PIN, BSP_PRINTF_PORT_FUNC);

        /* Enable clock  */
        FCG_Fcg1PeriphClockCmd(BSP_PRINTF_DEVICE_FCG, ENABLE);

        /* Configure UART */
        (void)USART_UART_StructInit(&stcUartInit);
        stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
        (void)USART_UART_Init(BSP_PRINTF_DEVICE, &stcUartInit, NULL);

        for (u32Div = 0UL; u32Div <= USART_CLK_DIV64; u32Div++)
        {
            USART_SetClockDiv(BSP_PRINTF_DEVICE, u32Div);
            i32Ret = USART_SetBaudrate(BSP_PRINTF_DEVICE, u32Baudrate, &f32Error);
            if ((LL_OK == i32Ret) &&
                ((-BSP_PRINTF_BAUDRATE_ERR_MAX <= f32Error) && (f32Error <= BSP_PRINTF_BAUDRATE_ERR_MAX)))
            {
                USART_FuncCmd(BSP_PRINTF_DEVICE, USART_TX, ENABLE);
                break;
            }
            else
            {
                i32Ret = LL_ERR;
            }
        }
    }

    return i32Ret;
}
#endif

#define ir_delay_us timer0_delay_us

#define IR_READ_PIN GPIO_ReadInputPins(GPIO_PORT_B, GPIO_PIN_09)

#define IR_USER_ID (0x6891) // 用户码, test on skyworth YK-60JB
#define IR_TIMER (50)       // uint 1 us, ir be sampled clk, suggest 60us~250us
#define IR_SYNC (6000)      // sync: 9ms
#define IR_DATA_0 (750)     // data 0: 1.125ms
#define IR_DATA_1 (1500)    // data 1: 2.25ms
#define IR_REPEAT (64000)   // repeat: 96ms

//
#define IR_ERROR_T (13334 / IR_TIMER) // sync time out: 20ms
#define IR_SYNC_MAX_T ((IR_SYNC + 3334) / IR_TIMER)
#define IR_SYNC_MIN_T ((IR_SYNC - 1000) / IR_TIMER)
//
#define IR_DATA_RPT_T ((IR_DATA_1 + 1334) / IR_TIMER) // if data time > IR_DATA_1 + 2ms, means repeat
#define IR_DATA_HIGH_T (1000 / IR_TIMER)

//
#define IR_BIT_LEN (32) // ir data bit length

//
_Bool b_ir_press_flag = 0;
_Bool b_flag = 0;
uint8_t g_u8_ir_code = 0xff;

uint16_t gethightime()
{
    uint16_t u16_count = 0;
    while (IR_READ_PIN)
    {
        // 进行超时判断，若高电平时间大于4.5ms，即大于引导码时间，则直接退出
        if (u16_count > 120) //(0x20*256) * (12/11059200) s = 8.9ms
        {
            break;
        }
        ir_delay_us(IR_TIMER);
        u16_count++;
    }
    return (u16_count); // 返回IRD引脚持续的高电平时T1计数值
}

uint16_t getlowtime()
{
    uint16_t u16_count = 0;
    while (!IR_READ_PIN) // 读取红外引脚状态
    {
        if (u16_count > 240) // TL1计数满则进位TH1，时间：TH1*256*12/11059200 s = 17.7ms
        {
            break; // 时间大于了NEC协议的引导码载波时间9ms，进行强制退出，避免假等待
        }
        ir_delay_us(IR_TIMER);
        u16_count++;
    }
    return (u16_count); // 返回低电平持续的计数值 每计数一次是一个机器周期的时间即1.085us
}
// 获取红外4个字节数据
uint8_t ir_ext_service(void)
{
    uint16_t time;
    uint8_t i, j;
    uint8_t byte_data = 0;
    uint8_t u8_buf_ir[4];
    time = getlowtime(); // 获取引脚低电平时间

    if ((time < 150) || (time > 200))
    {
        // LOG1("time low err ",time);
        return 0;
    }
    time = gethightime(); // 获取引脚高电平时间

    if ((time < 75) || (time > 100))
    {
        // LOG1("time high err ",time);
        return 0;
    }
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 8; j++)
        {
            time = getlowtime(); // 获取引脚低电平时间

            if ((time < 8) && (time > 15))
            {
                // LOG1("time low err !!!!!",time);
                return 0;
            }

            time = gethightime(); // 获取引脚高电平时间

            if ((time > 0) && (time < 15))
            {
                byte_data >>= 1;
                byte_data |= 0x00;
            }
            else if ((time > 15) && (time < 50))
            {
                byte_data >>= 1;
                byte_data |= 0x80;
            }
            else
            {
                // LOG1("time high err !!!!!",time);
                return 0;
            }
        }
        u8_buf_ir[i] = byte_data;
    }
    if (((u8_buf_ir[2] + u8_buf_ir[3]) == 0xFF) &&
        (u8_buf_ir[0] == (IR_USER_ID >> 8)) &&
        (u8_buf_ir[1] == (IR_USER_ID & 0xff))) // 判断数据有效性
    {
        g_u8_ir_code = u8_buf_ir[2]; // g_u8_ir_code 最终输出数据
        b_ir_press_flag = 0x01;
    }

    return 1;
}

// 输出红外数据
uint8_t mculib_ir_detect(void)
{
    uint8_t u8_key = IR_KEY_OTHER;
    static uint8_t s_u8_rpt_count = 0;

    if (b_ir_press_flag & 0x01)
    {
        b_ir_press_flag = 0;
        u8_key = g_u8_ir_code; // 红外中断更新数据
        s_u8_rpt_count = 0;
    }
    else
    {
        if (s_u8_rpt_count < 8)
        {
            s_u8_rpt_count++;
            if (s_u8_rpt_count == 8)
            {
                g_u8_ir_code = IR_KEY_OTHER;
            }
        }
    }
    return u8_key; // 输出红外数据
}

/**
 * @brief  KEY10(K10) External interrupt Ch.1 callback function
 *         IRQ No.1 in Global IRQ entry No.0~31 is used for EXTINT1
 * @param  None
 * @retval None
 */
static void EXTINT_IR_KEY_IrqCallback(void)
{
    if (SET == EXTINT_GetExtIntStatus(IR_KEY_EXTINT_CH))
    {
        ir_ext_service(); // 获取红外4字节数据
        while (PIN_RESET == GPIO_ReadInputPins(IR_KEY_PORT, IR_KEY_PIN))
        {
            ;
        }
        EXTINT_ClearExtIntStatus(IR_KEY_EXTINT_CH);
    }
}

/******************************************************************
 * @Name    mculib_key_exti_init
 * @brief
 * @param   None
 * @retval
 * @author  Yun Liu
 * @Data    2022-05-20
 ********************************************************************/
void mculib_key_exti_init(void)
{
    stc_extint_init_t stcExtIntInit;
    stc_irq_signin_config_t stcIrqSignConfig;
    stc_gpio_init_t stcGpioInit;

    /* GPIO config */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PullUp = PIN_PU_ON;
    stcGpioInit.u16PinDir = PIN_DIR_IN;
    stcGpioInit.u16PinAttr = PIN_ATTR_DIGITAL;
    stcGpioInit.u16ExtInt = PIN_EXTINT_ON;
    (void)GPIO_Init(IR_KEY_PORT, IR_KEY_PIN, &stcGpioInit);
    /* ExtInt config */
    (void)EXTINT_StructInit(&stcExtIntInit);
    stcExtIntInit.u32Filter = EXTINT_FILTER_ON;
    stcExtIntInit.u32FilterClock = EXTINT_FCLK_DIV8;
    stcExtIntInit.u32Edge = EXTINT_TRIG_FALLING;
    (void)EXTINT_Init(IR_KEY_EXTINT_CH, &stcExtIntInit);

    /* IRQ sign-in */
    stcIrqSignConfig.enIntSrc = IR_KEY_INT_SRC;
    stcIrqSignConfig.enIRQn = IR_KEY_INT_IRQn;
    stcIrqSignConfig.pfnCallback = &EXTINT_IR_KEY_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* NVIC config */
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, IR_KEY_INT_PRIO);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);
}



uint8_t Key_Read(uint8_t mode)
{
    static uint8_t key_up = 1; // 按键松开标志
    if (mode == 1)
        key_up = 1; // 支持连按

    if (key_up && (GPIO_ReadInputPins(GPIO_PORT_B,GPIO_PIN_0X) == 0))
    {
        DDL_DelayMS(10);  
        key_up = 0;
        if (GPIO_ReadInputPins(GPIO_PORT_B,GPIO_PIN_0X) == 0)
            return 1;
    }
    else if (GPIO_ReadInputPins(GPIO_PORT_B,GPIO_PIN_0X) == 1)
        key_up = 1;
    return 0; // 无按键按下
}














// int32_t I2C_Master_Transmit(uint16_t u16DevAddr, uint8_t const au8Data[], uint32_t u32Size, uint32_t u32Timeout)
//{
//    int32_t i32Ret;
//    I2C_Cmd(I2C_UNIT, ENABLE);

//    I2C_SWResetCmd(I2C_UNIT, ENABLE);
//    I2C_SWResetCmd(I2C_UNIT, DISABLE);
//    i32Ret = I2C_Start(I2C_UNIT, u32Timeout);
//    if (LL_OK == i32Ret) {
// #if (I2C_ADDR_MD == I2C_ADDR_MD_10BIT)
//        i32Ret = I2C_Trans10BitAddr(I2C_UNIT, u16DevAddr, I2C_DIR_TX, u32Timeout);
// #else
//        i32Ret = I2C_TransAddr(I2C_UNIT, u16DevAddr, I2C_DIR_TX, u32Timeout);
// #endif

//        if (LL_OK == i32Ret) {
//            i32Ret = I2C_TransData(I2C_UNIT, au8Data, u32Size, u32Timeout);
//        }
//    }

//    (void)I2C_Stop(I2C_UNIT, u32Timeout);
//    I2C_Cmd(I2C_UNIT, DISABLE);

//    return i32Ret;
//}

///**
// * @brief  Master receive data
// *
// * @param  [in] u16DevAddr          The slave address
// * @param  [in] au8Data             The data array
// * @param  [in] u32Size             Data size
// * @param  [in] u32Timeout          Time out count
// * @retval int32_t:
// *            - LL_OK:              Success
// *            - LL_ERR_TIMEOUT:     Time out
// */
// int32_t I2C_Master_Receive(uint16_t u16DevAddr, uint8_t au8Data[], uint32_t u32Size, uint32_t u32Timeout)
//{
//    int32_t i32Ret;

//    I2C_Cmd(I2C_UNIT, ENABLE);
//    I2C_SWResetCmd(I2C_UNIT, ENABLE);
//    I2C_SWResetCmd(I2C_UNIT, DISABLE);
//    i32Ret = I2C_Start(I2C_UNIT, u32Timeout);
//    if (LL_OK == i32Ret)
//     {
//        if (1UL == u32Size)
//        {
//            I2C_AckConfig(I2C_UNIT, I2C_NACK);
//        }

// #if (I2C_ADDR_MD == I2C_ADDR_MD_10BIT)
//         i32Ret = I2C_Trans10BitAddr(I2C_UNIT, u16DevAddr, I2C_DIR_RX, u32Timeout);
// #else
//         i32Ret = I2C_TransAddr(I2C_UNIT, u16DevAddr, I2C_DIR_RX, u32Timeout);
// #endif

//        if (LL_OK == i32Ret)
//        {
//            i32Ret = I2C_MasterReceiveDataAndStop(I2C_UNIT, au8Data, u32Size, u32Timeout);
//        }

//        I2C_AckConfig(I2C_UNIT, I2C_ACK);
//    }

//    if (LL_OK != i32Ret)
//    {
//        (void)I2C_Stop(I2C_UNIT, u32Timeout);
//    }
//    I2C_Cmd(I2C_UNIT, DISABLE);
//    return i32Ret;
//}
// int32_t I2C_Master_Initialize(void)
//{
//    int32_t i32Ret;
//    stc_i2c_init_t stcI2cInit;
//    float32_t fErr;

//    I2C_DeInit(I2C_UNIT);

//    (void)I2C_StructInit(&stcI2cInit);
//    stcI2cInit.u32ClockDiv = I2C_CLK_DIV2;
//    stcI2cInit.u32Baudrate = I2C_BAUDRATE;
//    stcI2cInit.u32SclTime = 3UL;
//    i32Ret = I2C_Init(I2C_UNIT, &stcI2cInit, &fErr);

//    I2C_BusWaitCmd(I2C_UNIT, ENABLE);

//    return i32Ret;
//}

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
