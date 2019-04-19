#include "main.h"

#include "user_gpio.h"
#include "user_wifi.h"
#include "user_rtc.h"
#include "user_udp.h"
#include "user_power.h"
#include "user_mqtt_client.h"
#include "user_function.h"
#include "http_server/app_httpd.h"

#define os_log(format, ...)  custom_log("TC1", format, ##__VA_ARGS__)



char rtc_init = 0;    //sntp校时成功标志位
uint32_t total_time=0;
char strMac[16] = { 0 };
uint32_t power=0;

system_config_t * sys_config;
user_config_t * user_config;

mico_gpio_t Relay[Relay_NUM] = { Relay_0, Relay_1, Relay_2, Relay_3, Relay_4, Relay_5 };

/* MICO system callback: Restore default configuration provided by application */
void appRestoreDefault_callback( void * const user_config_data, uint32_t size )
{
    int i, j;
    UNUSED_PARAMETER( size );


    mico_system_context_get( )->micoSystemConfig.name[0] = 1;   //在下次重启时使用默认名称
    mico_system_context_get( )->micoSystemConfig.name[1] = 0;

    user_config_t* userConfigDefault = user_config_data;

    userConfigDefault->user[0] = 0;
    userConfigDefault->mqtt_ip[0] = 0;
    userConfigDefault->mqtt_port = 0;
    userConfigDefault->mqtt_user[0] = 0;
    userConfigDefault->mqtt_password[0] = 0;

    userConfigDefault->version = USER_CONFIG_VERSION;
    for ( i = 0; i < PLUG_NUM; i++ )
    {
        userConfigDefault->plug[i].on = 1;

        //插座名称 插口1-6
        userConfigDefault->plug[i].name[0] = 0xe6;
        userConfigDefault->plug[i].name[1] = 0x8f;
        userConfigDefault->plug[i].name[2] = 0x92;
        userConfigDefault->plug[i].name[3] = 0xe5;
        userConfigDefault->plug[i].name[4] = 0x8f;
        userConfigDefault->plug[i].name[5] = 0xa3;
        userConfigDefault->plug[i].name[6] = i + '1';
        userConfigDefault->plug[i].name[7] = 0;

//        sprintf( userConfigDefault->plug[i].name, "插座%d", i );//编码异常
        for ( j = 0; j < PLUG_TIME_TASK_NUM; j++ )
        {
            userConfigDefault->plug[i].task[j].hour = 0;
            userConfigDefault->plug[i].task[j].minute = 0;
            userConfigDefault->plug[i].task[j].repeat = 0x00;
            userConfigDefault->plug[i].task[j].on = 0;
            userConfigDefault->plug[i].task[j].action = 1;
        }
    }
//    mico_system_context_update( sys_config );

}

int application_start( void )
{
    int i;
    os_log( "Start %s",VERSION );

    uint8_t main_num=0;
    uint32_t power_last = 0xffffffff;
    OSStatus err = kNoErr;

//    for ( i = 0; i < Relay_NUM; i++ )
//    {
//        MicoGpioOutputLow( Relay[(i)] );
//        MicoGpioInitialize( Relay[i], OUTPUT_PUSH_PULL );
//        MicoGpioOutputLow( Relay[(i)] );
//        //MicoGpioOutputHigh(Relay[i]);
//    }
    /* Create mico system context and read application's config data from flash */
    sys_config = mico_system_context_init( sizeof(user_config_t) );
    user_config = ((system_context_t *) sys_config)->user_config_data;
    require_action( user_config, exit, err = kNoMemoryErr );

    err = mico_system_init( sys_config );
    require_noerr( err, exit );

    MicoGpioInitialize( (mico_gpio_t) Button, INPUT_PULL_UP );
    if ( !MicoGpioInputGet( Button ) )
    {   //开机时按钮状态
        os_log( "wifi_start_easylink" );
        wifi_status = WIFI_STATE_NOEASYLINK;  //wifi_init中启动easylink
    }

    MicoGpioInitialize( (mico_gpio_t) Led, OUTPUT_PUSH_PULL );
    for ( i = 0; i < Relay_NUM; i++ )
    {
        MicoGpioInitialize( Relay[i], OUTPUT_PUSH_PULL );
        user_relay_set( i, user_config->plug[i].on );
    }
    MicoSysLed( 0 );

    if ( user_config->version != USER_CONFIG_VERSION || user_config->plug[0].task[0].hour < 0 || user_config->plug[0].task[0].hour > 23 )
    {
        os_log( "WARNGIN: user params restored!" );
        err = mico_system_context_restore( sys_config );
        require_noerr( err, exit );
    }

    if ( sys_config->micoSystemConfig.name[0] == 1 )
    {
        IPStatusTypedef para;
        os_log( "micoWlanGetIPStatus:%d", micoWlanGetIPStatus( &para, Station ));   //mac读出来全部是0??!!!
        strcpy( strMac, para.mac );
        os_log( "result:%s",strMac );
        os_log( "result:%s",para.mac );


        unsigned char mac1, mac2;
        mac1 = strtohex( strMac[8], strMac[9] );
        mac2 = strtohex( strMac[10], strMac[11] );
        os_log( "strtohex:0x%02x%02x",mac1,mac2 );
        sprintf( sys_config->micoSystemConfig.name, ZTC1_NAME, mac1, mac2 );
    }

    os_log( "user:%s",user_config->user );
    os_log( "device name:%s",sys_config->micoSystemConfig.name );
    os_log( "mqtt_ip:%s",user_config->mqtt_ip );
    os_log( "mqtt_port:%d",user_config->mqtt_port );
    os_log( "mqtt_user:%s",user_config->mqtt_user );
    os_log( "mqtt_password:%s",user_config->mqtt_password );

    os_log( "version:%d",user_config->version );
//    for ( i = 0; i < PLUG_NUM; i++ )
//    {
//        os_log("plug_%d:",i);
//        os_log("\tname:%s:",user_config->plug[i].name);
//        for ( j = 0; j < PLUG_TIME_TASK_NUM; j++ )
//        {
//            os_log("\t\ton:%d\t %02d:%02d repeat:0x%X",user_config->plug[i].task[j].on,
//                user_config->plug[i].task[j].hour,user_config->plug[i].task[j].minute,
//                user_config->plug[i].task[j].repeat);
//        }
//    }

    wifi_init( );
    user_udp_init( );
    key_init( );
    err = user_mqtt_init( );
    require_noerr( err, exit );
    err = user_rtc_init( );
    require_noerr( err, exit );
    user_power_init();

    /* start http server thread */
//      app_httpd_start();
    while ( 1 )
    {
        main_num++;
        //发送功率数据
        if ( power_last != power || main_num>4 )
        {
            power_last = power;
            main_num =0;
            uint8_t *power_buf = NULL;
            power_buf = malloc( 128 );
            if ( power_buf != NULL )
            {
                sprintf( power_buf, "{\"mac\":\"%s\",\"power\":\"%d.%d\",\"total_time\":%d}", strMac, power / 10, power % 10, total_time );
                user_send( 0, power_buf );
                free( power_buf );
            }
            user_mqtt_hass_power( );
        }
        mico_thread_msleep(1000);

    }
    exit:
    os_log("application_start ERROR!");
    return 0;
}

