#include "main.h"
#include "stdlib.h"
#include "tv5725.h"
#include "preset_deinterlacer.h"
#include "preset_md_section.h"
#include "preset_480p.h"
#include "preset_720p.h"
#include "preset_1080p.h"

extern uint16_t g_u16_sys_timer;
extern uint16_t g_u16_key_timer;
extern uint16_t g_u16_mis_timer;
extern uint16_t g_u16_osd_timer;
__IO uint8_t m_u8SpeedUpd = 0U;
// const uint8_t usFlashInitVal[4] __attribute__((at(0x00007FFC))) = {0x23,0x01,0x89,0x67};//��λ��flash��
// ��0����
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
#if (LL_TMR0_ENABLE == DDL_ON)
    TMR02_B_Config();
#endif
#if (LL_PRINT_ENABLE == DDL_ON)
    DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
#endif
#ifdef DEMO
    /*backtrace ���ʼ��*/
    cm_backtrace_init("usb_dev_cdc", HARDWARE_VERSION, SOFTWARE_VERSION);
#endif
#if (LL_PRINT_ENABLE == DDL_ON)
#endif
    TMR0_Start(CM_TMR0_2, TMR0_CHB);

    TmrAConfig();
    TMRA_Start(TMRA_POS_UNIT);

    LL_PERIPH_WE(LL_PERIPH_GPIO);
    V_I2C_Init();
    if (tv5725_init() == LL_OK)
    {
        tv5725_asw_init();
        tv5725_input_set_mode(TV5725_INPUT_AUTO);
        tv5725_output_path_init(preset_480p, 0); /* 480p preset, RGB input */
    }

    Menu_Init(&menu);

    for (;;)
    {
        if (m_u8SpeedUpd) //
        {
            Menu_Loop(&menu);
            m_u8SpeedUpd = 0;
            //            test = rand()%360;
        }
        else
        {
            if (g_u16_sys_timer >= SYS_TIMEOUT_100MS) // 100ms
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
