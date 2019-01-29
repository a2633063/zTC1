#define os_log(format, ...)  custom_log("RTC", format, ##__VA_ARGS__)

#include "main.h"
//#include "key.h"
#include "sntp.h"
#include "user_sntp.h"

void rtc_thread( mico_thread_arg_t arg );

OSStatus user_sntp_get_time( )
{
    OSStatus err = kNoErr;
    ntp_timestamp_t current_time;

    struct hostent * hostent_content = NULL;
    char ** pptr = NULL;
    struct in_addr ipp;

    mico_rtc_time_t rtc_time;

    hostent_content = gethostbyname( "pool.ntp.org" );
    pptr = hostent_content->h_addr_list;
    ipp.s_addr = *(uint32_t *) (*pptr);
    err = sntp_get_time( &ipp, &current_time );
    if ( err != kNoErr )
    {
        os_log("sntp_get_time1 err = %d.retry", err);
        ipp.s_addr = 0xd248912c;
        err = sntp_get_time( &ipp, &current_time );
    }
    if ( err != kNoErr )
    {
        os_log("sntp_get_time2 err = %d.", err);
    }

    if ( err == kNoErr )
    {
        mico_utc_time_ms_t utc_time_ms = (uint64_t) current_time.seconds * (uint64_t) 1000
                                         + (current_time.microseconds / 1000);
        mico_time_set_utc_time_ms( &utc_time_ms );
//        mico_utc_time_t utc_time = utc_time_ms / 1000 + 28800; //+8:00
//        struct tm * currentTime = localtime( (const time_t *) &utc_time );
//        rtc_time.sec = currentTime->tm_sec;
//        rtc_time.min = currentTime->tm_min;
//        rtc_time.hr = currentTime->tm_hour;
//        rtc_time.date = currentTime->tm_mday;
//        rtc_time.weekday = currentTime->tm_wday;
//        rtc_time.month = currentTime->tm_mon + 1;
//        rtc_time.year = (currentTime->tm_year + 1900) % 100;
//        MicoRtcSetTime( &rtc_time );
    }
    else
    {
        return err;
    }

}

OSStatus user_rtc_init( void )
{
    OSStatus err = kNoErr;

//    mico_rtc_time_t rtc_time;
//    rtc_time.sec = 0;
//    rtc_time.min = 0;
//    rtc_time.hr = 0;
//
//    rtc_time.date = 1;
//    rtc_time.weekday = 1;
//    rtc_time.month = 1;
//    rtc_time.year = 1;
//
//    MicoRtcSetTime( &rtc_time );

//    /* create mqtt msg send queue */
//    err = mico_rtos_init_queue( &mqtt_msg_send_queue, "mqtt_msg_send_queue", sizeof(p_mqtt_send_msg_t),
//                                MAX_MQTT_SEND_QUEUE_SIZE );
//    require_noerr_action( err, exit, app_log("ERROR: create mqtt msg send queue err=%d.", err) );

    /* start mqtt client */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "rtc",
                                   (mico_thread_function_t) rtc_thread,
                                   0x1000, 0 );
    require_noerr_string( err, exit, "ERROR: Unable to start the rtc thread." );

    if ( kNoErr != err ) os_log("ERROR, app thread exit err: %d", err);

    exit:
    return err;
}

void rtc_thread( mico_thread_arg_t arg )
{
    iso8601_time_t iso8601_time;
    OSStatus err = kUnknownErr;
    LinkStatusTypeDef LinkStatus;
    mico_rtc_time_t rtc_time;

    while ( 1 )
    {   //上电后连接了wifi才开始走时否则等待连接
        micoWlanGetLinkStatus( &LinkStatus );
        if ( LinkStatus.is_connected == 1 )
        {
            err = user_sntp_get_time( );
            if ( err == kNoErr )
            {
                rtc_init = 1;
                break;
            }
        }

        mico_rtos_thread_sleep( 3 );
    }

    while ( 1 )
    {
        if ( rtc_init == 0 || rtc_time.sec == 0 && rtc_time.min == 0 ) //开机及每小时校准一次
        {
            micoWlanGetLinkStatus( &LinkStatus );
            if ( LinkStatus.is_connected == 1 )
            {
                err = user_sntp_get_time( );
                if ( err == kNoErr ) rtc_init = 1;
            }
        }
        //        MicoRtcGetTime( &rtc_time );
//        os_log("time:20%02d/%02d/%02d %d %02d:%02d:%02d",rtc_time.year,rtc_time.month,rtc_time.date,rtc_time.weekday,rtc_time.hr,rtc_time.min,rtc_time.sec);

        os_log("rtc_thread");

        mico_utc_time_t utc_time;
        mico_rtc_time_t rtc_time;

        mico_time_get_utc_time( &utc_time );
        utc_time+= 28800;
        struct tm * currentTime = localtime( (const time_t *) &utc_time );
        rtc_time.sec = currentTime->tm_sec;
        rtc_time.min = currentTime->tm_min;
        rtc_time.hr = currentTime->tm_hour;

        rtc_time.date = currentTime->tm_mday;
        rtc_time.weekday = currentTime->tm_wday;
        rtc_time.month = currentTime->tm_mon + 1;
        rtc_time.year = (currentTime->tm_year + 1900) % 100;

//        MicoRtcSetTime( &rtc_time );      //MicoRtc不自动走时!
        os_log("time:20%02d/%02d/%02d %d %02d:%02d:%02d",rtc_time.year,rtc_time.month,rtc_time.date,rtc_time.weekday,rtc_time.hr,rtc_time.min,rtc_time.sec);

        mico_rtos_thread_msleep( 900 );
    }

    exit:
    os_log("EXIT: rtc exit with err = %d.", err);
    mico_rtos_delete_thread( NULL );
}

