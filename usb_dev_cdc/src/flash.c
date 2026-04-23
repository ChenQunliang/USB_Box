/**
 *******************************************************************************
 * @file  iap/iap_ymodem_boot/source/flash.c
 * @brief This file provides firmware functions to manage the Flash driver.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2022-03-31       CDT             First version
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2023, Xiaohua Semiconductor Co., Ltd. All rights reserved.
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
#include "flash.h"
#include "main.h"
#include "menuConfig.h"
/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
__ALIGN_BEGIN uint8_t u8_buf[256];
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  检查地址是否对齐。
 * @param  u32Addr                      Flash address
 * @retval An en_result_t enumeration value:
 *           - LL_OK: Address aligned
 *           - LL_ERR: Address unaligned
 */
int32_t FLASH_CheckAddrAlign(uint32_t u32Addr)
{
  uint32_t u32Step = FLASH_SECTOR_SIZE;

  if (VECT_TAB_STEP > FLASH_SECTOR_SIZE)
  {
    u32Step = VECT_TAB_STEP;
  }
  if ((u32Addr % u32Step) != 0UL)
  {
    return LL_ERR;
  }

  return LL_OK;
}

/**
 * @brief  擦除闪存扇区.
 * @param  u32Addr                      闪存地址
 * @param  u32Size                      固件大小（0：当前地址扇区）
 * @retval An en_result_t enumeration value:
 *           - LL_OK: Erase succeeded
 *           - LL_ERR: Erase timeout
 *           - LL_ERR_INVD_PARAM: The parameters is invalid.
 */
int32_t FLASH_EraseSector(uint32_t u32Addr, uint32_t u32Size)
{
  uint32_t i;
  uint32_t u32PageNum;

  if (u32Addr >= (FLASH_BASE + FLASH_SIZE)) // 判断是否越界
  {
    return LL_ERR_INVD_PARAM;
  }

  if (u32Size == 0U) // 长度为0 则擦除该地址所在扇区
  {
    // 进入这里
    return EFM_SectorErase(u32Addr);
  }
  else
  {
    u32PageNum = u32Size / FLASH_SECTOR_SIZE;
    if ((u32Size % FLASH_SECTOR_SIZE) != 0UL)
    {
      u32PageNum += 1U;
    }
    for (i = 0; i < u32PageNum; i++)
    {
      if (LL_OK != EFM_SectorErase(u32Addr + (i * FLASH_SECTOR_SIZE)))
      {
        return LL_ERR;
      }
    }
  }

  return LL_OK;
}

/**
 * @brief  Write data to flash.
 * @param  u32Addr                      Flash address
 * @param  pu8Buff                      Pointer to the buffer to be written
 * @param  u32Len                       Buffer length
 * @retval int32_t:
 *           - LL_OK: Program successful.
 *           - LL_ERR_INVD_PARAM: The parameters is invalid.
 *           - LL_ERR_NOT_RDY: EFM if not ready.
 *           - LL_ERR_ADDR_ALIGN: Address alignment error
 */
int32_t FLASH_WriteData(uint32_t u32Addr, uint8_t *pu8Buff, uint32_t u32Len)
{
  // __disable_irq;
  if ((pu8Buff == NULL) || (u32Len == 0U) || ((u32Addr + u32Len) > (FLASH_BASE + FLASH_SIZE)))
  {
    return LL_ERR_INVD_PARAM;
  }
  if (0UL != (u32Addr % 4U))
  {
    return LL_ERR_ADDR_ALIGN;
  }
  // __enable_irq;
  return EFM_Program(u32Addr, pu8Buff, u32Len);
}

/**
 * @brief  Read data from flash.
 * @param  u32Addr                      Flash address
 * @param  pu8Buff                      Pointer to the buffer to be reading
 * @param  u32Len                       Buffer length
 * @retval int32_t:
 *           - LL_OK: Read data succeeded
 *           - LL_ERR_INVD_PARAM: The parameters is invalid
 *           - LL_ERR_ADDR_ALIGN: Address alignment error
 */
int32_t FLASH_ReadData(uint32_t u32Addr, uint8_t *pu8Buff, uint32_t u32Len)
{
  // __disable_irq;
  uint32_t i;
  uint32_t u32WordLength, u8ByteRemain;
  uint32_t *pu32ReadBuff;
  __IO uint32_t *pu32FlashAddr;
  uint8_t *pu8Byte;
  __IO uint8_t *pu8FlashAddr;

  if ((pu8Buff == NULL) || (u32Len == 0U) || ((u32Addr + u32Len) > (FLASH_BASE + FLASH_SIZE)))
  {
    return LL_ERR_INVD_PARAM;
  }
  if (0UL != (u32Addr % 4U))
  {
    return LL_ERR_ADDR_ALIGN;
  }

  pu32ReadBuff = (uint32_t *)(uint32_t)pu8Buff;
  pu32FlashAddr = (uint32_t *)u32Addr;
  u32WordLength = u32Len / 4U;
  u8ByteRemain = u32Len % 4U;
  /* Read data */
  for (i = 0UL; i < u32WordLength; i++)
  {
    *(pu32ReadBuff++) = *(pu32FlashAddr++);
  }
  if (0UL != u8ByteRemain)
  {
    pu8Byte = (uint8_t *)pu32ReadBuff;
    pu8FlashAddr = (uint8_t *)pu32FlashAddr;
    for (i = 0UL; i < u8ByteRemain; i++)
    {
      *(pu8Byte++) = *(pu8FlashAddr++);
    }
  }
  // __enable_irq;
  return LL_OK;
}

