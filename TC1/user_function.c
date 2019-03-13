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
        user_mqtt_send(  s );
}

void user_function_cmd_received( int udp_flag, uint8_t *pusrdata )
{
    OSStatus err = kNoErr;

    cJSON * pJsonRoot = cJSON_Parse( pusrdata );
    if ( !pJsonRoot )
    {
        os_log( "this is not a json data:\r\n%s\r\n", pusrdata );
        return;
    }

    //解析device report
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

    //解析
    cJSON *p_idx = cJSON_GetObjectItem( pJsonRoot, "idx" );
    cJSON *p_description = cJSON_GetObjectItem( pJsonRoot, "description" );
    cJSON *p_name = cJSON_GetObjectItem( pJsonRoot, "name" );
    cJSON *p_mac = cJSON_GetObjectItem( pJsonRoot, "mac" );

    //
    if ( (p_idx && cJSON_IsNumber( p_idx ) && p_idx->valueint == user_config->idx)  //idx
         || (p_description && cJSON_IsString( p_description ) && strcmp( p_description->valuestring, sys_config->micoSystemConfig.name ) == 0)   //p_description name
         || (p_name && cJSON_IsString( p_name ) && strcmp( p_name->valuestring, sys_config->micoSystemConfig.name ) == 0)    //name
         || (p_mac && cJSON_IsString( p_mac ) && strcmp( p_mac->valuestring, strMac ) == 0)   //mac
         )
    {
        //          os_log("device enter\r\n");
        cJSON *json_send = cJSON_CreateObject( );
        cJSON_AddStringToObject( json_send, "mac", strMac );

        cJSON *p_nvalue = cJSON_GetObjectItem( pJsonRoot, "nvalue" );
//        if ( p_nvalue && cJSON_IsNumber( p_nvalue ) )
//        {
////            uint32 now_time = system_get_time( );
////            os_log( "system_get_time:%d,%d = %09d\r\n", last_time, now_time, now_time - last_time );
////            if ( now_time - last_time < 1500000 && p_idx && p_nvalue->valueint == user_rudder_get_direction( ) )
////            {
////                return_flag = false;
////            } else
////            {
////                user_rudder_press( p_nvalue->valueint );
////            }
////            user_json_set_last_time( );
//        }
//
//        if ( p_nvalue )
//        {
//            cJSON_AddNumberToObject( json_send, "nvalue", user_rudder_get_direction( ) );
//        } else
//            last_time = 0;

        cJSON *p_setting = cJSON_GetObjectItem( pJsonRoot, "setting" );
        if ( p_setting )
        {
            cJSON *json_setting_send = cJSON_CreateObject( );
            //设置设备名称/deviceid
            cJSON *p_setting_name = cJSON_GetObjectItem( p_setting, "name" );
            if ( p_setting_name && cJSON_IsString( p_setting_name ) )
            {
                sprintf( sys_config->micoSystemConfig.name, p_setting_name->valuestring );
            }

            //设置mqtt ip
            cJSON *p_mqtt_ip = cJSON_GetObjectItem( p_setting, "mqtt_uri" );
            if ( p_mqtt_ip && cJSON_IsString( p_mqtt_ip ) )
            {
                sprintf( user_config->mqtt_ip, p_mqtt_ip->valuestring );
            }

            //设置mqtt port
            cJSON *p_mqtt_port = cJSON_GetObjectItem( p_setting, "mqtt_port" );
            if ( p_mqtt_port && cJSON_IsNumber( p_mqtt_port ) )
            {
                user_config->mqtt_port = p_mqtt_port->valueint;
            }

            //设置mqtt user
            cJSON *p_mqtt_user = cJSON_GetObjectItem( p_setting, "mqtt_user" );
            if ( p_mqtt_user && cJSON_IsString( p_mqtt_user ) )
            {
                sprintf( user_config->mqtt_user, p_mqtt_user->valuestring );
            }

            //设置mqtt password
            cJSON *p_mqtt_password = cJSON_GetObjectItem( p_setting, "mqtt_password" );
            if ( p_mqtt_password && cJSON_IsString( p_mqtt_password ) )
            {
                sprintf( user_config->mqtt_password, p_mqtt_password->valuestring );
            }

            //设置domoticz idx
            cJSON *p_setting_idx = cJSON_GetObjectItem( p_setting, "idx" );
            if ( p_setting_idx && cJSON_IsNumber( p_setting_idx ) )
            {
                user_config->idx = p_setting_idx->valueint;
                os_log( "idx:%d",user_config->idx );
                mico_system_context_update( sys_config );
            }

            if ( p_setting_name || p_mqtt_ip || p_mqtt_port || p_mqtt_user || p_mqtt_password || p_setting_idx )
            {
                os_log( "mico_system_context_update" );
                err = mico_system_context_update( sys_config );
                require_noerr( err, exit );
            }

            //开发返回数据
            //设置设备名称/deviceid
            if ( p_setting_name )
            {
                cJSON_AddStringToObject( json_setting_send, "name", sys_config->micoSystemConfig.name );
            }

            //设置mqtt ip
            if ( p_mqtt_ip )
            {
                cJSON_AddStringToObject( json_setting_send, "mqtt_uri", user_config->mqtt_ip );
            }

            //设置mqtt port
            if ( p_mqtt_port )
            {
                cJSON_AddNumberToObject( json_setting_send, "mqtt_port", user_config->mqtt_port );
            }

            //设置mqtt user
            if ( p_mqtt_user )
            {
                cJSON_AddStringToObject( json_setting_send, "mqtt_user", user_config->mqtt_user );
            }

            //设置mqtt password
            if ( p_mqtt_password )
            {
                cJSON_AddStringToObject( json_setting_send, "mqtt_password", user_config->mqtt_password );
            }

            //设置domoticz idx
            if ( p_setting_idx )
            {
                cJSON_AddNumberToObject( json_setting_send, "idx", user_config->idx );
            }

            cJSON_AddItemToObject( json_send, "setting", json_setting_send );
        }

        cJSON_AddStringToObject( json_send, "name", sys_config->micoSystemConfig.name );

        //if (p_idx)
        if ( user_config->idx >= 0 )
        cJSON_AddNumberToObject( json_send, "idx", user_config->idx );

        //if ( return_flag == true )
        {
            char *json_str = cJSON_Print( json_send );
            os_log( "pRoot: %s\r\n", json_str );
            user_send( udp_flag, json_str ); //发送数据
            free( (void *) json_str );
        }
        cJSON_Delete( json_send );
    }

    cJSON_Delete( pJsonRoot );
    return;
    exit:
    os_log("user_function_cmd_received ERROR:0x%x",err);

    cJSON_Delete( pJsonRoot );

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
