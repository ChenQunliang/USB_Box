#include "main.h"
#include "stdlib.h"

extern uint16_t g_u16_sys_timer;
extern uint16_t g_u16_key_timer;
extern uint16_t g_u16_mis_timer;
extern uint16_t g_u16_osd_timer;
__IO uint8_t m_u8SpeedUpd = 0U;
// const uint8_t usFlashInitVal[4] __attribute__((at(0x00007FFC))) = {0x23,0x01,0x89,0x67};//定位在flash中
// 除0错误
static void fault_test_by_div0(void)
{
    volatile int *SCB_CCR = (volatile int *)0xE000ED14; // SCB->CCR
    int x, y, z;

    *SCB_CCR |= (1 << 4); /* bit4: DIV_0_TRP. */

    x = 10;
    y = 0;
    z = x / y;
    printf("z:%d\n", z);
}
xMenu menu;
int test;
int32_t main(void)
{
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);
    __enable_irq();
    /*Flash */
    EFM_FWMC_Cmd(ENABLE);
    BSP_CLK_Init();
    BSP_LED_Init();
//    USER_BUTTON_Init();
//    USER_SWITCHS_Init();
#if (LL_TMR0_ENABLE == DDL_ON)
    TMR02_A_Config();
    TMR02_B_Config();
    TMR01_A_Config();
#endif
    mculib_key_exti_init();
#if (LL_PRINT_ENABLE == DDL_ON)
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
#endif
#ifdef DEMO
    /*backtrace 库初始化*/
    cm_backtrace_init("usb_dev_cdc", HARDWARE_VERSION, SOFTWARE_VERSION);
#endif
#if (LL_PRINT_ENABLE == DDL_ON)
#endif
    TMR0_Start(CM_TMR0_2, TMR0_CHB);

    TmrAConfig();
    TMRA_Start(TMRA_POS_UNIT);
    Menu_Init(&menu);
    
    for (;;)
    {
        

        
//        mculib_i2c_write_16bidx8bval(0xB2, 0x785, 0x02); //luman peaking   02
//        mculib_i2c_write_16bidx8bval(0xB2, 0x0264, 0x03); //03
        if (m_u8SpeedUpd) // 
        {   
            Menu_Loop(&menu);
            m_u8SpeedUpd = 0;
//            test = rand()%360;
        }
        else
        {   
            if (g_u16_sys_timer >= SYS_TIMEOUT_100MS)        // 100ms
            {
                g_u16_sys_timer = 0;
            }
            if (g_u16_key_timer >= SYS_TIMEOUT_50MS) // 50MS
            {   
                g_u16_key_timer = 0;
            }
            if (g_u16_mis_timer >= SYS_TIMEOUT_100MS) // 100ms
            {
                g_u16_mis_timer = 0;
            }
            if (g_u16_osd_timer >= SYS_TIMEOUT_500MS) // 500MS   OSD
            {
                
                g_u16_osd_timer = 0;
            }
        }

    }
}
