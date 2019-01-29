
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

void user_function_cmd_received( uint8_t *pusrdata )
{
    OSStatus err = kNoErr;
    char *out;
    cJSON * pJsonRoot = cJSON_Parse( pusrdata );
    if ( !pJsonRoot )
    {
        return;
    }

    //½âÎöidx×Ö¶ÎintÄÚÈÝ   description×Ö¶ÎstringÄÚÈÝ     name×Ö¶ÎstringÄÚÈÝ
    cJSON *p_idx = cJSON_GetObjectItem( pJsonRoot, "idx" );
    cJSON *p_description = cJSON_GetObjectItem( pJsonRoot, "description" );
    cJSON *p_name = cJSON_GetObjectItem( pJsonRoot, "name" );

    if ( p_idx && cJSON_IsNumber( p_idx ) && (p_idx->valueint>=3 &&p_idx->valueint<=9) )  //idx
    {
        cJSON *p_nvalue = cJSON_GetObjectItem( pJsonRoot, "nvalue" );
        if ( p_nvalue )
        {
            user_led_set( p_nvalue->valueint );
            user_relay_set(p_idx->valueint-3,p_nvalue->valueint);
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
