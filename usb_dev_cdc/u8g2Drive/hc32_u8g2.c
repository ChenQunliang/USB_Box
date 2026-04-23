#include "hc32_u8g2.h"
#include "menuConfig.h"
#include "main.h"
// #include "TimerTick.h"

#define HARDWARE_I2C
// #define SOFTWARE_I2C

#define delay_ms(x) Delay_ms(x)
#define delay_us(x) Delay_us(x)

#define SEND_BUFFER_DISPLAY_MS(u8g2, ms) \
  do                                     \
  {                                      \
    u8g2_SendBuffer(u8g2);               \
    delay_ms(ms);                        \
  } while (0);

#define u8 unsigned char
#define MAX_LEN 128
#define OLED_ADDRESS 0x78
#define OLED_CMD 0x00
#define OLED_DATA 0x40

#ifdef HARDWARE_I2C

int32_t I2C_Master_Initialize(void)
{
  int32_t i32Ret;
  stc_i2c_init_t stcI2cInit;
  float32_t fErr;

  I2C_DeInit(I2C_UNIT);

  (void)I2C_StructInit(&stcI2cInit);
  stcI2cInit.u32ClockDiv = I2C_CLK_DIVX;
  stcI2cInit.u32Baudrate = I2C_BAUDRATE;
  stcI2cInit.u32SclTime = 3UL;
  i32Ret = I2C_Init(I2C_UNIT, &stcI2cInit, &fErr);

  I2C_BusWaitCmd(I2C_UNIT, ENABLE);

  return i32Ret;
}
//static int32_t I2C_Master_Transmit(uint16_t u16DevAddr, uint8_t const au8Data[], uint32_t u32Size, uint32_t u32Timeout)
//{
//  int32_t i32Ret;
//  I2C_Cmd(I2C_UNIT, ENABLE);

//  I2C_SWResetCmd(I2C_UNIT, ENABLE);
//  I2C_SWResetCmd(I2C_UNIT, DISABLE);
//  i32Ret = I2C_Start(I2C_UNIT, u32Timeout);
//  if (LL_OK == i32Ret)
//  {
//#if (I2C_ADDR_MD == I2C_ADDR_MD_10BIT)
//    i32Ret = I2C_Trans10BitAddr(I2C_UNIT, u16DevAddr, I2C_DIR_TX, u32Timeout);
//#else
//    i32Ret = I2C_TransAddr(I2C_UNIT, u16DevAddr, I2C_DIR_TX, u32Timeout);
//#endif

//    if (LL_OK == i32Ret)
//    {
//      i32Ret = I2C_TransData(I2C_UNIT, au8Data, u32Size, u32Timeout);
//    }
//  }

//  (void)I2C_Stop(I2C_UNIT, u32Timeout);
//  I2C_Cmd(I2C_UNIT, DISABLE);

//  return i32Ret;
//}

static uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{   
   static int32_t i32Ret;
  uint8_t *data = (uint8_t *)arg_ptr;
  switch (msg)
  {
  case U8X8_MSG_BYTE_SEND:
  {
    if (LL_OK == i32Ret)
    {
        i32Ret = I2C_TransData(I2C_UNIT, data, arg_int, TIMEOUT);
//        printf("IIC_Send State%d\n",i32Ret);
    }
    //    while (arg_int-- > 0)
    //    {
    //      //                I2C_SendData(I2C2, *data++);
    //      I2C_WriteData(I2C_UNIT, *data++);
    //      while (!I2C_WaitStatus(I2C_UNIT, I2C_FLAG_TX_CPLT, SET, TIMEOUT))
    //        continue;
    //    }
  }
  break;
  case U8X8_MSG_BYTE_INIT:
    /* 为启动 i2c 子系统添加自定义代码 */
    {
      
      if (LL_OK != I2C_Master_Initialize())
      {
        printf("IIC_Init Err \n");
      }
//      else 
//        printf("IIC_Init \n");
    }
    break;
  case U8X8_MSG_BYTE_SET_DC:
  { /* ignored for i2c */
//      printf("IIC_Dc \n");
  }
  break;
  case U8X8_MSG_BYTE_START_TRANSFER:
  { 
    
    
    I2C_Cmd(I2C_UNIT, ENABLE);
    I2C_SWResetCmd(I2C_UNIT, ENABLE);
    I2C_SWResetCmd(I2C_UNIT, DISABLE);
    i32Ret = I2C_Start(I2C_UNIT, TIMEOUT);
//    printf("IIC_Start state%d\n",i32Ret);
    if (LL_OK == i32Ret)
    {
      i32Ret = I2C_TransAddr(I2C_UNIT, DEVICE_ADDR , I2C_DIR_TX, TIMEOUT);
//      printf("\nAddr%d\n",i32Ret);
    }
  }
  break;
  case U8X8_MSG_BYTE_END_TRANSFER:
  {
    (void)I2C_Stop(I2C_UNIT, TIMEOUT);
    I2C_Cmd(I2C_UNIT, DISABLE);
//    printf("IIC_End state%d\n" ,i32Ret);  
      i32Ret = 1;
  }
  break;
  default:
    return 0;
  }
  return 1;
}
static uint8_t u8x8_gpio_and_delay_hw(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch (msg)
  {
  case U8X8_MSG_DELAY_100NANO: // delay arg_int * 100 nano seconds
    break;
  case U8X8_MSG_DELAY_10MICRO: // delay arg_int * 10 micro seconds
    break;
  case U8X8_MSG_DELAY_MILLI: // delay arg_int * 1 milli second
    DDL_DelayMS(1);
    break;
  case U8X8_MSG_DELAY_I2C:      // arg_int is the I2C speed in 100KHz, e.g. 4 = 400 KHz
    break;                      // arg_int=1: delay by 5us, arg_int = 4: delay by 1.25us
  case U8X8_MSG_GPIO_I2C_CLOCK: // arg_int=0: Output low at I2C clock pin
    break;                      // arg_int=1: Input dir with pullup high for I2C clock pin
  case U8X8_MSG_GPIO_I2C_DATA:  // arg_int=0: Output low at I2C data pin
    break;                      // arg_int=1: Input dir with pullup high for I2C data pin
  case U8X8_MSG_GPIO_MENU_SELECT:
    u8x8_SetGPIOResult(u8x8, /* get menu select pin state */ 0);
    break;
  case U8X8_MSG_GPIO_MENU_NEXT:
    u8x8_SetGPIOResult(u8x8, /* get menu next pin state */ 0);
    break;
  case U8X8_MSG_GPIO_MENU_PREV:
    u8x8_SetGPIOResult(u8x8, /* get menu prev pin state */ 0);
    break;
  case U8X8_MSG_GPIO_MENU_HOME:
    u8x8_SetGPIOResult(u8x8, /* get menu home pin state */ 0);
    break;
  default:
    u8x8_SetGPIOResult(u8x8, 1); // default return value
    break;
  }
  return 1;
}

