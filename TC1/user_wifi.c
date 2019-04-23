#include "user_wifi.h"

#include "main.h"
#include "mico_socket.h"
#include "user_gpio.h"
#include "user_sntp.h"

#define os_log(format, ...)  custom_log("WIFI", format, ##__VA_ARGS__)

char wifi_status = WIFI_STATE_NOCONNECT;

mico_timer_t wifi_led_timer;

static void wifi_connect_sys_config( void )
{
    if ( strlen( sys_config->micoSystemConfig.ssid ) > 0 )
    {
        os_log("connect ssid:%s key:%s",sys_config->micoSystemConfig.ssid,sys_config->micoSystemConfig.user_key);
        network_InitTypeDef_st wNetConfig;
        memset( &wNetConfig, 0, sizeof(network_InitTypeDef_st) );
        strcpy( wNetConfig.wifi_ssid, sys_config->micoSystemConfig.ssid );
        strcpy( wNetConfig.wifi_key, sys_config->micoSystemConfig.user_key );
        wNetConfig.wifi_mode = Station;
        wNetConfig.dhcpMode = DHCP_Client;
        wNetConfig.wifi_retry_interval = 6000;
        micoWlanStart( &wNetConfig );
        wifi_status = WIFI_STATE_CONNECTING;
    } else
        wifi_status = WIFI_STATE_FAIL;
}
void wifi_start_easylink( )
{
    wifi_status = WIFI_STATE_EASYLINK;
    micoWlanStartEasyLink( 20000 );
    user_led_set( 1 );
}

//easylink 完成回调
void wifi_easylink_completed_handle( network_InitTypeDef_st *nwkpara, void * arg )
{
    os_log("wifi_easylink_wps_completed_handle:");
    if ( nwkpara == NULL )
    {
        os_log("EasyLink fail");
        micoWlanStopEasyLink( );
        return;
    }

    os_log("ssid:\"%s\",\"%s\"",nwkpara->wifi_ssid,nwkpara->wifi_key);

    //保存wifi及密码
    strcpy( sys_config->micoSystemConfig.ssid, nwkpara->wifi_ssid );
    strcpy( sys_config->micoSystemConfig.user_key, nwkpara->wifi_key );
    sys_config->micoSystemConfig.user_keyLength = strlen( nwkpara->wifi_key );
    mico_system_context_update( sys_config );

    wifi_status = WIFI_STATE_NOCONNECT;
    os_log("EasyLink stop");
    micoWlanStopEasyLink( );
}

//wifi已连接获取到IP地址 回调
static void wifi_get_ip_callback( IPStatusTypedef *pnet, void * arg )
{
    os_log("got IP:%s", pnet->ip);
    wifi_status = WIFI_STATE_CONNECTED;
    user_function_cmd_received(1,"{\"cmd\":\"device report\"}");
}
//wifi连接状态改变回调
static void wifi_status_callback( WiFiEvent status, void *arg )
{
    if ( status == NOTIFY_STATION_UP ) //wifi连接成功
    {
        //wifi_status = WIFI_STATE_CONNECTED;
    } else if ( status == NOTIFY_STATION_DOWN ) //wifi断开
    {
        wifi_status = WIFI_STATE_NOCONNECT;
        if ( !mico_rtos_is_timer_running( &wifi_led_timer ) ) mico_rtos_start_timer( &wifi_led_timer );
    }
}
//100ms定时器回调
static void wifi_led_timer_callback( void* arg )
{
    static unsigned int num = 0;
    num++;

    switch ( wifi_status )
    {
        case WIFI_STATE_FAIL:
            os_log("wifi connect fail");
            user_led_set( 0 );
            mico_rtos_stop_timer( &wifi_led_timer );
            break;
        case WIFI_STATE_NOCONNECT:
            wifi_connect_sys_config( );
            break;

        case WIFI_STATE_CONNECTING:
            //if ( num > 1 )
        {
            num = 0;
            user_led_set( -1 );
        }
            break;
        case WIFI_STATE_NOEASYLINK:
            wifi_start_easylink( );
            break;
        case WIFI_STATE_EASYLINK:
            user_led_set( 1 );
            break;
        case WIFI_STATE_CONNECTED:
            user_led_set( 0 );
            mico_rtos_stop_timer( &wifi_led_timer );
            if ( relay_out( ) )
                user_led_set( 1 );
            else
                user_led_set( 0 );
            break;
    }
}

void wifi_init( void )
{
    //wifi配置初始化
//    network_InitTypeDef_st wNetConfig;

//    memset(&wNetConfig, 0, sizeof(network_InitTypeDef_st));
//    wNetConfig.wifi_mode = Station;
//    snprintf(wNetConfig.wifi_ssid, 32, "Honor 9" );
//    strcpy((char*)wNetConfig.wifi_key, "19910911");
//    wNetConfig.dhcpMode = DHCP_Client;
//    wNetConfig.wifi_retry_interval=6000;
//    micoWlanStart(&wNetConfig);

    //wifi状态下led闪烁定时器初始化
    mico_rtos_init_timer( &wifi_led_timer, 100, (void *) wifi_led_timer_callback, NULL );
    //easylink 完成回调
    mico_system_notify_register( mico_notify_EASYLINK_WPS_COMPLETED, (void *) wifi_easylink_completed_handle, NULL );
    //wifi已连接获取到IP地址 回调
    mico_system_notify_register( mico_notify_DHCP_COMPLETED, (void *) wifi_get_ip_callback, NULL );
    //wifi连接状态改变回调
    mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void*) wifi_status_callback, NULL );
    //sntp_init();
    //启动定时器开始进行wifi连接
    if ( !mico_rtos_is_timer_running( &wifi_led_timer ) ) mico_rtos_start_timer( &wifi_led_timer );

    IPStatusTypedef para;
    micoWlanGetIPStatus( &para, Station );
    strcpy( strMac, para.mac );

}

