#define os_log(format, ...)  custom_log("FUNCTION", format, ##__VA_ARGS__)

#include "main.h"
#include "user_gpio.h"
#include "cJSON/cJSON.h"

typedef struct _user_json_context_t
{
    int8_t idx;
    char name[maxNameLen];

    int8_t val;
} user_json_context_t;

void user_send( int udp_flag, uint8_t *s )
{
    if ( udp_flag )
        user_udp_send( s ); //发送数据
    else
        user_mqtt_send( "domoticz/in", s );
}

void user_function_cmd_received( int udp_flag, uint8_t *pusrdata )
{
    OSStatus err = kNoErr;
    char *out;
    cJSON * pJsonRoot = cJSON_Parse( pusrdata );
    if ( !pJsonRoot )
    {
        os_log( "this is not a json data:\r\n%s\r\n", pusrdata );
        return;
    }

    //解析device report
    os_log( "start json:device report\r\n" );
    cJSON *p_cmd = cJSON_GetObjectItem( pJsonRoot, "cmd" );
    if ( p_cmd && cJSON_IsString( p_cmd ) && strcmp( p_cmd->valuestring, "device report" ) == 0 )
    {

        cJSON *pRoot = cJSON_CreateObject( );
        cJSON_AddStringToObject( pRoot, "name", sys_config->micoSystemConfig.name );
        cJSON_AddStringToObject( pRoot, "mac", strMac );
        cJSON_AddNumberToObject( pRoot, "type", TYPE );
        cJSON_AddStringToObject( pRoot, "type_name", TYPE_NAME );
        char *s = cJSON_Print( pRoot );
        os_log( "pRoot: %s\r\n", s );

        user_send( udp_flag, s ); //发送数据
        free( (void *) s );
        cJSON_Delete( pRoot );
        //          cJSON_Delete(p_cmd);
    }

    //解析idx字段int内容   description字段string内容     name字段string内容
    cJSON *p_idx = cJSON_GetObjectItem( pJsonRoot, "idx" );
    cJSON *p_description = cJSON_GetObjectItem( pJsonRoot, "description" );
    cJSON *p_name = cJSON_GetObjectItem( pJsonRoot, "name" );

    if ( p_idx && cJSON_IsNumber( p_idx ) && (p_idx->valueint >= 3 && p_idx->valueint <= 9) )  //idx
    {
        cJSON *p_nvalue = cJSON_GetObjectItem( pJsonRoot, "nvalue" );
        if ( p_nvalue )
        {
            user_led_set( p_nvalue->valueint );
            user_relay_set( p_idx->valueint - 3, p_nvalue->valueint );
        }
    }

    cJSON_Delete( pJsonRoot );
    free( out );

}

unsigned char strtohex( char a, char b )
{
    if ( a >= 0x30 && a <= 0x39 )
        a -= 0x30;
    else if ( a >= 0x41 && a <= 0x46 )
    {
        a = a + 10 - 0x41;
    } else if ( a >= 0x61 && a <= 0x66 )
    {
        a = a + 10 - 0x61;
    }

    if ( b >= 0x30 && b <= 0x39 )
        b -= 0x30;
    else if ( b >= 0x41 && b <= 0x46 )
    {
        b = b + 10 - 0x41;
    } else if ( b >= 0x61 && b <= 0x66 )
    {
        b = b + 10 - 0x61;
    }

    return a * 16 + b;
}
