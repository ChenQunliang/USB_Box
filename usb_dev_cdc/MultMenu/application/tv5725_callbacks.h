#ifndef TV5725_CALLBACKS_H
#define TV5725_CALLBACKS_H

#include "menu.h"

/* Input source selection callbacks */
void cb_input_vga(xpMenu Menu);
void cb_input_rgbs(xpMenu Menu);
void cb_input_rgsb(xpMenu Menu);

/* Output resolution selection callbacks */
void cb_res_480p(xpMenu Menu);
void cb_res_720p(xpMenu Menu);
void cb_res_960p(xpMenu Menu);
void cb_res_1080p(xpMenu Menu);

/* SOG mode selection callbacks */
void cb_sog_normal(xpMenu Menu);
void cb_sog_force(xpMenu Menu);
void cb_sog_show(xpMenu Menu);

/* Chip ID callbacks */
void cb_chip_id_show(xpMenu Menu);

#endif /* TV5725_CALLBACKS_H */
