/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __HUD_HEADER_INCLUDED__
#define __HUD_HEADER_INCLUDED__

int hud_init(void);
int hud_init2(void);
void hud_render(void);
void hud_toggle_console(void);
void hud_think(void);
void con_input_char(u_int16_t);

#endif /* __HUD_HEADER_INCLUDED__ */
