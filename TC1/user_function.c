#define os_log(format, ...)  custom_log("FUNCTION", format, ##__VA_ARGS__)

#include "main.h"
#include "user_gpio.h"
#include "cJSON/cJSON.h"

uint32_t last_time = 0;

void user_function_set_last_time( )
{
    last_time = UpTicks( );
}

typedef struct _user_json_context_t
{
    int8_t idx;
    char name[maxNameLen];
    int8_t val;
} user_json_context_t;

bool json_plug_analysis( int udp_flag, char x, cJSON * pJsonRoot, cJSON * pJsonSend );
bool json_plug_task_analysis( char x, char y, cJSON * pJsonRoot, cJSON * pJsonSend );

void user_send( int udp_flag, char *s )
{
    if ( udp_flag || !user_mqtt_isconnect( ) )
        user_udp_send( s ); //发送数据
    else
        user_mqtt_send( s );
}

void user_function_cmd_received( int udp_flag, uint8_t *pusrdata )
{
    OSStatus err = kNoErr;
    char i;
    bool update_user_config_flag = false;   //标志位,记录最后是否需要更新储存的数据
    bool return_flag = true;    //为true时返回json结果,否则不返回

    cJSON * pJsonRoot = cJSON_Parse( pusrdata );
    if ( !pJsonRoot )
    {
        os_log( "this is not a json data:\r\n%s\r\n", pusrdata );
        return;
    }

    //解析UDP命令device report(MQTT同样回复命令)
    cJSON *p_cmd = cJSON_GetObjectItem( pJsonRoot, "cmd" );
    if ( p_cmd && cJSON_IsString( p_cmd ) && strcmp( p_cmd->valuestring, "device report" ) == 0 )
    {

        cJSON *pRoot = cJSON_CreateObject( );
        cJSON_AddStringToObject( pRoot, "name", sys_config->micoSystemConfig.name );
        cJSON_AddStringToObject( pRoot, "mac", strMac );
        cJSON_AddNumberToObject( pRoot, "type", TYPE );
        cJSON_AddStringToObject( pRoot, "type_name", TYPE_NAME );
        char *s = cJSON_Print( pRoot );
//        os_log( "pRoot: %s\r\n", s );
        user_send( udp_flag, s ); //发送数据
        free( (void *) s );
        cJSON_Delete( pRoot );
        //          cJSON_Delete(p_cmd);
    }

    //以下为解析命令部分
    cJSON *p_idx = cJSON_GetObjectItem( pJsonRoot, "idx" );
    cJSON *p_description = cJSON_GetObjectItem( pJsonRoot, "description" );
    cJSON *p_name = cJSON_GetObjectItem( pJsonRoot, "name" );
    cJSON *p_mac = cJSON_GetObjectItem( pJsonRoot, "mac" );

    //判断p_idx,使用"idx"作为主要字符串时
    for ( i = 0; i < PLUG_NUM; i++ )
    {
        //仅从插座的idx值判断 成立时不处理其他功能,仅改变插座状态
        if ( p_idx && cJSON_IsNumber( p_idx ) && p_idx->valueint >= 0 && p_idx->valueint == user_config->plug[i].idx )
        {
            cJSON *p_nvalue = cJSON_GetObjectItem( pJsonRoot, "nvalue" );
            if ( p_nvalue && cJSON_IsNumber( p_nvalue ) )
            {
                if ( p_nvalue->valueint != user_config->plug[i].on )
                {
                    user_relay_set( i, p_nvalue->valueint );
                    mico_system_context_update( sys_config ); //储存当前开关状态
                    cJSON *json_send = cJSON_CreateObject( );
                    cJSON_AddNumberToObject( json_send, "idx", user_config->plug[i].idx );
                    cJSON_AddNumberToObject( json_send, "nvalue", user_config->plug[i].on );
                    cJSON_AddStringToObject( json_send, "mac", strMac );

                    char strTemp1[] = "plug_X";
                    strTemp1[5] = i + '0';
                    cJSON *json_send_plug_on = cJSON_CreateObject( );
                    cJSON_AddNumberToObject( json_send_plug_on, "on", p_nvalue->valueint );

                    cJSON_AddItemToObject( json_send, strTemp1, json_send_plug_on );

                    char *json_str = cJSON_Print( json_send );
                    user_send( udp_flag, json_str ); //发送数据
                    free( json_str );
                    cJSON_Delete( json_send );
                }
            }
            cJSON_Delete( pJsonRoot );
            return;
        }
    }

    //开始正式处理所有命令
    if ( (p_idx && cJSON_IsNumber( p_idx ) && p_idx->valueint == user_config->idx)  //idx
         || (p_description && cJSON_IsString( p_description ) && strcmp( p_description->valuestring, sys_config->micoSystemConfig.name ) == 0)   //p_description name
         || (p_name && cJSON_IsString( p_name ) && strcmp( p_name->valuestring, sys_config->micoSystemConfig.name ) == 0)    //name
         || (p_mac && cJSON_IsString( p_mac ) && strcmp( p_mac->valuestring, strMac ) == 0)   //mac
         )
    {
        cJSON *json_send = cJSON_CreateObject( );
        cJSON_AddStringToObject( json_send, "mac", strMac );

        cJSON *p_nvalue = cJSON_GetObjectItem( pJsonRoot, "nvalue" );
        if ( p_nvalue )
        {
            if ( cJSON_IsNumber( p_nvalue ) )
            {
                uint32_t now_time = UpTicks( );
                os_log( "system_get_time:%d,%d = %09d\r\n", last_time, now_time, (now_time - last_time) );
                if ( now_time - last_time < 1000 && p_idx )
                {
                    return_flag = false;
                } else
                {
                    if ( p_nvalue->valueint != user_config->plug[i].on )
                    {
                        user_relay_set_all( p_nvalue->valueint );
                        update_user_config_flag = true;
                    }
                }
                user_function_set_last_time( );
            }
            cJSON_AddNumberToObject( json_send, "nvalue", p_nvalue->valueint );
        }

        //解析主机setting-----------------------------------------------------------------
        cJSON *p_setting = cJSON_GetObjectItem( pJsonRoot, "setting" );
        if ( p_setting )
        {
            cJSON *json_setting_send = cJSON_CreateObject( );
            //设置设备名称/deviceid
            cJSON *p_setting_name = cJSON_GetObjectItem( p_setting, "name" );
            if ( p_setting_name && cJSON_IsString( p_setting_name ) )
            {
                update_user_config_flag = true;
                sprintf( sys_config->micoSystemConfig.name, p_setting_name->valuestring );
            }

            //设置mqtt ip
            cJSON *p_mqtt_ip = cJSON_GetObjectItem( p_setting, "mqtt_uri" );
            if ( p_mqtt_ip && cJSON_IsString( p_mqtt_ip ) )
            {
                update_user_config_flag = true;
                sprintf( user_config->mqtt_ip, p_mqtt_ip->valuestring );
            }

            //设置mqtt port
            cJSON *p_mqtt_port = cJSON_GetObjectItem( p_setting, "mqtt_port" );
            if ( p_mqtt_port && cJSON_IsNumber( p_mqtt_port ) )
            {
                update_user_config_flag = true;
                user_config->mqtt_port = p_mqtt_port->valueint;
            }

            //设置mqtt user
            cJSON *p_mqtt_user = cJSON_GetObjectItem( p_setting, "mqtt_user" );
            if ( p_mqtt_user && cJSON_IsString( p_mqtt_user ) )
            {
                update_user_config_flag = true;
                sprintf( user_config->mqtt_user, p_mqtt_user->valuestring );
            }

            //设置mqtt password
            cJSON *p_mqtt_password = cJSON_GetObjectItem( p_setting, "mqtt_password" );
            if ( p_mqtt_password && cJSON_IsString( p_mqtt_password ) )
            {
                update_user_config_flag = true;
                sprintf( user_config->mqtt_password, p_mqtt_password->valuestring );
            }

            //设置domoticz idx
            cJSON *p_setting_idx = cJSON_GetObjectItem( p_setting, "idx" );
            if ( p_setting_idx && cJSON_IsNumber( p_setting_idx ) )
            {
                update_user_config_flag = true;
                user_config->idx = p_setting_idx->valueint;
                os_log( "idx:%d",user_config->idx );
                mico_system_context_update( sys_config );
            }

            //开发返回数据
            //返回设备名称/deviceid
            if ( p_setting_name ) cJSON_AddStringToObject( json_setting_send, "name", sys_config->micoSystemConfig.name );
            //返回mqtt ip
            if ( p_mqtt_ip ) cJSON_AddStringToObject( json_setting_send, "mqtt_uri", user_config->mqtt_ip );
            //返回mqtt port
            if ( p_mqtt_port ) cJSON_AddNumberToObject( json_setting_send, "mqtt_port", user_config->mqtt_port );
            //返回mqtt user
            if ( p_mqtt_user ) cJSON_AddStringToObject( json_setting_send, "mqtt_user", user_config->mqtt_user );
            //返回mqtt password
            if ( p_mqtt_password ) cJSON_AddStringToObject( json_setting_send, "mqtt_password", user_config->mqtt_password );
            //返回domoticz idx
            if ( p_setting_idx ) cJSON_AddNumberToObject( json_setting_send, "idx", user_config->idx );

            cJSON_AddItemToObject( json_send, "setting", json_setting_send );
        }

        //解析plug-----------------------------------------------------------------
        char plug_str[] = "plug_X";
        for ( i = 0; i < PLUG_NUM; i++ )
        {
            if ( json_plug_analysis( udp_flag, i, pJsonRoot, json_send ) )
                update_user_config_flag = true;
        }

        cJSON_AddStringToObject( json_send, "name", sys_config->micoSystemConfig.name );

        //if (p_idx)
        if ( user_config->idx >= 0 )
        cJSON_AddNumberToObject( json_send, "idx", user_config->idx );

        if ( return_flag == true )
        {
            char *json_str = cJSON_Print( json_send );
//            os_log( "pRoot: %s\r\n", json_str );
            user_send( udp_flag, json_str ); //发送数据
            free( (void *) json_str );
        }
        cJSON_Delete( json_send );
    }

    if ( update_user_config_flag )
    {
        mico_system_context_update( sys_config );
        update_user_config_flag = false;
    }

    cJSON_Delete( pJsonRoot );
    return;
    exit:
    os_log( "user_function_cmd_received ERROR:0x%x", err );

    cJSON_Delete( pJsonRoot );

}

