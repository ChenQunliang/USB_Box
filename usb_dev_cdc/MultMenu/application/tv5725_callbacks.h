#ifndef TV5725_CALLBACKS_H
#define TV5725_CALLBACKS_H

#include "menu.h"

/* Input source selection callbacks */
void cb_input_sv(xpMenu Menu);
void cb_input_cvbs(xpMenu Menu);
void cb_input_rgb0(xpMenu Menu);
void cb_input_rgb1(xpMenu Menu);
void cb_input_yuv(xpMenu Menu);
void cb_input_vga(xpMenu Menu);

/* Output resolution selection callbacks */
void cb_res_720p_4_3(xpMenu Menu);
void cb_res_720p_16_9(xpMenu Menu);
void cb_res_1080p_4_3(xpMenu Menu);
void cb_res_1080p_16_9(xpMenu Menu);

#endif /* TV5725_CALLBACKS_H */
