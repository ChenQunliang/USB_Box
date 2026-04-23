#ifndef __ENCODTIMA_H_
#define __ENCODTIMA_H_

#include "main.h"
#include "ev_hc32f460_lqfp100_v2_bsp.h"

#define PHASE_DIFF_CNT_X1 (1U)
#define PHASE_DIFF_CNT_X2 (2U)
#define PHASE_DIFF_CNT_X4 (4U)

/* Select position unit phase-difference count. */

/**
 * TimerA unit definitions for this example.
 *  ------------------|------------------|
 *  | TMRA_POS_UNIT   |     TMRA_Z_UNIT  |
 *  |-----------------|------------------|
 *  |  CM_TMRA_1      |      CM_TMRA_2   |
 *  |-----------------|------------------|
 *  |  CM_TMRA_3      |      CM_TMRA_4   |
 *  |-----------------|------------------|
 *  |  CM_TMRA_5      |      CM_TMRA_6   |
 *  |-----------------|------------------|
 */
#define GPIO_PIN_0X GPIO_PIN_05

#define PHASE_DIFF_CNT (PHASE_DIFF_CNT_X2)
#define TIMA 1
#define B_CLK 1
#define A_CLK 1

#if TIMA
#define TMRA_POS_UNIT (CM_TMRA_5) // CM_TMRA_5
#else
#define TMRA_POS_UNIT (CM_TMRA_3) // CM_TMRA_3
#endif

#if TIMA
#define TMRA_PERIPH_CLK (FCG2_PERIPH_TMRA_5) // FCG2_PERIPH_TMRA_5
#else
#define TMRA_PERIPH_CLK (FCG2_PERIPH_TMRA_3) // FCG2_PERIPH_TMRA_3
#endif

/* Select the pins for phase A and phase B inputting according to 'TMRA_POS_UNIT'. */
#if TIMA //****

#if A_CLK
#define PHASE_A_PORT (GPIO_PORT_A) // GPIO_PORT_A
#define PHASE_A_PIN (GPIO_PIN_02)  // GPIO_PIN_02

#define PHASE_A_PIN_FUNC (GPIO_FUNC_5) // GPIO_FUNC_5
#endif

#if B_CLK
#define PHASE_B_PORT (GPIO_PORT_A) // GPIO_PORT_A
#define PHASE_B_PIN (GPIO_PIN_03)  // GPIO_PIN_03

#define PHASE_B_PIN_FUNC (GPIO_FUNC_5) // GPIO_FUNC_5
#endif

#else //****

#if A_CLK
#define PHASE_A_PORT (GPIO_PORT_A) // GPIO_PORT_A
#define PHASE_A_PIN (GPIO_PIN_06)  // GPIO_PIN_02

#define PHASE_A_PIN_FUNC (GPIO_FUNC_5) // GPIO_FUNC_5
#endif

#if B_CLK
#define PHASE_B_PORT (GPIO_PORT_A) // GPIO_PORT_A
#define PHASE_B_PIN (GPIO_PIN_07)  // GPIO_PIN_03

#define PHASE_B_PIN_FUNC (GPIO_FUNC_5) // GPIO_FUNC_5
#endif

#endif //****

#if (PHASE_DIFF_CNT == PHASE_DIFF_CNT_X1)                                 // TMRA_CNT_UP_COND_CLKB_LOW_CLKA_RISING
#define TMRA_POS_UNIT_CNT_UP_COND (TMRA_CNT_UP_COND_CLKB_LOW_CLKA_RISING) // TMRA_CNT_UP_COND_SYM_UDF
// TMRA_CNT_UP_COND_CLKA_LOW_CLKB_RISING
#define TMRA_POS_UNIT_CNT_DOWN_COND (TMRA_CNT_UP_COND_CLKA_LOW_CLKB_RISING) // TMRA_CNT_DOWN_COND_SYM_UDF

#elif (PHASE_DIFF_CNT == PHASE_DIFF_CNT_X2) // TMRA_CNT_UP_COND_CLKA_HIGH_CLKB_RISING | TMRA_CNT_UP_COND_CLKA_LOW_CLKB_FALLING
#define TMRA_POS_UNIT_CNT_UP_COND (TMRA_CNT_UP_COND_CLKB_HIGH_CLKA_FALLING | TMRA_CNT_UP_COND_CLKA_LOW_CLKB_FALLING)
// TMRA_CNT_DOWN_COND_CLKB_HIGH_CLKA_RISING | TMRA_CNT_DOWN_COND_CLKB_LOW_CLKA_FALLING
#define TMRA_POS_UNIT_CNT_DOWN_COND (TMRA_CNT_DOWN_COND_CLKA_HIGH_CLKB_FALLING | TMRA_CNT_DOWN_COND_CLKA_LOW_CLKB_RISING)
#elif (PHASE_DIFF_CNT == PHASE_DIFF_CNT_X4)
#define TMRA_POS_UNIT_CNT_UP_COND (TMRA_CNT_UP_COND_CLKB_HIGH_CLKA_FALLING | TMRA_CNT_UP_COND_CLKA_LOW_CLKB_FALLING | \
                                   TMRA_CNT_UP_COND_CLKB_LOW_CLKA_RISING | TMRA_CNT_UP_COND_CLKA_HIGH_CLKB_RISING)

#define TMRA_POS_UNIT_CNT_DOWN_COND (TMRA_CNT_DOWN_COND_CLKA_HIGH_CLKB_FALLING | TMRA_CNT_DOWN_COND_CLKB_LOW_CLKA_FALLING | \
                                     TMRA_CNT_DOWN_COND_CLKA_LOW_CLKB_RISING | TMRA_CNT_DOWN_COND_CLKB_HIGH_CLKA_RISING)
#else
#error "Phase-difference count is NOT supported!!!"
#endif

void TmrAConfig(void);

#endif
