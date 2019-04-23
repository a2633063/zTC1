/**
 ******************************************************************************
 * @file    mqtt_client.c
 * @author  Eshen Wang
 * @version V1.0.0
 * @date    16-Nov-2015
 * @brief   MiCO application demonstrate a MQTT client.
 ******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
 ******************************************************************************
 */
#define app_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
#define mqtt_log(M, ...) custom_log("MQTT", M, ##__VA_ARGS__)

#include "main.h"
#include "mico.h"
#include "MQTTClient.h"
#include "user_function.h"
#include "user_gpio.h"
#include "user_mqtt_client.h"
#include "cJSON/cJSON.h"

//#define MQTT_CLIENT_SSL_ENABLE  // ssl

#define MAX_MQTT_TOPIC_SIZE         (256)
#define MAX_MQTT_DATA_SIZE          (1024)
#define MAX_MQTT_SEND_QUEUE_SIZE    (10)

#ifdef MQTT_CLIENT_SSL_ENABLE

#define MQTT_SERVER             "test.mosquitto.org"
#define MQTT_SERVER_PORT        8883
char* mqtt_server_ssl_cert_str =
"-----BEGIN CERTIFICATE-----\r\n\
MIIC8DCCAlmgAwIBAgIJAOD63PlXjJi8MA0GCSqGSIb3DQEBBQUAMIGQMQswCQYD\r\n\
VQQGEwJHQjEXMBUGA1UECAwOVW5pdGVkIEtpbmdkb20xDjAMBgNVBAcMBURlcmJ5\r\n\
MRIwEAYDVQQKDAlNb3NxdWl0dG8xCzAJBgNVBAsMAkNBMRYwFAYDVQQDDA1tb3Nx\r\n\
dWl0dG8ub3JnMR8wHQYJKoZIhvcNAQkBFhByb2dlckBhdGNob28ub3JnMB4XDTEy\r\n\
MDYyOTIyMTE1OVoXDTIyMDYyNzIyMTE1OVowgZAxCzAJBgNVBAYTAkdCMRcwFQYD\r\n\
VQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwGA1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1v\r\n\
c3F1aXR0bzELMAkGA1UECwwCQ0ExFjAUBgNVBAMMDW1vc3F1aXR0by5vcmcxHzAd\r\n\
BgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hvby5vcmcwgZ8wDQYJKoZIhvcNAQEBBQAD\r\n\
gY0AMIGJAoGBAMYkLmX7SqOT/jJCZoQ1NWdCrr/pq47m3xxyXcI+FLEmwbE3R9vM\r\n\
rE6sRbP2S89pfrCt7iuITXPKycpUcIU0mtcT1OqxGBV2lb6RaOT2gC5pxyGaFJ+h\r\n\
A+GIbdYKO3JprPxSBoRponZJvDGEZuM3N7p3S/lRoi7G5wG5mvUmaE5RAgMBAAGj\r\n\
UDBOMB0GA1UdDgQWBBTad2QneVztIPQzRRGj6ZHKqJTv5jAfBgNVHSMEGDAWgBTa\r\n\
d2QneVztIPQzRRGj6ZHKqJTv5jAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUA\r\n\
A4GBAAqw1rK4NlRUCUBLhEFUQasjP7xfFqlVbE2cRy0Rs4o3KS0JwzQVBwG85xge\r\n\
REyPOFdGdhBY2P1FNRy0MDr6xr+D2ZOwxs63dG1nnAnWZg7qwoLgpZ4fESPD3PkA\r\n\
1ZgKJc2zbSQ9fCPxt2W3mdVav66c6fsb7els2W2Iz7gERJSX\r\n\
-----END CERTIFICATE-----";

#else  // ! MQTT_CLIENT_SSL_ENABLE

#define MQTT_SERVER             user_config->mqtt_ip
#define MQTT_SERVER_PORT        user_config->mqtt_port

