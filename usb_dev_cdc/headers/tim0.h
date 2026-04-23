#ifndef __TIM0_H_
#define __TIM0_H_
#include "main.h"
#include "ev_hc32f460_lqfp100_v2_bsp.h"


#define QE_CYCLE_PER_ROUND              (PHASE_DIFF_CNT)
/* when phase Z rising edge the TMRA_Z_UNIT Counter plus one */
//#define TMRA_Z_UNIT_CNT_UP_COND         (TMRA_CNT_UP_COND_TRIG_RISING)

/* Timer0 for this example */
#define TMR0_UNIT                       (CM_TMR0_1)
#define TMR0_CH                         (TMR0_CH_B)
#define TMR0_PERIPH_CLK                 (FCG2_PERIPH_TMR0_1)
#define TMR0_INT_TYPE                   (TMR0_INT_CMP_B)
#define TMR0_INT_PRIO                   (DDL_IRQ_PRIO_03)
#define TMR0_INT_SRC                    (INT_SRC_TMR0_1_CMP_B)
#define TMR0_INT_IRQn                   (INT044_IRQn)
#define TMR0_INT_FLAG                   (TMR0_FLAG_CMP_B)

#define TMR0_CMP_VAL                    (80000U/64U)
#define TMR0_CLK_DIV                    (TMRA_CLK_DIV64)

/* Period = 1 / (8000000 / 4) * (1024 ) = 500ms    */
//extern __IO  uint8_t m_u8SpeedUpd ;


void Tmr0Config();
 void TMR0_IrqCallback(void);





#endif