static void HardWare_I2C2_GPIOInit(void)
{
  FCG_Fcg1PeriphClockCmd(I2C_FCG_USE, ENABLE);
  LL_PERIPH_WE(EXAMPLE_PERIPH_WE_IIC);
  GPIO_SetFunc(I2C_SCL_PORT, I2C_SCL_PIN, I2C_GPIO_SCL_FUNC);
  GPIO_SetFunc(I2C_SDA_PORT, I2C_SDA_PIN, I2C_GPIO_SDA_FUNC);
  printf("IIC_GPIO_Init \n");
}

#endif

#ifdef SOFTWARE_I2C
static uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch (msg)
  {
  case U8X8_MSG_DELAY_100NANO: // delay arg_int * 100 nano seconds
    __NOP();
    break;
  case U8X8_MSG_DELAY_10MICRO: // delay arg_int * 10 micro seconds
    for (uint16_t n = 0; n < 320; n++)
    {
      __NOP();
    }
    break;
  case U8X8_MSG_DELAY_MILLI: // delay arg_int * 1 milli second
    delay_ms(1);
    break;
  case U8X8_MSG_DELAY_I2C: // arg_int is the I2C speed in 100KHz, e.g. 4 = 400 KHz
    delay_us(5);
    break;                      // arg_int=1: delay by 5us, arg_int = 4: delay by 1.25us
  case U8X8_MSG_GPIO_I2C_CLOCK: // arg_int=0: Output low at I2C clock pin
    if (arg_int == 1)
    {
      GPIO_SetBits(I2C_PORT, I2C_SCL);
    }
    else if (arg_int == 0)
    {
      GPIO_ResetBits(I2C_PORT, I2C_SCL);
    }
    break;                     // arg_int=1: Input dir with pullup high for I2C clock pin
  case U8X8_MSG_GPIO_I2C_DATA: // arg_int=0: Output low at I2C data pin
    if (arg_int == 1)
    {
      GPIO_SetBits(I2C_PORT, I2C_SDA);
    }
    else if (arg_int == 0)
    {
      GPIO_ResetBits(I2C_PORT, I2C_SDA);
    }
    break; // arg_int=1: Input dir with pullup high for I2C data pin
  case U8X8_MSG_GPIO_MENU_SELECT:
    u8x8_SetGPIOResult(u8x8, /* get menu select pin state */ 0);
    break;
  case U8X8_MSG_GPIO_MENU_NEXT:
    u8x8_SetGPIOResult(u8x8, /* get menu next pin state */ 0);
    break;
  case U8X8_MSG_GPIO_MENU_PREV:
    u8x8_SetGPIOResult(u8x8, /* get menu prev pin state */ 0);
    break;
  case U8X8_MSG_GPIO_MENU_HOME:
    u8x8_SetGPIOResult(u8x8, /* get menu home pin state */ 0);
    break;
  default:
    u8x8_SetGPIOResult(u8x8, 1); // default return value
    break;
  }
  return 1;
}
#endif

void u8g2Init(u8g2_t *u8g2)
{
#ifdef SOFTWARE_I2C
  MyI2C_Init();
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_gpio_and_delay);
#endif

#ifdef HARDWARE_I2C
  HardWare_I2C2_GPIOInit();
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2, U8G2_R0, u8x8_byte_hw_i2c, u8x8_gpio_and_delay_hw);
#endif

  u8g2_InitDisplay(u8g2);
  u8g2_SetPowerSave(u8g2, 0);
  u8g2_ClearBuffer(u8g2);
}