static void common_parameter(uint8_t num)
{
  g_u8_display_brightness = u8_buf[num * 32 + 1];
  g_u8_display_contrast = u8_buf[num * 32 + 2];
  g_u8_display_saturation = u8_buf[num * 32 + 3];
  g_u8_display_hues = u8_buf[num * 32 + 4];
  g_u8_display_rgbsfilter = u8_buf[num * 32 + 5];
  g_u8_output_index = u8_buf[num * 32 + 6]; // = 1;  // 1=1080p // 0=720p
  g_u8_OutputPort = u8_buf[num * 32 + 7];   //  = OUT_MODE_HDMI;
  if (u8_buf[num * 32 + 11] == (0xfe))      // 负数
    adjust_value_up_down = (0 - u8_buf[num * 32 + 8]);
  else // fd
    adjust_value_up_down = u8_buf[num * 32 + 8];
  if (u8_buf[num * 32 + 12] == (0xfe)) // 负数
    adjust_value_left_right = (0 - u8_buf[num * 32 + 9]);
  else // fd
    adjust_value_left_right = u8_buf[num * 32 + 9];
  key_switch_4x3_bak = u8_buf[num * 32 + 10];
}

// 默认数据
void user_settings_init(void)
{

  // 默认数值标识
  u8_buf[13] = 'D';
  u8_buf[14] = 'E';
  u8_buf[15] = 'F';

  u8_buf[0] = 0x01;
  // SV
  u8_buf[1] = 56 + 2; // g_u8_display_brightness;
  u8_buf[2] = 37;     // g_u8_display_contrast;
  u8_buf[3] = 40;     // g_u8_display_saturation;
  u8_buf[4] = 52;     // g_u8_display_hues;
  u8_buf[5] = 0xCC;   // g_u8_display_rgbsfilter;
  u8_buf[6] = 1;      // g_u8_output_index; // = 1;  // 1=1080p // 0=720p
  u8_buf[7] = 1;      // g_u8_OutputPort;   //  = OUT_MODE_HDMI;
  u8_buf[8] = 0;      // adjust_value_up_down;
  u8_buf[9] = 0;      // adjust_value_left_right;
  u8_buf[10] = 0;     // change the aspect radio 0 -> 4:3   1-> 16:9
  u8_buf[11] = 0xfd;  // adjust_value_up_down正负
  u8_buf[12] = 0xfd;  // adjust_value_left_right正负
  u8_buf[16] = MS1824_NTSC_358;

  // CVBS
  u8_buf[1 * 32 + 1] = 56;            // g_u8_display_brightness;
  u8_buf[1 * 32 + 2] = 38;            // g_u8_display_contrast;
  u8_buf[1 * 32 + 3] = 37;            // g_u8_display_saturation;
  u8_buf[1 * 32 + 4] = 51;            // g_u8_display_hues;
  u8_buf[1 * 32 + 5] = 0xCC;          // g_u8_display_rgbsfilter;
  u8_buf[1 * 32 + 6] = 1;             // g_u8_output_index; // = 1;  // 1=1080p // 0=720p
  u8_buf[1 * 32 + 7] = OUT_MODE_HDMI; // g_u8_OutputPort;   //  = OUT_MODE_HDMI;
  u8_buf[1 * 32 + 8] = 0;             // adjust_value_up_down;
  u8_buf[1 * 32 + 9] = 0;             // adjust_value_left_right;
  u8_buf[1 * 32 + 10] = 0;            // change the aspect radio 0 -> 4:3   1-> 16:9
  u8_buf[1 * 32 + 11] = 0xfd;         // adjust_value_up_down正负
  u8_buf[1 * 32 + 12] = 0xfd;         // adjust_value_left_right正负
  u8_buf[1 * 32 + 16] = MS1824_NTSC_358;

  // YUV
  u8_buf[2 * 32 + 1] = 0x32;          // g_u8_display_brightness;0x32
  u8_buf[2 * 32 + 2] = 0x4A;          // g_u8_display_contrast;
  u8_buf[2 * 32 + 3] = 0x4B;          // g_u8_display_saturation;
  u8_buf[2 * 32 + 4] = 0x32;          // g_u8_display_hues;
  u8_buf[2 * 32 + 5] = 0xCC;          // g_u8_display_rgbsfilter;
  u8_buf[2 * 32 + 6] = 1;             // g_u8_output_index; // = 1;  // 1=1080p // 0=720p
  u8_buf[2 * 32 + 7] = OUT_MODE_HDMI; // g_u8_OutputPort;   //  = OUT_MODE_HDMI;
  u8_buf[2 * 32 + 8] = 0;             // adjust_value_up_down;
  u8_buf[2 * 32 + 9] = 0;             // adjust_value_left_right;
  u8_buf[2 * 32 + 10] = 1;            // change the aspect radio 0 -> 4:3   1-> 16:9
  u8_buf[2 * 32 + 11] = 0xfd;         // adjust_value_up_down正负
  u8_buf[2 * 32 + 12] = 0xfd;         // adjust_value_left_right正负
                                      //    u8_buf[2 * 32 + 16]     =MS1824_NTSC_358;

  // VGA
  u8_buf[3 * 32 + 1] = 0x32;          // g_u8_display_brightness;
  u8_buf[3 * 32 + 2] = 0x32;          // g_u8_display_contrast;
  u8_buf[3 * 32 + 3] = 0x32;          // g_u8_display_saturation;
  u8_buf[3 * 32 + 4] = 0x32;          // g_u8_display_hues;
  u8_buf[3 * 32 + 5] = 0xCC;          // g_u8_display_rgbsfilter;
  u8_buf[3 * 32 + 6] = 1;             // g_u8_output_index; // = 1;  // 1=1080p // 0=720p
  u8_buf[3 * 32 + 7] = OUT_MODE_HDMI; // g_u8_OutputPort;   //  = OUT_MODE_HDMI;
  u8_buf[3 * 32 + 8] = 0;             // adjust_value_up_down;
  u8_buf[3 * 32 + 9] = 0;             // adjust_value_left_right;
  u8_buf[3 * 32 + 10] = 1;            // change the aspect radio 0 -> 4:3   1-> 16:9
  u8_buf[3 * 32 + 11] = 0xfd;         // adjust_value_up_down正负
  u8_buf[3 * 32 + 12] = 0xfd;         // adjust_value_left_right正负
                                      //    u8_buf[3 * 32 + 16]     =MS1824_NTSC_358;

  // RGBs
  u8_buf[4 * 32 + 0] = 0x00; // rgbs_dir;

  u8_buf[4 * 32 + 1] = 0x3C;          // g_u8_display_brightness;
  u8_buf[4 * 32 + 2] = 0x3C;          // g_u8_display_contrast;
  u8_buf[4 * 32 + 3] = 0x4B;          // g_u8_display_saturation;
  u8_buf[4 * 32 + 4] = 0x32;          // g_u8_display_hues;
  u8_buf[4 * 32 + 5] = 0xCC;          // g_u8_display_rgbsfilter;
  u8_buf[4 * 32 + 6] = 1;             // g_u8_output_index; // = 1;  // 1=1080p // 0=720p
  u8_buf[4 * 32 + 7] = OUT_MODE_HDMI; // g_u8_OutputPort;   //  = OUT_MODE_HDMI;
  u8_buf[4 * 32 + 8] = 0;             // adjust_value_up_down;
  u8_buf[4 * 32 + 9] = 0;             // adjust_value_left_right;
  u8_buf[4 * 32 + 10] = 1;            // change the aspect radio 0 -> 4:3   1-> 16:9
  u8_buf[4 * 32 + 11] = 0xfd;         // adjust_value_up_down正负
  u8_buf[4 * 32 + 12] = 0xfd;         // adjust_value_left_right正负
                                      //    u8_buf[4 * 32 + 16]     =MS1824_NTSC_358;

  // DIGITAL
  u8_buf[5 * 32 + 1] = 0x32;          // g_u8_display_brightness;
  u8_buf[5 * 32 + 2] = 0x32;          // g_u8_display_contrast;
  u8_buf[5 * 32 + 3] = 0x32;          // g_u8_display_saturation;
  u8_buf[5 * 32 + 4] = 0x32;          // g_u8_display_hues;
  u8_buf[5 * 32 + 5] = 0xCC;          // g_u8_display_rgbsfilter;
  u8_buf[5 * 32 + 6] = 1;             // g_u8_output_index; // = 1;  // 1=1080p // 0=720p
  u8_buf[5 * 32 + 7] = OUT_MODE_HDMI; // g_u8_OutputPort;   //  = OUT_MODE_HDMI;
  u8_buf[5 * 32 + 8] = 0;             // adjust_value_up_down;
  u8_buf[5 * 32 + 9] = 0;             // adjust_value_left_right;
  u8_buf[5 * 32 + 10] = 1;            // change the aspect radio 0 -> 4:3   1-> 16:9
  u8_buf[5 * 32 + 11] = 0xfd;         // adjust_value_up_down正负
  u8_buf[5 * 32 + 12] = 0xfd;         // adjust_value_left_right正负
                                      //    u8_buf[5 * 32 + 16]     =MS1824_NTSC_358;

  // NOR
  u8_buf[6 * 32 + 1] = 0x32;          // g_u8_display_brightness;
  u8_buf[6 * 32 + 2] = 0x32;          // g_u8_display_contrast;
  u8_buf[6 * 32 + 3] = 0x32;          // g_u8_display_saturation;
  u8_buf[6 * 32 + 4] = 0x32;          // g_u8_display_hues;
  u8_buf[6 * 32 + 5] = 0xCC;          // g_u8_display_rgbsfilter;
  u8_buf[6 * 32 + 6] = 1;             // g_u8_output_index; // = 1;  // 1=1080p // 0=720p
  u8_buf[6 * 32 + 7] = OUT_MODE_HDMI; // g_u8_OutputPort;   //  = OUT_MODE_HDMI;
  u8_buf[6 * 32 + 8] = 0;             // adjust_value_up_down;
  u8_buf[6 * 32 + 9] = 0;             // adjust_value_left_right;
  u8_buf[6 * 32 + 10] = 1;            // change the aspect radio 0 -> 4:3   1-> 16:9
  u8_buf[6 * 32 + 11] = 0xfd;         // adjust_value_up_down正负
  u8_buf[6 * 32 + 12] = 0xfd;         // adjust_value_left_right正负
  //    u8_buf[6 * 32 + 16]     =MS1824_NTSC_358;
}

