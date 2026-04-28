/*

MIT License

Copyright (c) 2024 Zhang-JianFeng

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "menuConfig.h"
#include "menu.h"
#include "dispDirver.h"
#include "image.h"
#include "application.h"
#include "tv5725_callbacks.h"
//#include "flash.h"
#include "main.h"
//#include "DinoGame.h"
//#include "AirPlane.h"

/* Page*/
xPage Home_Page, Input_Page, Format_Page, Color_Page, Screen_Page, OutPut_Page, SOG_Page;
/* item */
xItem HomeHead_Item; 
xItem InputHead_Item, ColorHead_Item, FormatHead_Item, ScreenHead_Item, OutPutHead_Item;
xItem Input_Item    , Color_Item    , Format_Item    , Screen_Item    , OutPut_Item    ;
/*signal*/
xItem RGBS_Item, RGSB_Item, YUV_Item;
/*ChipID*/
xItem ChipID_Item;
/*SOG*/
xItem SOG_Item, SOGHead_Item, SOG_Normal_Item, SOG_Force_Item, SOG_Show_Item;
/*Format*/
xItem Auto_Item, Ntsc_Item, Ntsc443_Item, Pal_Item, Pal_M_Item, Secam_Item;
/*Color*/
xItem Brightness_Item, Contrast_Item, Saturation_Item, Hues_Item;
/*Screen*/
xItem LeftAndRight_Item, UpAndDown_Item;
/*Resolution*/
xItem  Resolution720_4_Item,    Resolution720_16_Item,  Resolution1080_4_Item,  Resolution1080_16_Item;
/*Save*/
xItem Save_Item;
/*Load*/
xItem Load_Item;





extern int test;

/**
 * 在此建立所需显示或更改的数据
 * 无参数
 * 无返回值
 SetBrightness
 SetContrast  
 SetSaturation
 SetHues      
 
 g_u8_display_brightness;
 g_u8_display_contrast;
 g_u8_display_saturation;
 g_u8_display_hues;
 
 
 
 */
void Create_Parameter(void)
{   
    
    static int Brightness ;
    static data_t Brightness_data;
    Brightness_data.name = "Brightness";
    Brightness_data.ptr = &Brightness;
    Brightness_data.function = NULL;
    Brightness_data.Function_Type = STEP_EXECUTE;
    Brightness_data.Data_Type = DATA_INT;
    Brightness_data.Operate_Type = READ_WRITE;
    Brightness_data.max = 100;
    Brightness_data.min = 0;
    Brightness_data.step = 1;
    static element_t Brightness_element;
    Brightness_element.data = &Brightness_data;
    Create_element(&Brightness_Item, &Brightness_element);
    
//    static int Contrast = 100;
    static data_t Contrast_data;
    Contrast_data.name = "Contrast";
    Contrast_data.ptr = &Brightness;
    Contrast_data.function = NULL;
    Contrast_data.Function_Type = STEP_EXECUTE;
    Contrast_data.Data_Type = DATA_INT;
    Contrast_data.Operate_Type = READ_WRITE;
    Contrast_data.max = 100;
    Contrast_data.min = 0;
    Contrast_data.step = 1;
    static element_t Contrast_element;
    Contrast_element.data = &Contrast_data;
    Create_element(&Contrast_Item, &Contrast_element);
    
    
//    static int Saturation = 100;
    static data_t Saturation_data;
    Saturation_data.name = "Saturation";
    Saturation_data.ptr = &Brightness;
    Saturation_data.function = NULL;
    Saturation_data.Function_Type = STEP_EXECUTE;
    Saturation_data.Data_Type = DATA_INT;
    Saturation_data.Operate_Type = READ_WRITE;
    Saturation_data.max = 100;
    Saturation_data.min = 0;
    Saturation_data.step = 1;
    static element_t Saturation_element;
    Saturation_element.data = &Saturation_data;
    Create_element(&Saturation_Item, &Saturation_element);
    
//    static int Hues = 100;
    static data_t Hues_data;
    Hues_data.name = "Hues";
    Hues_data.ptr = &Brightness;
    Hues_data.function = NULL;
    Hues_data.Function_Type = STEP_EXECUTE;
    Hues_data.Data_Type = DATA_INT;
    Hues_data.Operate_Type = READ_WRITE;
    Hues_data.max = 100;
    Hues_data.min = 0;
    Hues_data.step = 1;
    static element_t hues_element;
    hues_element.data = &Hues_data;
    Create_element(&Hues_Item, &hues_element);





//    static int LeftAndRight = 0;
    static data_t LeftAndRight_data;
    LeftAndRight_data.name = "LeftAndRight";
    LeftAndRight_data.ptr = &Brightness;
    LeftAndRight_data.function = NULL;
    LeftAndRight_data.Function_Type = STEP_EXECUTE;
    LeftAndRight_data.Data_Type = DATA_INT;
    LeftAndRight_data.Operate_Type = READ_WRITE;
    LeftAndRight_data.max = 20;
    LeftAndRight_data.min = -20;
    LeftAndRight_data.step = 1;
    static element_t LeftAndRight_element;
    LeftAndRight_element.data = &LeftAndRight_data;
    Create_element(&LeftAndRight_Item, &LeftAndRight_element);

//    static int UpAndDown = 0;
    static data_t UpAndDown_data;
    UpAndDown_data.name = "UpAndDown";
    UpAndDown_data.ptr = &Brightness;
    UpAndDown_data.function = NULL;
    UpAndDown_data.Function_Type = STEP_EXECUTE;
    UpAndDown_data.Data_Type = DATA_INT;
    UpAndDown_data.Operate_Type = READ_WRITE;
    UpAndDown_data.max = 20;
    UpAndDown_data.min = -20;
    UpAndDown_data.step = 1;
    static element_t UpAndDown_element;
    UpAndDown_element.data = &UpAndDown_data;
    Create_element(&UpAndDown_Item, &UpAndDown_element);
//    static uint8_t power = true;
//    static data_t Power_switch_data;
//    Power_switch_data.ptr = &power;
//    Power_switch_data.function = OLED_SetPowerSave;
//    Power_switch_data.Data_Type = DATA_SWITCH;
//    Power_switch_data.Operate_Type = READ_WRITE;
//    static element_t Power_element;
//    Power_element.data = &Power_switch_data;
//    Create_element(&Power_Item, &Power_element);

//    static data_t Wave_data;
//    Wave_data.name = "Wave";
//    Wave_data.ptr = &test;
//    Wave_data.Data_Type = DATA_INT;
//    Wave_data.max = 360;
//    Wave_data.min = 0;
//    static element_t Wave_element;
//    Wave_element.data = &Wave_data;
//    Create_element(&Wave_Item, &Wave_element);
}

