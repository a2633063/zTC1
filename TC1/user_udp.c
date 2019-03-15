#define os_log(format, ...)  custom_log("UDP", format, ##__VA_ARGS__)

#include "main.h"
#include "user_function.h"

#define LOCAL_UDP_PORT 10182
#define REMOTE_UDP_PORT 10181

#define MAX_UDP_DATA_SIZE          (1024)
#define MAX_UDP_SEND_QUEUE_SIZE    (5)
mico_queue_t udp_msg_send_queue = NULL;

typedef struct
{
    uint32_t datalen;
    uint8_t data[MAX_UDP_DATA_SIZE];
} udp_send_msg_t, *p_udp_send_msg_t;

static OSStatus udp_msg_send( int socket, const unsigned char* msg, uint32_t msg_len );
void udp_thread( void *arg );

OSStatus user_udp_init( void )
{
    OSStatus err = kNoErr;
    /* start udp client */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "udp",
                                   (mico_thread_function_t) udp_thread,
                                   0x1000, 0 );
    require_noerr_string( err, exit, "ERROR: Unable to start the rtc thread." );

    if ( kNoErr != err ) os_log("ERROR, app thread exit err: %d", err);

    exit:
    return err;

}

/*create udp socket*/
void udp_thread( void *arg )
{
    UNUSED_PARAMETER( arg );

    OSStatus err;
    struct sockaddr_in addr;
    fd_set readfds;

    socklen_t addrLen = sizeof(addr);
    int udp_fd = -1, len;
    p_udp_send_msg_t p_send_msg = NULL;
    int msg_send_event_fd = -1;
    char ip_address[16];
    uint8_t *buf = NULL;

    /* create udp msg send queue */
    err = mico_rtos_init_queue( &udp_msg_send_queue, "uqp_msg_send_queue", sizeof(p_udp_send_msg_t),
    MAX_UDP_SEND_QUEUE_SIZE );
    require_noerr_action( err, exit, os_log( "ERROR: create udp msg send queue err=%d.", err ) );
    /* create msg send queue event fd */
    msg_send_event_fd = mico_create_event_fd( udp_msg_send_queue );
    require_action( msg_send_event_fd >= 0, exit, os_log( "ERROR: create msg send queue event fd failed!!!" ) );

    buf = malloc( 1024 );
    require_action( buf, exit, err = kNoMemoryErr );

    /*Establish a UDP port to receive any data sent to this port*/
    udp_fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    require_action( IsValidSocket( udp_fd ), exit, err = kNoResourcesErr );

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons( LOCAL_UDP_PORT );
    err = bind( udp_fd, (struct sockaddr *) &addr, sizeof(addr) );
    require_noerr( err, exit );

    os_log("Open local UDP port %d", LOCAL_UDP_PORT);

    while ( 1 )
    {
        FD_ZERO( &readfds );
        FD_SET( msg_send_event_fd, &readfds );
        FD_SET( udp_fd, &readfds );
        select( udp_fd + 1, &readfds, NULL, NULL, NULL );

        /*Read data from udp and send data back */
        if ( FD_ISSET( udp_fd, &readfds ) )
        {
            len = recvfrom( udp_fd, buf, 1024, 0, (struct sockaddr *) &addr, &addrLen );
            require_action( len >= 0, exit, err = kConnectionErr );

            strcpy( ip_address, inet_ntoa( addr.sin_addr ) );
            if(len<1024) buf[len]=0;
            os_log( "udp recv from %s:%d, len:%d ", ip_address,addr.sin_port, len );
            user_function_cmd_received(1,buf);
//            sendto( udp_fd, buf, len, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in) );
        }

        /* recv msg from user worker thread to be sent to server */
        if ( FD_ISSET( msg_send_event_fd, &readfds ) )
        {
            while ( mico_rtos_is_queue_empty( &udp_msg_send_queue ) == false )
            {
                // get msg from send queue
                mico_rtos_pop_from_queue( &udp_msg_send_queue, &p_send_msg, 0 );
                require_string( p_send_msg, exit, "Wrong data point" );

                // send message to server
                err = udp_msg_send( udp_fd, p_send_msg->data, p_send_msg->datalen );
//                require_noerr_string( err, MQTT_reconnect, "ERROR: udp publish data err" );

                os_log( "udp send data success! msg=[%ld].\r\n", p_send_msg->datalen);
                free( p_send_msg );
                p_send_msg = NULL;
            }
        }
    }

    exit:
    if ( err != kNoErr )
        os_log("UDP thread exit with err: %d", err);
    if ( buf != NULL ) free( buf );
    mico_rtos_delete_thread( NULL );
}

// send msg to udp
static OSStatus udp_msg_send( int socket, const unsigned char* msg, uint32_t msg_len )
{
    OSStatus err = kUnknownErr;
    int ret = 0;

    require( msg_len && msg, exit );

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons( LOCAL_UDP_PORT );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_BROADCAST;
    addr.sin_port = htons( REMOTE_UDP_PORT );
    /*the receiver should bind at port=20000*/
    sendto( socket, msg, msg_len, 0, (struct sockaddr *) &addr, sizeof(addr) );

    exit:
    return err;
}

/* Application collect data and seng them to udp send queue */
OSStatus user_udp_send( char *arg )
{
    OSStatus err = kUnknownErr;
    p_udp_send_msg_t p_send_msg = NULL;

//    app_log("======App prepare to send ![%d]======", MicoGetMemoryInfo()->free_memory);

    /* Send queue is full, pop the oldest */
    if ( mico_rtos_is_queue_full( &udp_msg_send_queue ) == true )
    {
        mico_rtos_pop_from_queue( &udp_msg_send_queue, &p_send_msg, 0 );
        free( p_send_msg );
        p_send_msg = NULL;
    }

    /* Push the latest data into send queue*/
    p_send_msg = calloc( 1, sizeof(udp_send_msg_t) );
    require_action( p_send_msg, exit, err = kNoMemoryErr );

    p_send_msg->datalen = strlen( arg );
    memcpy( p_send_msg->data, arg, p_send_msg->datalen );

    err = mico_rtos_push_to_queue( &udp_msg_send_queue, &p_send_msg, 0 );
    require_noerr( err, exit );

    //app_log("Push user msg into send queue success!");

    exit:
    if ( err != kNoErr && p_send_msg ) free( p_send_msg );
    return err;

}
