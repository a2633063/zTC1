#ifndef __USER_MQTT_CLIENT_H_
#define __USER_MQTT_CLIENT_H_


#include "mico.h"



extern OSStatus user_mqtt_init(void);
extern OSStatus user_mqtt_send( char *arg );
extern bool user_mqtt_isconnect(void);
extern OSStatus user_mqtt_send_plug_state( char plug_id );
extern void user_mqtt_hass_auto( char plug_id );
extern void user_mqtt_hass_power( void );
#endif
