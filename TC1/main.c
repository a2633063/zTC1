#include "main.h"

#include "user_key.h"
#include "user_wifi.h"

#define os_log(format, ...)  custom_log("TC1", format, ##__VA_ARGS__)

system_config_t * sys_config;
user_config_t * user_config;

mico_gpio_t Relay[Relay_NUM] = { Relay_0, Relay_1, Relay_2, Relay_3, Relay_4, Relay_5 };

int application_start( void )
{

    os_log( "Start" );

    OSStatus err = kNoErr;

    /* Create mico system context and read application's config data from flash */
    sys_config = mico_system_context_init( sizeof(user_config_t) );
    user_config = ((system_context_t *) sys_config)->user_config_data;
    require_action( user_config, exit, err = kNoMemoryErr );
    os_log( "user config:%d",user_config->val );
    err = mico_system_init( sys_config );
    require_noerr( err, exit );


    for ( int i = 0; i < Relay_NUM; i++ )
    {
        MicoGpioInitialize( Relay[i], OUTPUT_PUSH_PULL );
        //MicoGpioOutputHigh(Relay[i]);
    }

    MicoGpioInitialize( (mico_gpio_t) Button, INPUT_PULL_UP );
    if ( !MicoGpioInputGet( Button ) )
    {   //开机时按钮状态
        os_log( "wifi_start_easylink" );
        wifi_status = WIFI_STATE_NOEASYLINK;  //wifi_init中启动easylink
    }

    MicoGpioInitialize( (mico_gpio_t) MICO_GPIO_5, OUTPUT_PUSH_PULL );
    led( 0 );

    wifi_init( );
    key_init( );
//    wifi_start_easylink();
    while ( 1 )
    {
//        mico_thread_msleep(500);
//        MicoGpioOutputTrigger(MICO_GPIO_5);
//        mico_gpio_output_toggle( MICO_SYS_LED );
//        mico_rtos_delay_milliseconds(1000);
    }
    exit:
    return 0;
}