#endif // MQTT_CLIENT_SSL_ENABLE

typedef struct
{
    char topic[MAX_MQTT_TOPIC_SIZE];
    char qos;
    char retained;

    uint8_t data[MAX_MQTT_DATA_SIZE];
    uint32_t datalen;
} mqtt_recv_msg_t, *p_mqtt_recv_msg_t, mqtt_send_msg_t, *p_mqtt_send_msg_t;

static void mqtt_client_thread( mico_thread_arg_t arg );
static void messageArrived( MessageData* md );
static OSStatus mqtt_msg_publish( Client *c, const char* topic, char qos, char retained, const unsigned char* msg, uint32_t msg_len );

OSStatus user_recv_handler( void *arg );

OSStatus user_mqtt_send_plug_state( char plug_id );
void user_mqtt_hass_auto( char plug_id );
void user_mqtt_hass_auto_power( void );

bool isconnect = false;
mico_queue_t mqtt_msg_send_queue = NULL;

Client c;  // mqtt client object
Network n;  // socket network for mqtt client

static mico_worker_thread_t mqtt_client_worker_thread; /* Worker thread to manage send/recv events */
static mico_timed_event_t mqtt_client_send_event;

char topic_state[MAX_MQTT_TOPIC_SIZE];
char topic_set[MAX_MQTT_TOPIC_SIZE];

mico_timer_t timer_handle;
static uint8_t timer_status = 0;
void user_mqtt_timer_func( void *arg )
{
    uint8_t *buf1 = NULL;

    LinkStatusTypeDef LinkStatus;
    micoWlanGetLinkStatus( &LinkStatus );
    if ( LinkStatus.is_connected != 1 )
    {
        mico_stop_timer( &timer_handle );
        return;
    }
    if ( mico_rtos_is_queue_empty( &mqtt_msg_send_queue ) == true )
    {
        timer_status++;
        switch ( timer_status )
        {
            case 1:
                user_mqtt_hass_auto_power( );
                break;
            case 2:
                user_mqtt_hass_auto( 0 );
                break;
            case 3:
                user_mqtt_hass_auto( 1 );
                break;
            case 4:
                user_mqtt_hass_auto( 2 );
                break;
            case 5:
                user_mqtt_hass_auto( 3 );
                break;
            case 6:
                user_mqtt_hass_auto( 4 );
                break;
            case 7:
                user_mqtt_hass_auto( 5 );
                break;
            case 8:
                user_mqtt_hass_auto_name( 0 );
                break;
            case 9:
                user_mqtt_hass_auto_name( 1 );
                break;
            case 10:
                user_mqtt_hass_auto_name( 2 );
                break;
            case 11:
                user_mqtt_hass_auto_name( 3 );
                break;
            case 12:
                user_mqtt_hass_auto_name( 4 );
                break;
            case 13:
                user_mqtt_hass_auto_name( 5 );
                break;
            case 14:
                user_mqtt_hass_auto_power_name( );
                break;
            case 15:

                buf1 = malloc( 1024 ); //idx为1位时长度为24
                if ( buf1 != NULL )
                {
                    sprintf(
                        buf1,
                        "{\"mac\":\"%s\",\"version\":null,\"plug_0\":{\"on\":null,\"setting\":{\"name\":null}},\"plug_1\":{\"on\":null,\"setting\":{\"name\":null}},\"plug_2\":{\"on\":null,\"setting\":{\"name\":null}},\"plug_3\":{\"on\":null,\"setting\":{\"name\":null}},\"plug_4\":{\"on\":null,\"setting\":{\"name\":null}},\"plug_5\":{\"on\":null,\"setting\":{\"name\":null}}}",
                        strMac );
                    user_function_cmd_received( 0, buf1 );
                    free( buf1 );
                }
                break;
            default:
                mico_stop_timer( &timer_handle );
//            mico_deinit_timer( &timer_handle );
                break;
        }
    }
}

