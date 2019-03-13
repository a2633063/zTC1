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

void user_function_cmd_received(struct sockaddr *addr, uint8_t *pusrdata )
{
    OSStatus err = kNoErr;
    char *out;
    cJSON * pJsonRoot = cJSON_Parse( pusrdata );
    if ( !pJsonRoot )
    {
        os_log( "this is not a json data:\r\n%s\r\n", pusrdata );
        return;
    }

//    //解析device report
//    os_printf( "start json:device report\r\n" );
//    cJSON *p_cmd = cJSON_GetObjectItem( pJsonRoot, "cmd" );
//    if ( p_cmd && cJSON_IsString( p_cmd ) && os_strcmp( p_cmd->valuestring, "device report" ) == 0 )
//    {
//
//        cJSON *pRoot = cJSON_CreateObject( );
//        cJSON_AddStringToObject( pRoot, "name", mqtt_device_id );
//        cJSON_AddStringToObject( pRoot, "mac", strMac );
//        cJSON_AddNumberToObject( pRoot, "type", TYPE );
//        cJSON_AddStringToObject( pRoot, "type_name", TYPE_NAME );
//        char *s = cJSON_Print( pRoot );
//        os_printf( "pRoot: %s\r\n", s );
//
//        if ( addr )
//        {
//            if ( addr && pesp_conn->type == ESPCONN_UDP )
//            {
//                pesp_conn->type = ESPCONN_UDP;
//                pesp_conn->proto.udp->remote_port = 10181;  //获取端口
//                pesp_conn->proto.udp->remote_ip[0] = 255;   //获取IP地址
//                pesp_conn->proto.udp->remote_ip[1] = 255;
//                pesp_conn->proto.udp->remote_ip[2] = 255;
//                pesp_conn->proto.udp->remote_ip[3] = 255;
//            }
//            espconn_send( pesp_conn, s, os_strlen( s ) );   //发送数据
//        } else
//        {
//            user_mqtt_send( "domoticz/in", s );
//        }
//        cJSON_free( (void *) s );
//        cJSON_Delete( pRoot );
//        //          cJSON_Delete(p_cmd);
//    }

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

    /*
     if (
     (p_idx && cJSON_IsNumber( p_idx ) && p_idx->valueint == 2)  //idx
     || (p_description && cJSON_IsString( p_description ) && strcmp( p_description->valuestring, "123" ) == 0)   //description mqttid
     || (p_name && cJSON_IsString( p_name ) && strcmp( p_name->valuestring, sys_config->micoSystemConfig.name ) == 0)    //name
     )
     {
     cJSON *p_nvalue = cJSON_GetObjectItem( pJsonRoot, "nvalue" );
     if ( p_nvalue ) led( p_nvalue->valueint );
     }
     */
    cJSON_Delete( pJsonRoot );
    free( out );

}
