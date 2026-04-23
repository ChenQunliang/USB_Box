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

volatile unsigned long g_SystemTicks = 0;
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

#if (LL_TMR0_ENABLE == DDL_ON)
/**
 * @brief  TMR0 compare interrupt callback function.
 * @param  None
 * @retval None
 */

void TMR02_B_Config(void)
{
    stc_tmr0_init_t stcTmr0Init;
    stc_irq_signin_config_t stcIrqSignConfig;

    /* Enable timer0 and AOS clock */
    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_2, ENABLE);
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

    /* TIMER0 configuration */
    (void)TMR0_StructInit(&stcTmr0Init);
    stcTmr0Init.u32ClockSrc = TMR0_CLK_SRC_INTERN_CLK; // ʱדԴӉԃŚҿ֍ֱ̙մǷ
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

static uint32_t timer0_CHB_count = 0;
void TMR0_CHB_CompareIrqCallback(void) // 10US
{
    static uint32_t u32TmrCnt = 0U;
    if (timer0_CHB_count % 100 == 0) // 1ms
    {
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

        /*led*/
        if ((led_sw & 0x01) && (led_sw & LED_ERR_RED))
        {
            g_u16_led_timer++;
            BSP_LED_Sw(0x01);
            if (g_u16_led_timer >= LED_TIME)
            {
                g_u16_led_timer = 0;
                led_sw = 0;
            }
        }
        else if ((led_sw & 0x01) && (led_sw & LED_ERR_GREEN))
        {
            g_u16_led_timer++;
            BSP_LED_Sw(0x02);
            if (g_u16_led_timer >= LED_TIME)
            {
                g_u16_led_timer = 0;
                led_sw = 0;
            }
        }
        else if ((led_sw & 0x01) && (led_sw & LED_ERR_BLUE))
        {
            g_u16_led_timer++;
            BSP_LED_Sw(0x04);
            if (g_u16_led_timer >= LED_TIME)
            {
                g_u16_led_timer = 0;
                led_sw = 0;
            }
        }
        else if ((led_sw & 0x01) && (led_sw & LED_OK))
        {
            g_u16_led_timer++;
            BSP_LED_Sw(0x01 | 0x02 | 0x04);
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
            BSP_LED_Sw(led_state); // led_state
    }
    timer0_CHB_count++;

    TMR0_ClearStatus(CM_TMR0_2, TMR0_FLAG_CMP_B);
}

void timer0_delay_us(uint32_t us)
{
    if (us > 300)
    {
        us = 300;
        printf("%s %d param us is overrun \r\n", __FILE__, __LINE__);
    }
    TMR0_SetCountValue(CM_TMR0_2, TMR0_CH_A, 0);
    TMR0_SetCompareValue(CM_TMR0_2, TMR0_CH_A, us * 97);
    TMR0_Start(CM_TMR0_2, TMR0_CHA);
    while (!TMR0_GetStatus(CM_TMR0_2, TMR0_FLAG_CMP_A))
        ;
    TMR0_ClearStatus(CM_TMR0_2, TMR0_FLAG_CMP_A);
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
    stc_clock_xtal_init_t stcXtalInit;
    stc_clock_pll_init_t stcMpllInit;

    GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_IN_PIN | BSP_XTAL_OUT_PIN, ENABLE);
    (void)CLK_XtalStructInit(&stcXtalInit);
    (void)CLK_PLLStructInit(&stcMpllInit);

    /* Set bus clk div. */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL, (CLK_HCLK_DIV1 | CLK_EXCLK_DIV2 | CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 |
                                      CLK_PCLK2_DIV4 | CLK_PCLK3_DIV4 | CLK_PCLK4_DIV2));

    /* Config Xtal and enable Xtal */
    stcXtalInit.u8Mode = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv = CLK_XTAL_DRV_ULOW;
    stcXtalInit.u8State = CLK_XTAL_ON;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    (void)CLK_XtalInit(&stcXtalInit);

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
        ;
    }

    /* sram init include read/write wait cycle setting */
    SRAM_SetWaitCycle(SRAM_SRAMH, SRAM_WAIT_CYCLE0, SRAM_WAIT_CYCLE0);
    SRAM_SetWaitCycle((SRAM_SRAM12 | SRAM_SRAM3 | SRAM_SRAMR), SRAM_WAIT_CYCLE1, SRAM_WAIT_CYCLE1);

    /* flash read wait cycle setting */
    (void)EFM_SetWaitCycle(EFM_WAIT_CYCLE5);
    /* 3 cycles for 126MHz ~ 200MHz */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT3);
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

uint8_t Key_Read(uint8_t mode)
{
    static uint8_t key_up = 1; // �����ɿ���־
    if (mode == 1)
        key_up = 1; // ֧������

    if (key_up && (GPIO_ReadInputPins(GPIO_PORT_B, GPIO_PIN_0X) == 0))
    {
        DDL_DelayMS(10);
        key_up = 0;
        if (GPIO_ReadInputPins(GPIO_PORT_B, GPIO_PIN_0X) == 0)
            return 1;
    }
    else if (GPIO_ReadInputPins(GPIO_PORT_B, GPIO_PIN_0X) == 1)
        key_up = 1;
    return 0; // �ް�������
}

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
