#ifndef __USER_MQTT_CLIENT_H_
#define __USER_MQTT_CLIENT_H_


#include "mico.h"



extern void user_mqtt_init(void);
extern OSStatus user_mqtt_send( char *arg );


#endif
