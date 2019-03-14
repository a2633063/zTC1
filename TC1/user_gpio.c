#define os_log(format, ...)  custom_log("KEY", format, ##__VA_ARGS__)

#include "main.h"
#include "user_gpio.h"
#include "user_mqtt_client.h"
#include "cJSON/cJSON.h"

mico_gpio_t relay[Relay_NUM] = { Relay_0, Relay_1, Relay_2, Relay_3, Relay_4, Relay_5 };

void user_led_set( char x )
{
    if ( x == -1 )
        MicoGpioOutputTrigger( Led );
    else if ( x )
        MicoGpioOutputHigh( Led );
    else
        MicoGpioOutputLow( Led );
}

bool relay_out( void )
{
    char i;
    for ( i = 0; i < PLUG_NUM; i++ )
    {
        if ( user_config->plug[i].on != 0 )
        {
            return true;
        }
    }
    return false;
}
#define set_relay(a,b) if(((b) == 1) ? Relay_ON : Relay_OFF) MicoGpioOutputHigh( relay[(a)] );else MicoGpioOutputLow( relay[(a)] )
/*user_relay_set
 * 设置继电器开关
 * x:编号 0-5
 * y:开关 0:关 1:开
 */
void user_relay_set( char x, char y )
{
    if ( x < 0 || x >= PLUG_NUM ) return;

    set_relay( x, y );
    user_config->plug[x].on = y;

    if ( relay_out( ) )
        user_led_set( 1 );
    else
        user_led_set( 0 );
}

/*
 * 设置所有继电器开关
 * y:0:全部关   1:根据记录状态开关所有
 *
 */
void user_relay_set_all( char y )
{
    char i;
    for ( i = 0; i < PLUG_NUM; i++ )
        user_relay_set( i, y );
}

static void key_long_press( void )
{
    os_log("key_long_press");
    user_led_set( 1 );
    user_mqtt_send( "mqtt test" );

}
static void key_short_press( void )
{
    char i;

    cJSON *json_send = cJSON_CreateObject( );

    cJSON_AddStringToObject( json_send, "mac", strMac );
    if ( user_config->idx >= 0 ) cJSON_AddNumberToObject( json_send, "idx", user_config->idx );

    if ( relay_out( ) )
    {
        user_relay_set_all( 0 );
        cJSON_AddNumberToObject( json_send, "nvalue", 1 );
    }
    else
    {
        user_relay_set_all( 1 );
        cJSON_AddNumberToObject( json_send, "nvalue", 0 );
    }

    char *json_str = cJSON_Print( json_send );
    if ( !user_mqtt_isconnect() )//发送数据
        user_udp_send( json_str );
    else
        user_mqtt_send( json_str );
    free( (void *) json_str );
}

void key_init( void )
{
    button_init_t button_config = {
        .gpio = Button,
        .long_pressed_func = key_long_press,
        .pressed_func = key_short_press,
        .long_pressed_timeout = 800,
    };

    button_init( IOBUTTON_USER_1, button_config );
}

