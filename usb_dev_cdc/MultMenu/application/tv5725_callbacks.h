#ifndef TV5725_CALLBACKS_H
#define TV5725_CALLBACKS_H

#include "menu.h"

/* Input source selection callbacks */
void cb_input_rgbs(xpMenu Menu);
void cb_input_rgsb(xpMenu Menu);
void cb_input_yuv(xpMenu Menu);

/* Output resolution selection callbacks */
void cb_res_720p_4_3(xpMenu Menu);
void cb_res_720p_16_9(xpMenu Menu);
void cb_res_1080p_4_3(xpMenu Menu);
void cb_res_1080p_16_9(xpMenu Menu);

/* SOG mode selection callbacks */
void cb_sog_normal(xpMenu Menu);
void cb_sog_force(xpMenu Menu);
void cb_sog_show(xpMenu Menu);

/* Chip ID callbacks */
void cb_chip_id_show(xpMenu Menu);

#endif /* TV5725_CALLBACKS_H */