/* Application entrance */
OSStatus user_mqtt_init( void )
{
    OSStatus err = kNoErr;

    sprintf( topic_set, MQTT_CLIENT_SUB_TOPIC1 );
    sprintf( topic_state, MQTT_CLIENT_PUB_TOPIC, strMac );

#ifdef MQTT_CLIENT_SSL_ENABLE
    int mqtt_thread_stack_size = 0x3000;
#else
    //TODO size:0x800
    int mqtt_thread_stack_size = 0x2000;
#endif
    uint32_t mqtt_lib_version = MQTTClientLibVersion( );
    app_log( "MQTT client version: [%ld.%ld.%ld]",
        0xFF & (mqtt_lib_version >> 16), 0xFF & (mqtt_lib_version >> 8), 0xFF & mqtt_lib_version);

    /* create mqtt msg send queue */
    err = mico_rtos_init_queue( &mqtt_msg_send_queue, "mqtt_msg_send_queue", sizeof(p_mqtt_send_msg_t),
    MAX_MQTT_SEND_QUEUE_SIZE );
    require_noerr_action( err, exit, app_log("ERROR: create mqtt msg send queue err=%d.", err) );

    /* start mqtt client */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "mqtt_client",
                                   (mico_thread_function_t) mqtt_client_thread,
                                   mqtt_thread_stack_size, 0 );
    require_noerr_string( err, exit, "ERROR: Unable to start the mqtt client thread." );

    /* Create a worker thread for user handling MQTT data event  */
    err = mico_rtos_create_worker_thread( &mqtt_client_worker_thread, MICO_APPLICATION_PRIORITY, 0x800, 5 );
    require_noerr_string( err, exit, "ERROR: Unable to start the mqtt client worker thread." );

    exit:
    if ( kNoErr != err ) app_log("ERROR, app thread exit err: %d", err);
    return err;
}

static OSStatus mqtt_client_release( Client *c, Network *n )
{
    OSStatus err = kNoErr;

    if ( c->isconnected ) MQTTDisconnect( c );

    n->disconnect( n );  // close connection

    if ( MQTT_SUCCESS != MQTTClientDeinit( c ) )
    {
        app_log("MQTTClientDeinit failed!");
        err = kDeletedErr;
    }
    return err;
}

// publish msg to mqtt server
static OSStatus mqtt_msg_publish( Client *c, const char* topic, char qos, char retained,
                                  const unsigned char* msg,
                                  uint32_t msg_len )
{
    OSStatus err = kUnknownErr;
    int ret = 0;
    MQTTMessage publishData = MQTTMessage_publishData_initializer;

    require( topic && msg_len && msg, exit );

    // upload data qos0
    publishData.qos = (enum QoS) qos;
    publishData.retained = retained;
    publishData.payload = (void*) msg;
    publishData.payloadlen = msg_len;

    ret = MQTTPublish( c, topic, &publishData );

    if ( MQTT_SUCCESS == ret )
    {
        err = kNoErr;
    } else if ( MQTT_SOCKET_ERR == ret )
    {
        err = kConnectionErr;
    } else
    {
        err = kUnknownErr;
    }

    exit:
    return err;
}