/*
 *解析处理定时任务json
 *udp_flag:发送udp/mqtt标志位,此处修改插座开关状态时,需要实时更新给domoticz
 *x:插座编号
 */
bool json_plug_analysis( int udp_flag, char x, cJSON * pJsonRoot, cJSON * pJsonSend )
{
    if ( !pJsonRoot ) return false;
    if ( !pJsonSend ) return false;
    char i;
    bool return_flag = false;
    char plug_str[] = "plug_X";
    plug_str[5] = x + '0';

    cJSON *p_plug = cJSON_GetObjectItem( pJsonRoot, plug_str );
    if ( !p_plug ) return_flag = false;

    cJSON *json_plug_send = cJSON_CreateObject( );

    //解析plug on------------------------------------------------------
    if ( p_plug )
    {
        cJSON *p_plug_on = cJSON_GetObjectItem( p_plug, "on" );
        if ( p_plug_on )
        {
            if ( cJSON_IsNumber( p_plug_on ) )
            {
                user_relay_set( x, p_plug_on->valueint );
                return_flag = true;
                if ( user_config->plug[x].idx > 0 )
                {
                    cJSON *json_return_now = cJSON_CreateObject( );
                    cJSON_AddNumberToObject( json_return_now, "idx", user_config->plug[x].idx );
                    cJSON_AddNumberToObject( json_return_now, "nvalue", user_config->plug[x].on );
                    char *json_str = cJSON_Print( json_return_now );
                    user_send( udp_flag, json_str ); //发送数据
                    free( json_str );
                    cJSON_Delete( json_return_now );
                }
            }

        }

        //解析plug中setting项目----------------------------------------------
        cJSON *p_plug_setting = cJSON_GetObjectItem( p_plug, "setting" );
        if ( p_plug_setting )
        {
            cJSON *json_plug_setting_send = cJSON_CreateObject( );
            //解析plug中setting中name----------------------------------------
            cJSON *p_plug_setting_name = cJSON_GetObjectItem( p_plug_setting, "name" );
            if ( p_plug_setting_name )
            {
                if ( cJSON_IsString( p_plug_setting_name ) )
                {
                    return_flag = true;
                    sprintf( user_config->plug[x].name, p_plug_setting_name->valuestring );
                }
                cJSON_AddStringToObject( json_plug_setting_send, "name", user_config->plug[x].name );
            }

            //解析plug中setting中idx----------------------------------------
            cJSON *p_plug_setting_idx = cJSON_GetObjectItem( p_plug_setting, "idx" );
            if ( p_plug_setting_idx )
            {
                if ( cJSON_IsNumber( p_plug_setting_idx ) )
                {
                    return_flag = true;
                    user_config->plug[x].idx = p_plug_setting_idx->valueint;
                }
                cJSON_AddNumberToObject( json_plug_setting_send, "idx", user_config->plug[x].idx );
            }

            //解析plug中setting中task----------------------------------------
            for ( i = 0; i < PLUG_TIME_TASK_NUM; i++ )
            {
                if ( json_plug_task_analysis( x, i, p_plug_setting, json_plug_setting_send ) )
                    return_flag = true;
            }

            cJSON_AddItemToObject( json_plug_send, "setting", json_plug_setting_send );
        }
    }
    cJSON *p_nvalue = cJSON_GetObjectItem( pJsonRoot, "nvalue" );
//    if ( p_plug || p_nvalue )
        cJSON_AddNumberToObject( json_plug_send, "on", user_config->plug[x].on );

    cJSON_AddItemToObject( pJsonSend, plug_str, json_plug_send );
    return return_flag;
}

