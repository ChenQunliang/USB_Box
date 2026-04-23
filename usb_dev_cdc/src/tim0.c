#include "tim0.h"


void Tmr0Config()
{
    stc_tmr0_init_t stcTmr0Init;
    stc_irq_signin_config_t stcIrq;

    /* 1. Enable Timer0 peripheral clock. */
    FCG_Fcg2PeriphClockCmd(TMR0_PERIPH_CLK, ENABLE);

    /* 2. Set a default initialization value for stcTmr0Init. */
    (void)TMR0_StructInit(&stcTmr0Init);

    /* 3. Modifies the initialization values depends on the application. */
    stcTmr0Init.u32ClockSrc = TMR0_CLK_SRC_INTERN_CLK;
    stcTmr0Init.u32ClockDiv = TMR0_CLK_DIV;
    stcTmr0Init.u32Func = TMR0_FUNC_CMP;
    stcTmr0Init.u16CompareValue = (uint16_t)TMR0_CMP_VAL;
    (void)TMR0_Init(TMR0_UNIT, TMR0_CH, &stcTmr0Init);

    /* 4. Configures IRQ if needed. */
    stcIrq.enIntSrc = TMR0_INT_SRC;
    stcIrq.enIRQn = TMR0_INT_IRQn;
    stcIrq.pfnCallback = &TMR0_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrq);
    NVIC_ClearPendingIRQ(stcIrq.enIRQn);
    NVIC_SetPriority(stcIrq.enIRQn, TMR0_INT_PRIO);
    NVIC_EnableIRQ(stcIrq.enIRQn);
    /* Enable the specified interrupts of Timer0. */
    TMR0_IntCmd(TMR0_UNIT, TMR0_INT_TYPE, ENABLE);
}

/**
 * @brief  Timer0 interrupt callback function.
 * @param  None
 * @retval None
 */
void TMR0_IrqCallback(void) // 100ms
{
    //    static uint32_t u32TmrCnt = 0U;
    //    lv_tick_inc(1);   //1ms
    //    u32TmrCnt++;
    //    /* 100ms * 10 = 1s */
    //    if (u32TmrCnt >= 5U)
    //    {
    //        m_u8SpeedUpd = 1U;
    //        u32TmrCnt    = 0U;
    //    }
    TMR0_ClearStatus(TMR0_UNIT, TMR0_INT_FLAG); // 软件清除标志
}
