#define os_log(format, ...)  custom_log("OTA", format, ##__VA_ARGS__)

#include "mico.h"
#include "ota_server/ota_server.h"
#include "main.h"
#include "user_udp.h"
#include "user_mqtt_client.h"
#include "user_function.h"

static void ota_server_status_handler( OTA_STATE_E state, float progress )
{
    char str[64] = { 0 };
    switch ( state )
    {
        case OTA_LOADING:
            os_log("ota server is loading, progress %.2f%%", progress);
            if ( ((int) progress)%10 == 1 )
                sprintf( str, "{\"mac\":\"%s\",\"ota_progress\":%d}", strMac,((int) progress) );
            break;
        case OTA_SUCCE:
            os_log("ota server daemons success");
            sprintf( str, "{\"mac\":\"%s\",\"ota_progress\":100}", strMac );
            break;
        case OTA_FAIL:
            os_log("ota server daemons failed");
            sprintf( str, "{\"mac\":\"%s\",\"ota_progress\":-1}", strMac );
            break;
        default:
            break;
    }
    if ( str[0] > 0 )
    {
        user_send( true, str );
    }
}

void user_ota_start( char *url, char *md5 )
{
    os_log("ready to ota:%s",url);
    ota_server_start( url, md5, ota_server_status_handler );
}