void mqtt_client_thread( mico_thread_arg_t arg )
{
    OSStatus err = kUnknownErr;

    int i, rc = -1;
    fd_set readfds;
    struct timeval t = { 0, MQTT_YIELD_TMIE * 1000 };

    ssl_opts ssl_settings;
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

    p_mqtt_send_msg_t p_send_msg = NULL;
    int msg_send_event_fd = -1;
    bool no_mqtt_msg_exchange = true;

    mqtt_log("MQTT client thread started...");

    memset( &c, 0, sizeof(c) );
    memset( &n, 0, sizeof(n) );

    /* create msg send queue event fd */
    msg_send_event_fd = mico_create_event_fd( mqtt_msg_send_queue );
    require_action( msg_send_event_fd >= 0, exit, mqtt_log("ERROR: create msg send queue event fd failed!!!") );

    MQTT_start:

    isconnect = false;
    /* 1. create network connection */
#ifdef MQTT_CLIENT_SSL_ENABLE
    ssl_settings.ssl_enable = true;
    ssl_settings.ssl_debug_enable = false;  // ssl debug log
    ssl_settings.ssl_version = TLS_V1_2_MODE;
    ssl_settings.ca_str_len = strlen(mqtt_server_ssl_cert_str);
    ssl_settings.ca_str = mqtt_server_ssl_cert_str;
#else
    ssl_settings.ssl_enable = false;
#endif
    LinkStatusTypeDef LinkStatus;
    while ( 1 )
    {
        isconnect = false;
        mico_rtos_thread_sleep( 3 );
        if ( MQTT_SERVER[0] < 0x20 || MQTT_SERVER[0] > 0x7f || MQTT_SERVER_PORT < 1 ) continue;  //未配置mqtt服务器时不连接

        micoWlanGetLinkStatus( &LinkStatus );
        if ( LinkStatus.is_connected != 1 )
        {
            mqtt_log("ERROR:WIFI not connection , waiting 3s for connecting and then connecting MQTT ", rc);
            mico_rtos_thread_sleep( 3 );
            continue;
        }

        rc = NewNetwork( &n, MQTT_SERVER, MQTT_SERVER_PORT, ssl_settings );
        if ( rc == MQTT_SUCCESS ) break;
        mqtt_log("ERROR: MQTT network connection err=%d, reconnect after 3s...", rc);

    }
    mqtt_log("MQTT network connection success!");

    /* 2. init mqtt client */
    //c.heartbeat_retry_max = 2;
    rc = MQTTClientInit( &c, &n, MQTT_CMD_TIMEOUT );
    require_noerr_string( rc, MQTT_reconnect, "ERROR: MQTT client init err." );

    mqtt_log("MQTT client init success!");

    /* 3. create mqtt client connection */
    connectData.willFlag = 0;
    connectData.MQTTVersion = 4;  // 3: 3.1, 4: v3.1.1
    connectData.clientID.cstring = strMac;
    connectData.username.cstring = user_config->mqtt_user;
    connectData.password.cstring = user_config->mqtt_password;
    connectData.keepAliveInterval = MQTT_CLIENT_KEEPALIVE;
    connectData.cleansession = 1;

    rc = MQTTConnect( &c, &connectData );
    require_noerr_string( rc, MQTT_reconnect, "ERROR: MQTT client connect err." );

    mqtt_log("MQTT client connect success!");

    /* 4. mqtt client subscribe */
    rc = MQTTSubscribe( &c, topic_set, QOS0, messageArrived );
    require_noerr_string( rc, MQTT_reconnect, "ERROR: MQTT client subscribe err." );
    mqtt_log("MQTT client subscribe success! recv_topic=[%s].", topic_set);
    /*4.1 连接成功后先更新发送一次数据*/
    isconnect = true;

    mico_init_timer( &timer_handle, 150, user_mqtt_timer_func, &arg );
    mico_start_timer( &timer_handle );
    /* 5. client loop for recv msg && keepalive */
    while ( 1 )
    {
        isconnect = true;
        no_mqtt_msg_exchange = true;
        FD_ZERO( &readfds );
        FD_SET( c.ipstack->my_socket, &readfds );
        FD_SET( msg_send_event_fd, &readfds );
        select( msg_send_event_fd + 1, &readfds, NULL, NULL, &t );

        /* recv msg from server */
        if ( FD_ISSET( c.ipstack->my_socket, &readfds ) )
        {
            rc = MQTTYield( &c, (int) MQTT_YIELD_TMIE );
            require_noerr( rc, MQTT_reconnect );
            no_mqtt_msg_exchange = false;
        }

        /* recv msg from user worker thread to be sent to server */
        if ( FD_ISSET( msg_send_event_fd, &readfds ) )
        {
            while ( mico_rtos_is_queue_empty( &mqtt_msg_send_queue ) == false )
            {
                // get msg from send queue
                mico_rtos_pop_from_queue( &mqtt_msg_send_queue, &p_send_msg, 0 );
                require_string( p_send_msg, exit, "Wrong data point" );

                // send message to server
                err = mqtt_msg_publish( &c, p_send_msg->topic, p_send_msg->qos, p_send_msg->retained,
                                        p_send_msg->data,
                                        p_send_msg->datalen );

                require_noerr_string( err, MQTT_reconnect, "ERROR: MQTT publish data err" );

                mqtt_log("MQTT publish data success! send_topic=[%s], msg=[%ld].\r\n", p_send_msg->topic, p_send_msg->datalen);
                no_mqtt_msg_exchange = false;
                free( p_send_msg );
                p_send_msg = NULL;
            }
        }

        /* if no msg exchange, we need to check ping msg to keep alive. */
        if ( no_mqtt_msg_exchange )
        {
            rc = keepalive( &c );
            require_noerr_string( rc, MQTT_reconnect, "ERROR: keepalive err" );
        }
    }

    MQTT_reconnect:

    mqtt_log("Disconnect MQTT client, and reconnect after 5s, reason: mqtt_rc = %d, err = %d", rc, err );

    timer_status=100;

    mqtt_client_release( &c, &n );
    isconnect = false;
    user_led_set( -1 );
    mico_rtos_thread_msleep( 100 );
    user_led_set( -1 );
    mico_rtos_thread_sleep( 5 );
    goto MQTT_start;

    exit:
    isconnect = false;
    mqtt_log("EXIT: MQTT client exit with err = %d.", err);
    mqtt_client_release( &c, &n );
    mico_rtos_delete_thread( NULL );
}