/**
 * 在此建立所需显示或更改的文本
 * 无参数
 * 无返回值
 */
void Create_Text(void)
{
//    static text_t github_text;
//    github_text.font = MENU_FONT;
//    github_text.font_hight = Font_Hight;
//    github_text.font_width = Font_Width;
//    github_text.ptr = "https://github.com/JFeng-Z/MultMenu";
//    static element_t github_element;
//    github_element.text = &github_text;
//    Create_element(&Github_Item, &github_element);

//    static text_t bilibili_text;
//    bilibili_text.font = MENU_FONT;
//    bilibili_text.font_hight = Font_Hight;
//    bilibili_text.font_width = Font_Width;
//    bilibili_text.ptr = "https://www.bilibili.com/video/BV1d4421Q7kD?vd_source=11fa79768e087179635ff2a439abe018";
//    static element_t bilibili_element;
//    bilibili_element.text = &bilibili_text;
//    Create_element(&Bilibili_Item, &bilibili_element);
}

/*
 * 菜单构建函数
 * 该函数不接受参数，也不返回任何值。
 * 功能：静态地构建一个菜单系统。
//                AddItem(" -Contrast", DATA, NULL, &Contrast_Item, &Input_Page, NULL, NULL);
 */
void Create_MenuTree(xpMenu Menu)
{
    AddPage("[HomePage]", &Home_Page, IMAGE);
        AddItem("[HomePage]", LOOP_FUNCTION, NULL, &HomeHead_Item, &Home_Page, NULL, Show_Logo);
        AddItem(" +Input", PARENTS, logo_allArray[1], &Input_Item, &Home_Page, &Input_Page, NULL);
            AddPage("[Back]", &Input_Page, TEXT);
                AddItem("[Back]", RETURN, NULL, &InputHead_Item,   &Input_Page, &Home_Page, NULL);
                AddItem(" -RGBS", ONCE_FUNCTION,   NULL, &RGBS_Item, &Input_Page, NULL, cb_input_rgbs);
                AddItem(" -RGSB", ONCE_FUNCTION,   NULL, &RGSB_Item, &Input_Page, NULL, cb_input_rgsb);
                AddItem(" -YUV" , ONCE_FUNCTION,   NULL, &YUV_Item,  &Input_Page, NULL, cb_input_yuv);
                AddItem(" -ChipID", ONCE_FUNCTION, NULL, &ChipID_Item, &Input_Page, NULL, cb_chip_id_show);
                AddItem(" +SOG Mode", PARENTS, NULL, &SOG_Item, &Input_Page, &SOG_Page, NULL);
                    AddPage("[Back]", &SOG_Page, TEXT);
                        AddItem("[Back]" , RETURN, NULL, &SOGHead_Item, &SOG_Page, &Input_Page, NULL);
                        AddItem(" -Normal", ONCE_FUNCTION, NULL, &SOG_Normal_Item, &SOG_Page, NULL, cb_sog_normal);
                        AddItem(" -Force" , ONCE_FUNCTION, NULL, &SOG_Force_Item , &SOG_Page, NULL, cb_sog_force);
                        AddItem(" -Read"  , ONCE_FUNCTION, NULL, &SOG_Show_Item  , &SOG_Page, NULL, cb_sog_show);
        AddItem(" +VideoFormat", PARENTS, logo_allArray[2], &Format_Item, &Home_Page, &Format_Page, NULL);
            AddPage("[Back]", &Format_Page, TEXT);
                AddItem("[Back]" , RETURN, NULL, &FormatHead_Item    , &Format_Page  , &Home_Page, NULL);
                AddItem(" -Auto"    , ONCE_FUNCTION,   NULL, &Auto_Item     , &Format_Page, NULL, NULL   );       
                AddItem(" -NTSC"    , ONCE_FUNCTION,   NULL, &Ntsc_Item     , &Format_Page, NULL, NULL   ); 
                AddItem(" -NTSC443" , ONCE_FUNCTION,   NULL, &Ntsc443_Item  , &Format_Page, NULL, NULL); 
                AddItem(" -PAL"     , ONCE_FUNCTION,   NULL, &Pal_Item      , &Format_Page, NULL, NULL    ); 
                AddItem(" -PAL_M"   , ONCE_FUNCTION,   NULL, &Pal_M_Item    , &Format_Page, NULL, NULL  ); 
                AddItem(" -SECAM"   , ONCE_FUNCTION,   NULL, &Secam_Item    , &Format_Page, NULL, NULL  ); 
        AddItem(" +Color", PARENTS, logo_allArray[3], &Color_Item, &Home_Page, &Color_Page, NULL);
            AddPage("[Back]", &Color_Page, TEXT);
                AddItem("[Back]" , RETURN, NULL, &ColorHead_Item    , &Color_Page  , &Home_Page, NULL);
                AddItem(" -Brightness"  , DATA  , NULL, &Brightness_Item    , &Color_Page  , NULL      , NULL);
                AddItem(" -Contrast"    , DATA  , NULL, &Contrast_Item      , &Color_Page  , NULL      , NULL);
                AddItem(" -Saturation"  , DATA  , NULL, &Saturation_Item    , &Color_Page  , NULL      , NULL);
                AddItem(" -Hues"        , DATA  , NULL, &Hues_Item          , &Color_Page  , NULL      , NULL);
        AddItem(" +ScreenPosition", PARENTS, logo_allArray[7], &Screen_Item, &Home_Page, &Screen_Page, NULL);
            AddPage("[Back]", &Screen_Page, TEXT);
                AddItem("[Back]" , RETURN, NULL, &ScreenHead_Item     , &Screen_Page  , &Home_Page, NULL);
                AddItem(" -Left&Right" , DATA  , NULL, &LeftAndRight_Item   , &Screen_Page  , NULL      , NULL);
                AddItem(" -Up&Down"    , DATA  , NULL, &UpAndDown_Item      , &Screen_Page  , NULL      , NULL);
        AddItem(" +OutPut", PARENTS, logo_allArray[4], &OutPut_Item, &Home_Page, &OutPut_Page, NULL);
            AddPage("[Back]", &OutPut_Page, TEXT);
                AddItem("[Back]" , RETURN, NULL, &OutPutHead_Item     , &OutPut_Page  , &Home_Page, NULL);
                AddItem(" -720P(4*3)"    , ONCE_FUNCTION,   NULL, &Resolution720_4_Item    , &OutPut_Page, NULL, cb_res_720p_4_3);
                AddItem(" -720P(16*9)"   , ONCE_FUNCTION,   NULL, &Resolution720_16_Item   , &OutPut_Page, NULL, cb_res_720p_16_9);
                AddItem(" -1080P(4*3)"   , ONCE_FUNCTION,   NULL, &Resolution1080_4_Item   , &OutPut_Page, NULL, cb_res_1080p_4_3);
                AddItem(" -1080P(16*9)"  , ONCE_FUNCTION,   NULL, &Resolution1080_16_Item  , &OutPut_Page, NULL, cb_res_1080p_16_9);
        AddItem(" -Save", ONCE_FUNCTION, logo_allArray[5], &Save_Item, &Home_Page, NULL, NULL);
        AddItem(" -LoadDefault", ONCE_FUNCTION, logo_allArray[6], &Load_Item, &Home_Page, NULL, NULL);
                
//        AddItem(" -Image", LOOP_FUNCTION, logo_allArray[6], &Image_Item, &Home_Page, NULL, Show_Logo);
//        AddItem(" -Github", _TEXT_, logo_allArray[5], &Github_Item, &Home_Page, NULL, NULL);
//        AddItem(" -Bilibili", _TEXT_, logo_allArray[7], &Bilibili_Item, &Home_Page, NULL, NULL);
//        AddItem(" -Wave", WAVE, logo_allArray[9], &Wave_Item, &Home_Page, NULL, NULL);
//        AddItem(" -DinoGame", LOOP_FUNCTION, logo_allArray[3], &Dino_Item, &Home_Page, NULL, DinoGame_Run);
//        AddItem(" -AirPlane", LOOP_FUNCTION, logo_allArray[0], &AirPlane_Item, &Home_Page, NULL, AirPlane_Run);
    
}

void Menu_Init(xpMenu Menu)
{
    Disp_Init();
    Create_Menu(Menu, &HomeHead_Item);
    Draw_Home(NULL);
}
