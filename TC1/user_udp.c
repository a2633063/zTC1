#define os_log(format, ...)  custom_log("UDP", format, ##__VA_ARGS__)

#include "main.h"

#define LOCAL_UDP_PORT 10182


#define MAX_UDP_DATA_SIZE          (1024)
#define MAX_UDP_SEND_QUEUE_SIZE    (5)
mico_queue_t udp_msg_send_queue = NULL;

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
    char ip_address[16];
    uint8_t *buf = NULL;

//    /* create udp msg send queue */
//    err = mico_rtos_init_queue( &udp_msg_send_queue, "uqp_msg_send_queue", MAX_UDP_DATA_SIZE/*sizeof(p_mqtt_send_msg_t)*/,
//                                MAX_UDP_SEND_QUEUE_SIZE );
//    require_noerr_action( err, exit, os_log( "ERROR: create mqtt msg send queue err=%d.", err ) );

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
        FD_SET( udp_fd, &readfds );

        require_action( select( udp_fd + 1, &readfds, NULL, NULL, NULL ) >= 0, exit, err = kConnectionErr );

        /*Read data from udp and send data back */
        if ( FD_ISSET( udp_fd, &readfds ) )
        {
            len = recvfrom( udp_fd, buf, 1024, 0, (struct sockaddr *) &addr, &addrLen );
            require_action( len >= 0, exit, err = kConnectionErr );

//            os_log( "udp s_type:0x%x",((struct sockaddr )addr).s_type);
//            os_log( "udp s_port:0x%x",((struct sockaddr )addr).s_port);
//            os_log( "udp s_ip:0x%x",((struct sockaddr )addr).s_type);
//            os_log( "udp s_spares: %x %x %x %x %x %x",((struct sockaddr )addr).s_spares[0],((struct sockaddr )addr).s_spares[1],
//                    ((struct sockaddr )addr).s_spares[2],((struct sockaddr )addr).s_spares[3],
//                    ((struct sockaddr )addr).s_spares[4],((struct sockaddr )addr).s_spares[5],);

            strcpy( ip_address, inet_ntoa( addr.sin_addr ) );
//            os_log( "udp recv from %s:%d, len:%d :%s", ip_address,addr.sin_port, len ,buf);
            sendto( udp_fd, buf, len, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in) );
        }
    }

    exit:
    if ( err != kNoErr )
        os_log("UDP thread exit with err: %d", err);
    if ( buf != NULL ) free( buf );
    mico_rtos_delete_thread( NULL );
}