// callback, msg received from mqtt server
static void messageArrived( MessageData* md )
{
    OSStatus err = kUnknownErr;
    p_mqtt_recv_msg_t p_recv_msg = NULL;
    MQTTMessage* message = md->message;

    p_recv_msg = (p_mqtt_recv_msg_t) calloc( 1, sizeof(mqtt_recv_msg_t) );
    require_action( p_recv_msg, exit, err = kNoMemoryErr );

    p_recv_msg->datalen = message->payloadlen;
    p_recv_msg->qos = (char) (message->qos);
    p_recv_msg->retained = message->retained;
    strncpy( p_recv_msg->topic, md->topicName->lenstring.data, md->topicName->lenstring.len );
    memcpy( p_recv_msg->data, message->payload, message->payloadlen );

    err = mico_rtos_send_asynchronous_event( &mqtt_client_worker_thread, user_recv_handler, p_recv_msg );
    require_noerr( err, exit );

    exit:
    if ( err != kNoErr )
    {
        app_log("ERROR: Recv data err = %d", err);
        if ( p_recv_msg ) free( p_recv_msg );
    }
    return;
}

/* Application process MQTT received data */
OSStatus user_recv_handler( void *arg )
{
    OSStatus err = kUnknownErr;
    p_mqtt_recv_msg_t p_recv_msg = arg;
    require( p_recv_msg, exit );

    app_log("user get data success! from_topic=[%s], msg=[%ld].\r\n", p_recv_msg->topic, p_recv_msg->datalen);
    user_function_cmd_received( 0, p_recv_msg->data );
    free( p_recv_msg );

    exit:
    return err;
}

