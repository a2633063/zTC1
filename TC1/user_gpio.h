
#ifndef __USER_KEY_H_
#define __USER_KEY_H_


#include "mico.h"
#include "MiCOKit_EXT.h"

extern void user_led_set(char x);
extern void key_init(void);
extern void user_relay_set(unsigned char x,unsigned char y );
extern void user_relay_set_all( char y );
extern bool relay_out( void );

#endif
