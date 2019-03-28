#define os_log(format, ...)  custom_log("KEY", format, ##__VA_ARGS__)

#include "main.h"
#include "user_gpio.h"
#include "user_mqtt_client.h"
#include "user_udp.h"
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
    unsigned char i;
    for ( i = 0; i < PLUG_NUM; i++ )
    {
        if ( user_config->plug[i].on != 0 )
        {
            return true;
        }
    }
    return false;
}

/*user_relay_set
 * 设置继电器开关
 * x:编号 0-5
 * y:开关 0:关 1:开
 */
void user_relay_set(unsigned char x,unsigned char y )
{
    if (x >= PLUG_NUM ) return;

    if((y == 1) ? Relay_ON : Relay_OFF) MicoGpioOutputHigh( relay[x] );else MicoGpioOutputLow( relay[x] );

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
//    os_log("key_long_press");
//    user_led_set( 1 );
//    user_mqtt_send( "mqtt test" );
}

static void key_long_10s_press( void )
{
    OSStatus err;
    char i = 0;
    os_log( "WARNGIN: user params restored!" );
//    for ( i = 0; i < 3; i++ )
//    {
//        user_led_set( 1 );
//        mico_rtos_thread_msleep( 100 );
//        user_led_set( 0 );
//    }
//
    appRestoreDefault_callback( user_config, sizeof(user_config_t) );
    sys_config->micoSystemConfig.ssid[0] = 0;
    mico_system_context_update( mico_system_context_get( ) );
}
static void key_short_press( void )
{
    char i;
    OSStatus err;

    if ( relay_out() )
    {
        user_relay_set_all( 0 );
    }
    else
    {
        user_relay_set_all( 1 );
    }

    for ( i = 0; i < PLUG_NUM; i++ )
    {
        user_mqtt_send_plug_state(i);
    }



}
mico_timer_t user_key_timer;
uint16_t key_time = 0;
#define BUTTON_LONG_PRESS_TIME    10     //100ms*10=1s

static void key_timeout_handler( void* arg )
{

    static uint8_t key_trigger, key_continue;
    static uint8_t key_last;
    //按键扫描程序
    uint8_t tmp = ~(0xfe | MicoGpioInputGet( Button ));
    key_trigger = tmp & (tmp ^ key_continue);
    key_continue = tmp;
//    os_log("button scan:%02x %02x",key_trigger,key_continue);
    if ( key_trigger != 0 ) key_time = 0; //新按键按下时,重新开始按键计时
    if ( key_continue != 0 )
    {
        //any button pressed
        key_time++;
        if ( key_time < BUTTON_LONG_PRESS_TIME )
            key_last = key_continue;
        else
        {
            os_log("button long pressed:%d",key_time);

            if ( key_time == 30 )
            {
                key_long_press( );
            }
            else if ( key_time == 100 )
            {
                key_long_10s_press( );
            }
            else if ( key_time == 102 )
            {
                user_led_set( 1 );
            }
            else if ( key_time == 103 )
            {
                user_led_set( 0 );
                key_time = 101;
            }
        }

    } else
    {
        //button released
        if ( key_time < BUTTON_LONG_PRESS_TIME )
        {   //100ms*10=1s 大于1s为长按
            key_time = 0;
            os_log("button short pressed:%d",key_time);
            key_short_press( );
        } else if ( key_time > 100 )
        {
            MicoSystemReboot( );
        }
        key_last = 0;
        mico_rtos_stop_timer( &user_key_timer );
    }
}

static void key_falling_irq_handler( void* arg )
{
    mico_rtos_start_timer( &user_key_timer );
}
void key_init( void )
{
    MicoGpioInitialize( Button, INPUT_PULL_UP );
    mico_rtos_init_timer( &user_key_timer, 100, key_timeout_handler, NULL );

    MicoGpioEnableIRQ( Button, IRQ_TRIGGER_FALLING_EDGE, key_falling_irq_handler, NULL );

}