// 检查当前模式是否存在用户数据
static uint8_t checks_video_data(UINT8 comm)
{
  if (
      u8_buf[13] == 'S' &&
      u8_buf[14] == 'E')
  {
    switch (comm)
    {
    case IN_MODE_SV:
    {
      LOG("USER_SV\n");
      return u8_buf[15] & S_Video;
    }
    break;
    case IN_MODE_CVBS:
    {
      LOG("USER_CVBS\n");
      return u8_buf[15] & CVBS;
    }
    break;
    case IN_MODE_YUV:
    {
      LOG("USER_YUV\n");
      return u8_buf[15] & YPBPR;
    }
    break;
    case IN_MODE_VGA:
    {
      LOG("USER_VGA\n");
      return u8_buf[15] & VGA;
    }
    break;
    case IN_MODE_RGBs:
    {
      if (rgbs_dir == 0)
      {
        LOG("USER_RGBs1\n");
        return u8_buf[15] & RGBs1;
      }
      else if (rgbs_dir == 1)
      {
        LOG("USER_RGBs2\n");
        return u8_buf[15] & RGBs2;
      }
    }
    break;
    default:
      break;
    }
  }
  LOG("check-empty data\n");
  return 0;
}

void my_home_read(UINT8 com)
{
  uint8_t num = 0;
  memset((uint8_t *)u8_buf, 0, sizeof(u8_buf));
  __disable_irq();
  FLASH_ReadData(FLASH_LEAF_ADDR(USER), (uint8_t *)u8_buf, 256); // 读取用户保存数据用于显示
  if (checks_video_data(com))
  {
    LOG("myUSER_DATA\n");
    led_state |= LED_GREEN;
    led_state &= (~LED_RED);
  }
  else
  {
    memset((uint8_t *)u8_buf, 0, sizeof(u8_buf));
    FLASH_ReadData(FLASH_LEAF_ADDR(DEFAULT), (uint8_t *)u8_buf, 256); // 读取默认数据
    if (u8_buf[13] == ('D') & u8_buf[14] == ('E') & u8_buf[15] == ('F'))
    {
      led_state |= (LED_RED | LED_GREEN);
      LOG("myDEFAULT_DATA\n");
    }
  }

  if ((u8_buf[0]) > (0xF0)) // 读取失败 ，Flash返回0XFF
  {
    user_settings_init(); // 默认情况
    LOG("myNOR_DATA\n");
  }

  switch (com)
  {
  case IN_MODE_SV: // IN_MODE_SV:
  {
    num = 0;
    InputCVBS_VIC = u8_buf[num * 32 + 16]; // tvmode
  }
  break;
  case IN_MODE_CVBS: // IN_MODE_CVBS:
  {
    num = 1;
    InputCVBS_VIC = u8_buf[num * 32 + 16]; // tvmode
  }
  break;
  case IN_MODE_RGBs: // IN_MODE_RGBs:
  {
    num = 4;
    //            rgbs_dir = u8_buf[num * 32 + 0 ];
  }
  break;
  case IN_MODE_YUV: // IN_MODE_YUV:
  {
    num = 2;
  }
  break;
  case IN_MODE_VGA: // IN_MODE_VGA:
  {
    num = 3;
  }
  break;
  case IN_MODE_DIGITAL: // IN_MODE_DIGITAL:
  {
    num = 5;
  }
  break;
  case IN_MODE_NOR: // IN_MODE_NOR:
  {
    num = 6;
  }
  break;
  default:
    break;
  }
  common_parameter(num);

  //    sys_switch_input_mode();//切换输入端口
  // 亮度，对比度，饱和度，色调 设置
  set_bcsh_brightness(g_u8_display_brightness);
  set_bcsh_contrast(g_u8_display_contrast);
  set_bcsh_saturation(g_u8_display_saturation);
  set_bcsh_hues(g_u8_display_hues);
  _video_manual_scaler(); // 视频偏移设置
  __enable_irq();
  LOG("%02x\n", com);
}

