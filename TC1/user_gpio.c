#define os_log(format, ...)  custom_log("KEY", format, ##__VA_ARGS__)

#include "main.h"
#include "user_gpio.h"
#include "user_mqtt_client.h"

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
}

/*
 * 设置所有继电器开关
 * y:0:全部关   1:根据记录状态开关所有
 *
 */
void user_relay_set_all( char y )
{
    char onoff = (y == 1 ? Relay_ON : Relay_OFF);
    char i, temp;

    if ( y != 0 )
    {
        for ( i = 0; i < PLUG_NUM; i++ )
            temp |= user_config->plug[i].on;

        if ( temp == 0 )
        {
            for ( i = 0; i < PLUG_NUM; i++ )
                user_config->plug[i].on = 1;
        }

        for ( i = 0; i < PLUG_NUM; i++ )
            set_relay( i, user_config->plug[i].on );
    }
    else
    {
        set_relay( i, 0 );
    }
}

static void key_long_press( void )
{
    os_log("key_long_press");
    user_led_set( 1 );
    user_mqtt_send( "mqtt test" );

}
static void key_short_press( void )
{
//os_log("test");
    user_led_set( -1 );
    //user_relay_set(6,-1);
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