OSStatus user_mqtt_send_topic( char *topic, char *arg, char retained )
{
    OSStatus err = kUnknownErr;
    p_mqtt_send_msg_t p_send_msg = NULL;

//    app_log("======App prepare to send ![%d]======", MicoGetMemoryInfo()->free_memory);

    /* Send queue is full, pop the oldest */
    if ( mico_rtos_is_queue_full( &mqtt_msg_send_queue ) == true )
    {
        mico_rtos_pop_from_queue( &mqtt_msg_send_queue, &p_send_msg, 0 );
        free( p_send_msg );
        p_send_msg = NULL;
    }

    /* Push the latest data into send queue*/
    p_send_msg = (p_mqtt_send_msg_t) calloc( 1, sizeof(mqtt_send_msg_t) );
    require_action( p_send_msg, exit, err = kNoMemoryErr );

    p_send_msg->qos = 0;
    p_send_msg->retained = retained;
    p_send_msg->datalen = strlen( arg );
    memcpy( p_send_msg->data, arg, p_send_msg->datalen );
    strncpy( p_send_msg->topic, topic, MAX_MQTT_TOPIC_SIZE );

    err = mico_rtos_push_to_queue( &mqtt_msg_send_queue, &p_send_msg, 0 );
    require_noerr( err, exit );

    //app_log("Push user msg into send queue success!");

    exit:
    if ( err != kNoErr && p_send_msg ) free( p_send_msg );
    return err;
}

/* Application collect data and seng them to MQTT send queue */
OSStatus user_mqtt_send( char *arg )
{
    return user_mqtt_send_topic( topic_state, arg, 0 );
}

//更新ha开关状态
OSStatus user_mqtt_send_plug_state( char plug_id )
{

    uint8_t *send_buf = NULL;
    uint8_t *topic_buf = NULL;
    send_buf = malloc( 64 ); //
    topic_buf = malloc( 64 ); //
    if ( send_buf != NULL && topic_buf != NULL )
    {
        sprintf( topic_buf, "homeassistant/switch/%s/plug_%d/state", strMac, plug_id );
        sprintf( send_buf, "{\"mac\":\"%s\",\"plug_%d\":{\"on\":%d}}", strMac, plug_id, user_config->plug[plug_id].on );
        user_mqtt_send_topic( topic_buf, send_buf, 1 );
    }
    if ( send_buf ) free( send_buf );
    if ( topic_buf ) free( topic_buf );
}