void FLASH_READ_MS1824_DATA(void)
{
  uint16_t i = 0; // u32FlashAddr_save
  memset((uint8_t *)u8_buf, 0, sizeof(u8_buf));
  __disable_irq();
  FLASH_ReadData(FLASH_LEAF_ADDR(USER), (uint8_t *)u8_buf, 256); // 读取用户保存数据用于显示
  g_u8_InputPort = u8_buf[0];
  if (checks_video_data(g_u8_InputPort))
  {
    LOG("USER_DATA\n");
    led_state |= LED_GREEN;
    led_state &= (~LED_RED);
  }
  else
  {
    memset((uint8_t *)u8_buf, 0, sizeof(u8_buf));
    FLASH_ReadData(FLASH_LEAF_ADDR(DEFAULT), (uint8_t *)u8_buf, 256); // 读取默认数据
    if (u8_buf[13] == ('D') & u8_buf[14] == ('E') & u8_buf[15] == ('F'))
    {
      led_state |= (LED_RED | LED_GREEN);
      LOG("DEFAULT_DATA\n");
    }
  }

  if ((u8_buf[0]) > (0xF0)) // 读取失败 ，Flash返回0XFF
  {
    user_settings_init(); // 默认情况
    LOG("NOR_DATA\n");
  }

  g_u8_InputPort = u8_buf[0]; //  从Flash指定地址中读取数据 数组0 的值为1
  switch (g_u8_InputPort)
  {
  case IN_MODE_SV: // IN_MODE_SV:
  {
    i = 0;
    InputCVBS_VIC = u8_buf[i * 32 + 16];
  }
  break;
  case IN_MODE_CVBS: // IN_MODE_CVBS:
  {
    i = 1;
    InputCVBS_VIC = u8_buf[i * 32 + 16];
  }
  break;
  case IN_MODE_YUV: // IN_MODE_YUV:
  {
    i = 2;
  }
  break;
  case IN_MODE_VGA: // IN_MODE_VGA:
  {
    i = 3;
  }
  break;
  case IN_MODE_RGBs: // IN_MODE_RGBs:
  {
    i = 4;
    rgbs_dir = u8_buf[4 * 32 + 0];
  }
  break;
  case IN_MODE_DIGITAL: // IN_MODE_DIGITAL:
  {
    i = 5;
  }
  break;
  case IN_MODE_NOR: // IN_MODE_NOR:
  {
    i = 6;
  }
  break;
  default:
    break;
  }
  common_parameter(i);
  __enable_irq();

  LOG("func->READ_MS1824_DATA:\n");
  //    LOG("%02x\n",u8_buf[0]);

  LOG("InputPort    :0x%02x\n", u8_buf[0]);
  LOG("brightness   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 1]);
  LOG("contrast     :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 2]);
  LOG("saturation   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 3]);
  LOG("hues         :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 4]);
  LOG("rgbsfilter   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 5]);
  LOG("output_index :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 6]);
  LOG("OutputPort   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 7]);
  if (u8_buf[g_u8_InputPort * 32 + 11] == (0xfe))
    LOG("up_down      :-0x%02x\n", u8_buf[g_u8_InputPort * 32 + 8]);
  else
    LOG("up_down      :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 8]);
  if (u8_buf[g_u8_InputPort * 32 + 12] == (0xfe))
    LOG("left_right   :-0x%02x\n", u8_buf[g_u8_InputPort * 32 + 9]);
  else
    LOG("left_right   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 9]);
  LOG("4x3_bak      :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 10]);
  if (g_u8_InputPort <= 0x01)
    LOG("CVBS_VIC     :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 16]);
  LOG("0x%02x-", u8_buf[g_u8_InputPort * 32 + 11]);
  LOG("0x%02x\n", u8_buf[g_u8_InputPort * 32 + 12]);
}

void FLASH_SAVE_DEFAULT(void)
{
  uint16_t i = 0;
  __disable_irq();
  FLASH_ReadData(FLASH_LEAF_ADDR(DEFAULT), (uint8_t *)u8_buf, 256);
  //        QSPI_FLASH_Read(FLASH_LEAF_ADDR(DEFAULT), (uint8_t *)u8_buf, 256);
  if (u8_buf[13] == ('D') & u8_buf[14] == ('E') & u8_buf[15] == ('F'))
  {
    memset((uint8_t *)u8_buf, 0, sizeof(u8_buf));
    LOG("LL_OK_Read\n");
  }
  else
  {
    user_settings_init();
    FLASH_EraseSector(FLASH_LEAF_ADDR(DEFAULT), FLASH_SECTOR_SIZE);
    //            QSPI_FLASH_EraseSector(FLASH_LEAF_ADDR(DEFAULT));  //清除扇区
    if (LL_OK == FLASH_WriteData(FLASH_LEAF_ADDR(DEFAULT), (uint8_t *)u8_buf, 256))
    {
      __enable_irq();
      LOG("LL_OK_Write\n");
    }
    __enable_irq();
  }
  __enable_irq();
}

void mem_mygui_settings(void)
{

  FLASH_ReadData(EFM_BASE + FLASH_LEAF_ADDR(USER), (uint8_t *)u8_buf, 256);

  // 如果Flash中的用户区没有保存任何用户信息  则读取默认信息进行填充再进行修改
  if (u8_buf[13] != 'S' &&
      u8_buf[14] != 'E')
  {
    FLASH_ReadData(EFM_BASE + FLASH_LEAF_ADDR(DEFAULT), (uint8_t *)u8_buf, 256);
  }

  u8_buf[0] = g_u8_InputPort; // 当前模式

  // 根据当前模式分区存储用户信息
  if (g_u8_InputPort == IN_MODE_RGBs) // RGBS 端口选择
  {
    u8_buf[g_u8_InputPort * 32 + 0] = rgbs_dir;
  }
  u8_buf[g_u8_InputPort * 32 + 1] = g_u8_display_brightness;
  u8_buf[g_u8_InputPort * 32 + 2] = g_u8_display_contrast;
  u8_buf[g_u8_InputPort * 32 + 3] = g_u8_display_saturation;
  u8_buf[g_u8_InputPort * 32 + 4] = g_u8_display_hues;

  u8_buf[g_u8_InputPort * 32 + 5] = g_u8_display_rgbsfilter;

  u8_buf[g_u8_InputPort * 32 + 6] = g_u8_output_index;
  u8_buf[g_u8_InputPort * 32 + 7] = g_u8_OutputPort;

  if (adjust_value_up_down < 0)
    u8_buf[g_u8_InputPort * 32 + 11] = 0xfe; // 负数
  else
    u8_buf[g_u8_InputPort * 32 + 11] = 0xfd;
  if (adjust_value_left_right < 0)
    u8_buf[g_u8_InputPort * 32 + 12] = 0xfe; // 负数
  else
    u8_buf[g_u8_InputPort * 32 + 12] = 0xfd;

  u8_buf[g_u8_InputPort * 32 + 8] = abs(adjust_value_up_down);
  u8_buf[g_u8_InputPort * 32 + 9] = abs(adjust_value_left_right);
  u8_buf[g_u8_InputPort * 32 + 10] = key_switch_4x3_bak;
  u8_buf[g_u8_InputPort * 32 + 16] = InputCVBS_VIC;

  // #define S_Video 0x01
  // #define CVBS    0x02
  // #define RGBs    0x04
  // #define YPBPR   0x08
  // #define VGA     0x10

  // IN_MODE_SV		= 0,
  // IN_MODE_CVBS	= 1,
  // IN_MODE_YUV     = 2,
  // IN_MODE_VGA     = 3,
  // IN_MODE_RGBs    = 4,
  // IN_MODE_DIGITAL = 5,
  // IN_MODE_NOR     = 0XFF,
  u8_buf[13] = 'S';
  u8_buf[14] = 'E';
  switch (g_u8_InputPort)
  {
  case IN_MODE_SV:
  {
    u8_buf[15] |= S_Video;
  }
  break;
  case IN_MODE_CVBS:
  {
    u8_buf[15] |= CVBS;
  }
  break;
  case IN_MODE_YUV:
  {
    u8_buf[15] |= YPBPR;
  }
  break;
  case IN_MODE_VGA:
  {
    u8_buf[15] |= VGA;
  }
  break;
  case IN_MODE_RGBs:
  {
    if (rgbs_dir == 0)
      u8_buf[15] |= RGBs1;
    else if (rgbs_dir == 1)
      u8_buf[15] |= RGBs2;
  }
  break;
  default:
    break;
  }

  __disable_irq();                                                        // 可以使用这个函数 关闭总中断
  FLASH_EraseSector(EFM_BASE + FLASH_LEAF_ADDR(USER), FLASH_SECTOR_SIZE); // 清除扇区

  FLASH_WriteData(EFM_BASE + FLASH_LEAF_ADDR(USER), (uint8_t *)u8_buf, 256);
  //    FLASH_ReadData(EFM_BASE+FLASH_LEAF_ADDR(USER), (uint8_t *)u8_buf, 256);
  __enable_irq();

  LOG("InputPort    :0x%02x\n", u8_buf[0]);
  if (g_u8_InputPort == IN_MODE_RGBs)
  {
    LOG("rgbs_dir   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 0]);
  }
  LOG("brightness   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 1]);
  LOG("contrast     :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 2]);
  LOG("saturation   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 3]);
  LOG("hues         :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 4]);
  LOG("rgbsfilter   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 5]);
  LOG("output_index :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 6]);
  LOG("OutputPort   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 7]);
  if (u8_buf[g_u8_InputPort * 32 + 11] == (0xfe))
    LOG("up_down      :-0x%02x\n", u8_buf[g_u8_InputPort * 32 + 8]);
  else
    LOG("up_down      :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 8]);
  if (u8_buf[g_u8_InputPort * 32 + 12] == (0xfe))
    LOG("left_right   :-0x%02x\n", u8_buf[g_u8_InputPort * 32 + 9]);
  else
    LOG("left_right   :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 9]);
  LOG("4x3_bak      :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 10]);
  if (g_u8_InputPort <= 0x01)
    LOG("CVBS_VIC     :0x%02x\n", u8_buf[g_u8_InputPort * 32 + 16]);
  LOG("0x%02x-", u8_buf[g_u8_InputPort * 32 + 11]);
  LOG("0x%02x\n", u8_buf[g_u8_InputPort * 32 + 12]);
  //    LOG("SAVE_Settings\n");
}

// #define S-Video 0x01
// #define CVBS    0x02
// #define RGBs    0x04
// #define YPBPR   0x08
// #define VGA     0x10

// IN_MODE_SV		= 0,
// IN_MODE_CVBS	= 1,
// IN_MODE_YUV     = 2,
// IN_MODE_VGA     = 3,
// IN_MODE_RGBs    = 4,
// IN_MODE_DIGITAL = 5,
// IN_MODE_NOR     = 0XFF,

// 下载默认数据
void mem_mygui_default(void)
{

  __disable_irq();                                             // FLASH_LEAF_ADDR(USER)
  FLASH_EraseSector(FLASH_LEAF_ADDR(USER), FLASH_SECTOR_SIZE); // 清除用户扇区

  //    FLASH_ReadData(FLASH_LEAF_ADDR(DEFAULT), (uint8_t *)u8_buf, 256);   // 获取默认数据
  FLASH_EraseSector(FLASH_LEAF_ADDR(DEFAULT), FLASH_SECTOR_SIZE); // 清除默认扇区
  user_settings_init();
  u8_buf[0] = g_u8_InputPort; // 保留输入协议
  FLASH_WriteData(FLASH_LEAF_ADDR(DEFAULT), (uint8_t *)u8_buf, 256);
  __enable_irq();
  //    u8_buf[13] = 'D';
  //    u8_buf[14] = 'E';
  //    u8_buf[15] = 'F';
  //    g_u8_InputPort = u8_buf[0];    //  从Flash指定地址中读取数据 数组0 的值为1
  switch (g_u8_InputPort)
  {
  case IN_MODE_SV: // IN_MODE_SV:
  {
    g_u8_display_brightness = u8_buf[1];
    g_u8_display_contrast = u8_buf[2];
    g_u8_display_saturation = u8_buf[3];
    g_u8_display_hues = u8_buf[4];
    g_u8_display_rgbsfilter = u8_buf[5];
    g_u8_output_index = u8_buf[6];               // = 1;  // 1=1080p // 0=720p
    g_u8_OutputPort = u8_buf[7];                 //  = OUT_MODE_HDMI;
    if (u8_buf[11] == (0xfe))                    // 负数
      adjust_value_up_down = (0 - u8_buf[8]);    //
    else                                         // fd
      adjust_value_up_down = u8_buf[8];          //
    if (u8_buf[12] == (0xfe))                    // 负数
      adjust_value_left_right = (0 - u8_buf[9]); // 有符号
    else                                         // fd
      adjust_value_left_right = u8_buf[9];

    key_switch_4x3_bak = u8_buf[10];
    InputCVBS_VIC = u8_buf[16];
  }
  break;
  case IN_MODE_CVBS: // IN_MODE_CVBS:
  {
    g_u8_display_brightness = u8_buf[1 * 32 + 1];
    g_u8_display_contrast = u8_buf[1 * 32 + 2];
    g_u8_display_saturation = u8_buf[1 * 32 + 3];
    g_u8_display_hues = u8_buf[1 * 32 + 4];
    g_u8_display_rgbsfilter = u8_buf[1 * 32 + 5];
    g_u8_output_index = u8_buf[1 * 32 + 6]; // = 1;  // 1=1080p // 0=720p
    g_u8_OutputPort = u8_buf[1 * 32 + 7];   //  = OUT_MODE_HDMI;
    if (u8_buf[1 * 32 + 11] == (0xfe))      // 负数
      adjust_value_up_down = (0 - u8_buf[1 * 32 + 8]);
    else // fd
      adjust_value_up_down = u8_buf[1 * 32 + 8];
    if (u8_buf[1 * 32 + 12] == (0xfe)) // 负数
      adjust_value_left_right = (0 - u8_buf[1 * 32 + 9]);
    else // fd
      adjust_value_left_right = u8_buf[1 * 32 + 9];
    key_switch_4x3_bak = u8_buf[1 * 32 + 10];
    InputCVBS_VIC = u8_buf[1 * 32 + 16];
  }
  break;
  case IN_MODE_YUV: // IN_MODE_RGBs:
  {
    rgbs_dir = u8_buf[2 * 32 + 0];
    g_u8_display_brightness = u8_buf[2 * 32 + 1];
    g_u8_display_contrast = u8_buf[2 * 32 + 2];
    g_u8_display_saturation = u8_buf[2 * 32 + 3];
    g_u8_display_hues = u8_buf[2 * 32 + 4];
    g_u8_display_rgbsfilter = u8_buf[2 * 32 + 5];
    g_u8_output_index = u8_buf[2 * 32 + 6]; // = 1;  // 1=1080p // 0=720p
    g_u8_OutputPort = u8_buf[2 * 32 + 7];   //  = OUT_MODE_HDMI;
    if (u8_buf[2 * 32 + 11] == (0xfe))      // 负数
      adjust_value_up_down = (0 - u8_buf[2 * 32 + 8]);
    else // fd
      adjust_value_up_down = u8_buf[2 * 32 + 8];
    if (u8_buf[2 * 32 + 12] == (0xfe)) // 负数
      adjust_value_left_right = (0 - u8_buf[2 * 32 + 9]);
    else // fd
      adjust_value_left_right = u8_buf[2 * 32 + 9];
    key_switch_4x3_bak = u8_buf[2 * 32 + 10];
    InputCVBS_VIC = u8_buf[2 * 32 + 16];
  }
  break;
  case IN_MODE_VGA: // IN_MODE_YUV:
  {
    g_u8_display_brightness = u8_buf[3 * 32 + 1];
    g_u8_display_contrast = u8_buf[3 * 32 + 2];
    g_u8_display_saturation = u8_buf[3 * 32 + 3];
    g_u8_display_hues = u8_buf[3 * 32 + 4];
    g_u8_display_rgbsfilter = u8_buf[3 * 32 + 5];
    g_u8_output_index = u8_buf[3 * 32 + 6]; // = 1;  // 1=1080p // 0=720p
    g_u8_OutputPort = u8_buf[3 * 32 + 7];   //  = OUT_MODE_HDMI;
    if (u8_buf[3 * 32 + 11] == (0xfe))      // 负数
      adjust_value_up_down = (0 - u8_buf[3 * 32 + 8]);
    else // fd
      adjust_value_up_down = u8_buf[3 * 32 + 8];
    if (u8_buf[3 * 32 + 12] == (0xfe)) // 负数
      adjust_value_left_right = (0 - u8_buf[3 * 32 + 9]);
    else // fd
      adjust_value_left_right = u8_buf[3 * 32 + 9];
    key_switch_4x3_bak = u8_buf[3 * 32 + 10];
    InputCVBS_VIC = u8_buf[3 * 32 + 16];
  }
  break;
  case IN_MODE_RGBs: // IN_MODE_VGA:
  {
    g_u8_display_brightness = u8_buf[4 * 32 + 1];
    g_u8_display_contrast = u8_buf[4 * 32 + 2];
    g_u8_display_saturation = u8_buf[4 * 32 + 3];
    g_u8_display_hues = u8_buf[4 * 32 + 4];
    g_u8_display_rgbsfilter = u8_buf[4 * 32 + 5];
    g_u8_output_index = u8_buf[4 * 32 + 6]; // = 1;  // 1=1080p // 0=720p
    g_u8_OutputPort = u8_buf[4 * 32 + 7];   //  = OUT_MODE_HDMI;
    if (u8_buf[4 * 32 + 11] == (0xfe))      // 负数
      adjust_value_up_down = (0 - u8_buf[4 * 32 + 8]);
    else // fd
      adjust_value_up_down = u8_buf[4 * 32 + 8];
    if (u8_buf[4 * 32 + 12] == (0xfe)) // 负数
      adjust_value_left_right = (0 - u8_buf[4 * 32 + 9]);
    else // fd
      adjust_value_left_right = u8_buf[4 * 32 + 9];
    key_switch_4x3_bak = u8_buf[4 * 32 + 10];
    InputCVBS_VIC = u8_buf[4 * 32 + 16];
  }
  break;
  case IN_MODE_DIGITAL: // IN_MODE_DIGITAL:
  {
    g_u8_display_brightness = u8_buf[5 * 32 + 1];
    g_u8_display_contrast = u8_buf[5 * 32 + 2];
    g_u8_display_saturation = u8_buf[5 * 32 + 3];
    g_u8_display_hues = u8_buf[5 * 32 + 4];
    g_u8_display_rgbsfilter = u8_buf[5 * 32 + 5];
    g_u8_output_index = u8_buf[5 * 32 + 6]; // = 1;  // 1=1080p // 0=720p
    g_u8_OutputPort = u8_buf[5 * 32 + 7];   //  = OUT_MODE_HDMI;
    if (u8_buf[5 * 32 + 11] == (0xfe))      // 负数
      adjust_value_up_down = (0 - u8_buf[5 * 32 + 8]);
    else // fd
      adjust_value_up_down = u8_buf[5 * 32 + 8];
    if (u8_buf[5 * 32 + 12] == (0xfe)) // 负数
      adjust_value_left_right = (0 - u8_buf[5 * 32 + 9]);
    else // fd
      adjust_value_left_right = u8_buf[5 * 32 + 9];
    key_switch_4x3_bak = u8_buf[5 * 32 + 10];
    InputCVBS_VIC = u8_buf[5 * 32 + 16];
  }
  break;
  case IN_MODE_NOR: // IN_MODE_NOR:
  {
    g_u8_display_brightness = u8_buf[6 * 32 + 1];
    g_u8_display_contrast = u8_buf[6 * 32 + 2];
    g_u8_display_saturation = u8_buf[6 * 32 + 3];
    g_u8_display_hues = u8_buf[6 * 32 + 4];
    g_u8_display_rgbsfilter = u8_buf[6 * 32 + 5];
    g_u8_output_index = u8_buf[6 * 32 + 6]; // = 1;  // 1=1080p // 0=720p
    g_u8_OutputPort = u8_buf[6 * 32 + 7];   //  = OUT_MODE_HDMI;
    if (u8_buf[6 * 32 + 11] == (0xfe))      // 负数
      adjust_value_up_down = (0 - u8_buf[6 * 32 + 8]);
    else // fd
      adjust_value_up_down = u8_buf[6 * 32 + 8];
    if (u8_buf[6 * 32 + 12] == (0xfe)) // 负数
      adjust_value_left_right = (0 - u8_buf[6 * 32 + 9]);
    else // fd
      adjust_value_left_right = u8_buf[6 * 32 + 9];
    key_switch_4x3_bak = u8_buf[6 * 32 + 10];
    InputCVBS_VIC = u8_buf[6 * 32 + 16]; // tvmode
  }
  break;
  default:
    break;
  }

  sys_switch_input_mode();
  _video_manual_scaler();
  sys_switch_ouput_mode();
  set_bcsh_brightness(g_u8_display_brightness);
  set_bcsh_contrast(g_u8_display_contrast);
  set_bcsh_saturation(g_u8_display_saturation);
  set_bcsh_hues(g_u8_display_hues);
  mculib_i2c_write_16bidx8bval(0xb2, 0x10C, g_u8_display_rgbsfilter); // CB

  LOG("InputPort:0x%02x\n", g_u8_InputPort);
  LOG("brightness:0x%02x\n", g_u8_display_brightness);
  LOG("contrast:0x%02x\n", g_u8_display_contrast);
  LOG("saturation:0x%02x\n", g_u8_display_saturation);
  LOG("hues:0x%02x\n", g_u8_display_hues);
  LOG("rgbsfilter:0x%02x\n", g_u8_display_rgbsfilter);
  LOG("output_index:0x%02x\n", g_u8_output_index);
  LOG("OutputPort:0x%02x\n", g_u8_OutputPort);
  LOG("up_down:0x%02x\n", adjust_value_up_down);
  LOG("left_right:0x%02x\n", adjust_value_left_right);
  LOG("4x3_bak:0x%02x\n", key_switch_4x3_bak);
  LOG("CVBS_VIC:0x%02x\n", InputCVBS_VIC);
}

void color_default(uint8_t m)
{
  if (m == USER)
  {
    FLASH_ReadData(FLASH_LEAF_ADDR(USER), (uint8_t *)u8_buf, 256); // 读取用户保存数据用于显示
    if (checks_video_data(g_u8_InputPort))
    {
      LOG("USER_DATA\n");
    }
    else
    {
      memset((uint8_t *)u8_buf, 0, sizeof(u8_buf));
      FLASH_ReadData(FLASH_LEAF_ADDR(DEFAULT), (uint8_t *)u8_buf, 256); // 读取默认数据
      if (u8_buf[13] == ('D') & u8_buf[14] == ('E') & u8_buf[15] == ('F'))
      {
        LOG("DEFAULT_DATA\n");
      }
    }
  }
  else if (m == DEFAULT) // default
  {
    memset((uint8_t *)u8_buf, 0, sizeof(u8_buf));
    FLASH_ReadData(FLASH_LEAF_ADDR(DEFAULT), (uint8_t *)u8_buf, 256); // 读取默认数据
    if (u8_buf[13] == ('D') & u8_buf[14] == ('E') & u8_buf[15] == ('F'))
    {
      LOG("DEFAULT_DATA\n");
    }
  }

  if ((u8_buf[0]) > (0xF0)) // 读取失败 ，Flash返回0XFF
  {
    user_settings_init(); // 默认情况
    LOG("NOR_DATA\n");
  }

  g_u8_display_brightness = u8_buf[g_u8_InputPort * 32 + 1];
  g_u8_display_contrast = u8_buf[g_u8_InputPort * 32 + 2];
  g_u8_display_saturation = u8_buf[g_u8_InputPort * 32 + 3];
  g_u8_display_hues = u8_buf[g_u8_InputPort * 32 + 4];

  //    g_u8_display_brightness     =      50;
  //    g_u8_display_contrast       =      50;
  //    g_u8_display_saturation     =      50;
  //    g_u8_display_hues           =      50;

  set_bcsh_brightness(g_u8_display_brightness);
  set_bcsh_contrast(g_u8_display_contrast);
  set_bcsh_saturation(g_u8_display_saturation);
  set_bcsh_hues(g_u8_display_hues);
}

void Save_Run(xpMenu Menu)
{
  mem_mygui_settings();
}

void Load_Run(xpMenu Menu)
{
  mem_mygui_default();
}
/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
