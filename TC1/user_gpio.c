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

void user_relay_set( char x, char y )
{
    char onoff = (y == 1 ? Relay_ON : Relay_OFF);

    OSStatus (* Gpiosetfunction)( mico_gpio_t );
    if ( y == -1 )
        Gpiosetfunction = MicoGpioOutputTrigger;
    else if ( onoff )
        Gpiosetfunction = MicoGpioOutputHigh;
    else
        Gpiosetfunction = MicoGpioOutputLow;


    if ( x >= 0 && x < Relay_NUM )
    {
        (*Gpiosetfunction)( relay[x] );
        os_log("set relay %d:%d",x,y);
    } else if ( x == Relay_NUM )
    {
        for ( int i = 0; i < Relay_NUM; i++ )
        {
            (*Gpiosetfunction)( relay[i] );
            os_log("set relay %d:%d",i,y);
        }
    }
}

static void key_long_press( void )
{
    os_log("key_long_press");
    user_led_set(1);
    user_mqtt_send( "mqtt test" );

}
static void key_short_press( void )
{
//os_log("test");
    user_led_set(-1);
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