/*
 *解析处理定时任务json
 *x:插座编号 y:任务编号
 */
bool json_plug_task_analysis( char x, char y, cJSON * pJsonRoot, cJSON * pJsonSend )
{
    if ( !pJsonRoot ) return false;
    bool return_flag = false;

    char plug_task_str[] = "task_X";
    plug_task_str[5] = y + '0';

    cJSON *p_plug_task = cJSON_GetObjectItem( pJsonRoot, plug_task_str );
    if ( !p_plug_task ) return false;

    cJSON *json_plug_task_send = cJSON_CreateObject( );

    cJSON *p_plug_task_hour = cJSON_GetObjectItem( p_plug_task, "hour" );
    cJSON *p_plug_task_minute = cJSON_GetObjectItem( p_plug_task, "minute" );
    cJSON *p_plug_task_repeat = cJSON_GetObjectItem( p_plug_task, "repeat" );
    cJSON *p_plug_task_action = cJSON_GetObjectItem( p_plug_task, "action" );
    cJSON *p_plug_task_on = cJSON_GetObjectItem( p_plug_task, "on" );

    if ( p_plug_task_hour && p_plug_task_minute && p_plug_task_repeat &&
         p_plug_task_action
         && p_plug_task_on )
    {

        if ( cJSON_IsNumber( p_plug_task_hour )
             && cJSON_IsNumber( p_plug_task_minute )
             && cJSON_IsNumber( p_plug_task_repeat )
             && cJSON_IsNumber( p_plug_task_action )
             && cJSON_IsNumber( p_plug_task_on )
                                )
        {
            return_flag = true;
            user_config->plug[x].task[y].hour = p_plug_task_hour->valueint;
            user_config->plug[x].task[y].minute = p_plug_task_minute->valueint;
            user_config->plug[x].task[y].repeat = p_plug_task_repeat->valueint;
            user_config->plug[x].task[y].action = p_plug_task_action->valueint;
            user_config->plug[x].task[y].on = p_plug_task_on->valueint;
        }

    }
    cJSON_AddNumberToObject( json_plug_task_send, "hour", user_config->plug[x].task[y].hour );
    cJSON_AddNumberToObject( json_plug_task_send, "minute", user_config->plug[x].task[y].minute );
    cJSON_AddNumberToObject( json_plug_task_send, "repeat", user_config->plug[x].task[y].repeat );
    cJSON_AddNumberToObject( json_plug_task_send, "action", user_config->plug[x].task[y].action );
    cJSON_AddNumberToObject( json_plug_task_send, "on", user_config->plug[x].task[y].on );

    cJSON_AddItemToObject( pJsonSend, plug_task_str, json_plug_task_send );
    return return_flag;
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