//hass mqtt自动发现数据开关发送
void user_mqtt_hass_auto( char plug_id )
{
    uint8_t i;
    uint8_t *send_buf = NULL;
    uint8_t *topic_buf = NULL;
    send_buf = malloc( 512 ); //
    topic_buf = malloc( 128 ); //
    if ( send_buf != NULL && topic_buf != NULL )
    {
        sprintf( topic_buf, "homeassistant/switch/%s/plug_%d/config", strMac, plug_id );
        sprintf( send_buf, "{"
                 "\"name\":\"zTC1_plug%d_%s\","
                 "\"stat_t\":\"homeassistant/switch/%s/plug_%d/state\","
                 "\"cmd_t\":\"device/ztc1/set\","
                 "\"pl_on\":\"{\\\"mac\\\":\\\"%s\\\",\\\"plug_%d\\\":{\\\"on\\\":1}}\","
                 "\"pl_off\":\"{\\\"mac\\\":\\\"%s\\\",\\\"plug_%d\\\":{\\\"on\\\":0}}\""
                 "}\0",
                 plug_id, strMac + 8, strMac, plug_id, strMac, plug_id, strMac, plug_id );
        user_mqtt_send_topic( topic_buf, send_buf, 1 );
    }
    if ( send_buf ) free( send_buf );
    if ( topic_buf ) free( topic_buf );

}
void user_mqtt_hass_auto_name( char plug_id )
{
    uint8_t *send_buf = NULL;
    uint8_t *topic_buf = NULL;
    send_buf = (uint8_t *) malloc( 300 );
    topic_buf = (uint8_t *) malloc( 64 );
    if ( send_buf != NULL && topic_buf != NULL )
    {
        sprintf( topic_buf, "homeassistant/switch/%s/plug_%d/config", strMac, plug_id );
        sprintf( send_buf, "{"
                 "\"name\":\"%s\","
                 "\"stat_t\":\"homeassistant/switch/%s/plug_%d/state\","
                 "\"cmd_t\":\"device/ztc1/set\","
                 "\"pl_on\":\"{\\\"mac\\\":\\\"%s\\\",\\\"plug_%d\\\":{\\\"on\\\":1}}\","
                 "\"pl_off\":\"{\\\"mac\\\":\\\"%s\\\",\\\"plug_%d\\\":{\\\"on\\\":0}}\""
                 "}\0",
                 user_config->plug[plug_id].name, strMac, plug_id, strMac, plug_id, strMac, plug_id );
        user_mqtt_send_topic( topic_buf, send_buf, 0 );
    }
    if ( send_buf )
        free( send_buf );
    if ( topic_buf )
        free( topic_buf );
}
//hass mqtt自动发现数据功率发送
void user_mqtt_hass_auto_power( void )
{
    uint8_t i;
    uint8_t *send_buf = NULL;
    uint8_t *topic_buf = NULL;
    send_buf = malloc( 512 ); //
    topic_buf = malloc( 128 ); //
    if ( send_buf != NULL && topic_buf != NULL )
    {
        sprintf( topic_buf, "homeassistant/sensor/%s/power/config", strMac );
        sprintf( send_buf, "{"
                 "\"name\":\"zTC1_power_%s\","
                 "\"state_topic\":\"homeassistant/sensor/%s/power/state\","
                 "\"unit_of_measurement\":\"W\","
                 "\"icon\":\"mdi:gauge\","
                 "\"value_template\":\"{{ value_json.power }}\""
                 "}",
                 strMac + 8, strMac );

        user_mqtt_send_topic( topic_buf, send_buf, 1 );
    }
    if ( send_buf ) free( send_buf );
    if ( topic_buf ) free( topic_buf );
}
void user_mqtt_hass_auto_power_name( void )
{
    uint8_t *send_buf = NULL;
    uint8_t *topic_buf = NULL;
    send_buf = (uint8_t *) malloc( 300 ); //
    topic_buf = (uint8_t *) malloc( 64 ); //
    if ( send_buf != NULL && topic_buf != NULL )
    {
        sprintf( topic_buf, "homeassistant/sensor/%s/power/config", strMac );
        sprintf( send_buf, "{"
                 "\"name\":\"zTC1xxxxxx\","
                 "\"state_topic\":\"homeassistant/sensor/%s/power/state\","
                 "\"unit_of_measurement\":\"W\","
                 "\"icon\":\"mdi:gauge\","
                 "\"value_template\":\"{{ value_json.power }}\""
                 "}",
                 strMac );
        send_buf[13] = 0xe5;
        send_buf[14] = 0x8a;
        send_buf[15] = 0x9f;
        send_buf[16] = 0xe7;
        send_buf[17] = 0x8e;
        send_buf[18] = 0x87;
        user_mqtt_send_topic( topic_buf, send_buf, 0 );
    }
    if ( send_buf )
        free( send_buf );
    if ( topic_buf )
        free( topic_buf );
}

void user_mqtt_hass_power( void )
{
    uint8_t i;
    uint8_t *send_buf = NULL;
    uint8_t *topic_buf = NULL;
    send_buf = malloc( 512 ); //
    topic_buf = malloc( 128 ); //
    if ( send_buf != NULL && topic_buf != NULL )
    {
        sprintf( topic_buf, "homeassistant/sensor/%s/power/state", strMac );
        sprintf( send_buf, "{\"power\":\"%d.%d\"}", power / 10, power % 10 );
        user_mqtt_send_topic( topic_buf, send_buf, 0 );
    }
    if ( send_buf ) free( send_buf );
    if ( topic_buf ) free( topic_buf );
}

bool user_mqtt_isconnect( )
{
    return isconnect;
}
