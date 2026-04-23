#include "EncodTimA.h"

void TmrAConfig(void)
{
    stc_tmra_init_t stcTmraInit;

    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_SRAM);
    /* 1. Configures the function of phase A, phase B and phase Z. */
#if A_CLK
    GPIO_SetFunc(PHASE_A_PORT, PHASE_A_PIN, PHASE_A_PIN_FUNC);
#endif
#if B_CLK
    GPIO_SetFunc(PHASE_B_PORT, PHASE_B_PIN, PHASE_B_PIN_FUNC);
#endif

    /* 2. Enable TimerA peripheral clock. */
    FCG_Fcg2PeriphClockCmd(TMRA_PERIPH_CLK, ENABLE);

    (void)TMRA_StructInit(&stcTmraInit);
    /* 3. Initializes position-count unit. */
    stcTmraInit.u8CountSrc = TMRA_CNT_SRC_HW;
    stcTmraInit.hw_count.u16CountUpCond = TMRA_POS_UNIT_CNT_UP_COND;
    stcTmraInit.hw_count.u16CountDownCond = TMRA_POS_UNIT_CNT_DOWN_COND;

    (void)TMRA_Init(TMRA_POS_UNIT, &stcTmraInit);
    // TMRA_CLR_COND_SYM_TRIG_RISING
    /* 3.1. Enable The TMRA_POS_UNIT counter reset when phase Z rising edge */
    // ENABLE
    TMRA_HWClearCondCmd(TMRA_POS_UNIT, TMRA_CLR_COND_SYM_TRIG_RISING, ENABLE); // Çå³ýÌõ¼þ

    /* 5. Enable Filter if needed. */
#if A_CLK
    TMRA_SetFilterClockDiv(TMRA_POS_UNIT, TMRA_PIN_CLKA, TMRA_FILTER_CLK_DIV64);
    TMRA_FilterCmd(TMRA_POS_UNIT, TMRA_PIN_CLKA, ENABLE);
#endif

#if B_CLK // 1 4 16 64
    TMRA_SetFilterClockDiv(TMRA_POS_UNIT, TMRA_PIN_CLKB, TMRA_FILTER_CLK_DIV64);
    TMRA_FilterCmd(TMRA_POS_UNIT, TMRA_PIN_CLKB, ENABLE);
#endif

    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_SRAM);
}