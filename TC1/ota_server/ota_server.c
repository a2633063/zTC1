/**
 ******************************************************************************
 * @file    ota_server.c
 * @author  QQ ding
 * @version V1.0.0
 * @date    19-Oct-2016
 * @brief   Create a OTA server thread, download update bin file. Reboot system
 *          if download success.
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */
#include "mico.h"
#include "HTTPUtils.h"
#include "SocketUtils.h"
#include "ota_server.h"
#include "url.h"

#if OTA_DEBUG
#define ota_server_log(M, ...) custom_log("OTA", M, ##__VA_ARGS__)
#else
#define ota_server_log(M, ...)
#endif

static ota_server_context_t *ota_server_context = NULL;
static HTTPHeader_t *httpHeader = NULL;

static CRC16_Context crc_context;
static md5_context md5;
static uint32_t offset = 0;

static OSStatus onReceivedData( struct _HTTPHeader_t * httpHeader,
                                uint32_t pos,
                                uint8_t *data,
                                size_t len,
                                void * userContext );

static void hex2str(uint8_t *hex, int hex_len, char *str)
{
  int i = 0;
  for(i=0; i<hex_len; i++){
    sprintf(str+i*2, "%02x", hex[i]);
  }
}

static void upper2lower(char *str, int len)
{
  int i = 0;
  for(i=0; i<len; i++)
  {
    if( (*(str+i) >= 'A') &&  (*(str+i) <= 'Z') ){
      *(str+i) += 32;
    }
  }
}

static int ota_server_send( uint8_t *data, int datalen )
{
    int res = 0;
    if( ota_server_context->download_url.HTTP_SECURITY == HTTP_SECURITY_HTTP ){
        res = send( ota_server_context->download_url.ota_fd, data, datalen, 0 );
    }
#if OTA_USE_HTTPS
    if( ota_server_context->download_url.HTTP_SECURITY == HTTP_SECURITY_HTTPS ){
        res = ssl_send( ota_server_context->download_url.ota_ssl, data, datalen);
    }
#endif
    return res;
}

static OSStatus ota_server_connect( struct sockaddr_in *addr, socklen_t addrlen )
{
    OSStatus err = kNoErr;
#if OTA_USE_HTTPS
    int ssl_errno = 0;
#endif

    err = connect( ota_server_context->download_url.ota_fd, (struct sockaddr *)addr, addrlen );
    require_noerr_string( err, exit, "ERROR: connect ota server failed" );

#if OTA_USE_HTTPS
    if( ota_server_context->download_url.HTTP_SECURITY == HTTP_SECURITY_HTTPS ){
        ota_server_context->download_url.ota_ssl = ssl_connect( ota_server_context->download_url.ota_fd, 0, NULL, &ssl_errno );
        require_action_string( ota_server_context->download_url.ota_ssl != NULL, exit, err = kConnectionErr,"ERROR: ssl disconnect" );
    }
#endif

exit:
    return err;
}

static int ota_server_read_header( HTTPHeader_t *httpHeader )
{
    int res = 0;
    if( ota_server_context->download_url.HTTP_SECURITY == HTTP_SECURITY_HTTP ){
        res = SocketReadHTTPHeader( ota_server_context->download_url.ota_fd, httpHeader );
    }
    if( ota_server_context->download_url.HTTP_SECURITY == HTTP_SECURITY_HTTPS ){
#if OTA_USE_HTTPS
        res = SocketReadHTTPSHeader( ota_server_context->download_url.ota_ssl, httpHeader );
#endif
    }

    return res;
}

static int ota_server_read_body( HTTPHeader_t *httpHeader )
{
    int res = 0;
    if( ota_server_context->download_url.HTTP_SECURITY == HTTP_SECURITY_HTTP ){
        res = SocketReadHTTPBody( ota_server_context->download_url.ota_fd, httpHeader );
    }
    if( ota_server_context->download_url.HTTP_SECURITY == HTTP_SECURITY_HTTPS ){
#if OTA_USE_HTTPS
        res = SocketReadHTTPSBody( ota_server_context->download_url.ota_ssl, httpHeader );
#endif
    }
    return res;
}

static int ota_server_send_header( void )
{
    char *header = NULL;
    int j = 0;
    int ret = 0;
    header = malloc( OTA_SEND_HEAD_SIZE );
    memset( header, 0x00, OTA_SEND_HEAD_SIZE );

    j = sprintf( header, "GET " );
    j += sprintf( header + j, "/%s HTTP/1.1\r\n", ota_server_context->download_url.url );

    if ( ota_server_context->download_url.port == 0 )
    {
        j += sprintf( header + j, "Host: %s\r\n", ota_server_context->download_url.host );
    } else
    {
        j += sprintf( header + j, "Host: %s:%d\r\n", ota_server_context->download_url.host, ota_server_context->download_url.port );
    }

    j += sprintf( header + j, "Connection: close\r\n" );  //Keep-Alive close

    //Range: bytes=start-end
    if ( ota_server_context->download_state.download_begin_pos > 0 )
    {
        if ( ota_server_context->download_state.download_end_pos > 0 )
        {
            j += sprintf( header + j, "Range: bytes=%d-%d\r\n", ota_server_context->download_state.download_begin_pos,
                          ota_server_context->download_state.download_end_pos );
        } else
        {
            j += sprintf( header + j, "Range: bytes=%d-\r\n", ota_server_context->download_state.download_begin_pos );
        }
    }

    j += sprintf( header + j, "\r\n" );

    ret = ota_server_send( (uint8_t *) header, strlen( header ) );

//    ota_server_log("send: %d\r\n%s", strlen(header), header);
    if ( header != NULL ) free( header );
    return ret;
}

static void ota_server_socket_close( void )
{
#if OTA_USE_HTTPS
    if ( ota_server_context->download_url.ota_ssl ) ssl_close( ota_server_context->download_url.ota_ssl );
#endif
    SocketClose( &(ota_server_context->download_url.ota_fd) );
    ota_server_context->download_url.ota_fd = -1;
}

static int ota_server_connect_server( struct in_addr in_addr )
{
    int err = 0;
    struct sockaddr_in server_address;

    if ( ota_server_context->download_url.port == 0 )
    {
        if ( ota_server_context->download_url.HTTP_SECURITY == HTTP_SECURITY_HTTP )
        {
            server_address.sin_port = htons(80);
        } else
        {
            server_address.sin_port = htons(443);
        }
    } else
    {
        server_address.sin_port = htons(ota_server_context->download_url.port);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr = in_addr;

    err = ota_server_connect( &server_address, sizeof(server_address) );
    if ( err != 0 )
    {
        mico_thread_sleep( 1 );
        return -1;
    }

    ota_server_log("ota server connected!");
    return 0;
}

static void ota_server_progress_set( OTA_STATE_E state )
{
    float progress = 0.00;

    progress =(float) ota_server_context->download_state.download_begin_pos / ota_server_context->download_state.download_len;
    progress = progress*100;
    if(  ota_server_context->ota_server_cb != NULL )
        ota_server_context->ota_server_cb(state, progress);
}

static void ota_server_thread( mico_thread_arg_t arg )
{
    OSStatus err;
    uint16_t crc16 = 0;
    char md5_value[16] = {0};
    char md5_value_string[33] = {0};
    fd_set readfds;
    struct hostent* hostent_content = NULL;
    char **pptr = NULL;
    struct in_addr in_addr;

    mico_logic_partition_t* ota_partition = MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP );
    
    ota_server_context->ota_control = OTA_CONTROL_START;

    hostent_content = gethostbyname( ota_server_context->download_url.host );
    require_action_quiet( hostent_content != NULL, DELETE, ota_server_progress_set(OTA_FAIL));
    pptr=hostent_content->h_addr_list;
    in_addr.s_addr = *(uint32_t *)(*pptr);
    strcpy( ota_server_context->download_url.ip, inet_ntoa(in_addr));
    ota_server_log("OTA server address: %s, host ip: %s", ota_server_context->download_url.host, ota_server_context->download_url.ip);

    offset = 0;
    MicoFlashErase( MICO_PARTITION_OTA_TEMP, 0x0, ota_partition->partition_length );
    
    CRC16_Init( &crc_context );
    if( ota_server_context->ota_check.is_md5 == true ){
        InitMd5( &md5 );
    }
    
    httpHeader = HTTPHeaderCreateWithCallback( 1024, onReceivedData, NULL, NULL );
    require_action( httpHeader, DELETE, ota_server_progress_set(OTA_FAIL) );

    while ( 1 )
    {
        if ( ota_server_context->ota_control == OTA_CONTROL_PAUSE ){
            mico_thread_sleep( 1 );
            continue;
        }else if( ota_server_context->ota_control == OTA_CONTROL_STOP ){
            goto DELETE;
        }

        ota_server_context->download_url.ota_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
        err = ota_server_connect_server( in_addr );
        require_noerr_action( err, RECONNECTED,  ota_server_progress_set(OTA_FAIL));

        /* Send HTTP Request */
        ota_server_send_header( );

        FD_ZERO( &readfds );
        FD_SET( ota_server_context->download_url.ota_fd, &readfds );

        select( ota_server_context->download_url.ota_fd + 1, &readfds, NULL, NULL, NULL );
        if ( FD_ISSET( ota_server_context->download_url.ota_fd, &readfds ) )
        {
            /*parse header*/
            err = ota_server_read_header( httpHeader );
            if ( ota_server_context->ota_control == OTA_CONTROL_START )
            {
                ota_server_context->download_state.download_len = httpHeader->contentLength;
                ota_server_context->ota_control = OTA_CONTROL_CONTINUE;
            }
            switch ( err )
            {
                case kNoErr:
#if OTA_DEBUG
                    PrintHTTPHeader( httpHeader );
#endif
                    err = ota_server_read_body( httpHeader );/*get body data*/
                    require_noerr( err, RECONNECTED );
                    /*get data and print*/
                    break;
                case EWOULDBLOCK:
                case kNoSpaceErr:
                case kConnectionErr:
                default:
                    ota_server_log("ERROR: HTTP Header parse error: %d", err);
                    break;
            }
        }

        if ( ota_server_context->download_state.download_len == ota_server_context->download_state.download_begin_pos )
        {
            if( httpHeader->statusCode != 200 ){
                ota_server_progress_set(OTA_FAIL);
                goto DELETE;
            }
            CRC16_Final( &crc_context, &crc16 );
            if( ota_server_context->ota_check.is_md5 == true ){
                Md5Final( &md5, (unsigned char *) md5_value );
                hex2str((uint8_t *)md5_value, 16, md5_value_string);
            }
            if ( memcmp( md5_value_string, ota_server_context->ota_check.md5, OTA_MD5_LENTH ) == 0 ){
                ota_server_progress_set(OTA_SUCCE);
                mico_ota_switch_to_new_fw( ota_server_context->download_state.download_len, crc16 );
                mico_system_power_perform( mico_system_context_get( ), eState_Software_Reset );
            }else{
                ota_server_log("OTA md5 check err, Calculation:%s, Get:%s", md5_value_string, ota_server_context->ota_check.md5);
                ota_server_progress_set(OTA_FAIL);
            }
            goto DELETE;
        }

    RECONNECTED:
        ota_server_socket_close( );
        mico_thread_sleep(2);
        continue;

    }
DELETE:
    HTTPHeaderDestory( &httpHeader );
    ota_server_socket_close( );
    if( ota_server_context != NULL ){
        if( ota_server_context->download_url.url != NULL ){
            free(ota_server_context->download_url.url);
            ota_server_context->download_url.url = NULL;
        }
        free(ota_server_context);
        ota_server_context = NULL;
    }

    ota_server_log("ota server thread will delete");
    mico_rtos_delete_thread(NULL);
}

/*one request may receive multi reply*/
static OSStatus onReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData,
                                size_t inLen, void * inUserContext )
{
    OSStatus err = kNoErr;

    if ( inLen == 0 )
        return err;

    ota_server_context->download_state.download_begin_pos += inLen;

    CRC16_Update( &crc_context, inData, inLen );
    if( ota_server_context->ota_check.is_md5 == true ){
        Md5Update( &md5, inData, inLen );
    }

    MicoFlashWrite( MICO_PARTITION_OTA_TEMP, &offset, (uint8_t *) inData, inLen );

    ota_server_progress_set(OTA_LOADING);

    if( ota_server_context->ota_control == OTA_CONTROL_PAUSE ){
        while( 1 ){
            if( ota_server_context->ota_control != OTA_CONTROL_PAUSE )
                break;
            mico_thread_msleep(100);
        }
    }

    if( ota_server_context->ota_control == OTA_CONTROL_STOP ){
        err = kUnsupportedErr;
    }

    return err;
}

static OSStatus ota_server_set_url( char *url )
{
    OSStatus err = kNoErr;
    url_field_t *url_t;
    char *pos = NULL;

    url_t = url_parse( url );
    require_action(url, exit, err = kParamErr);
#if OTA_DEBUG
    url_field_print( url_t );
#endif
    if ( !strcmp( url_t->schema, "https" ) )
    {
        ota_server_context->download_url.HTTP_SECURITY = HTTP_SECURITY_HTTPS;
    } else
    {
        ota_server_context->download_url.HTTP_SECURITY = HTTP_SECURITY_HTTP;
    }

    strcpy( ota_server_context->download_url.host, url_t->host );
    ota_server_context->download_url.port = atoi( url_t->port );
    pos = strstr( url, url_t->path );
    if ( pos == NULL )
    {
        strcpy( ota_server_context->download_url.url, "" );
    } else
    {
        strcpy( ota_server_context->download_url.url, pos );
    }

exit:
    url_free( url_t );
    return err;
}

OSStatus ota_server_start( char *url, char *md5, ota_server_cb_fn call_back )
{
    OSStatus err = kNoErr;

    require_action(url, exit, err = kParamErr);

    if( ota_server_context != NULL ){
        if( ota_server_context->download_url.url != NULL ){
            free(ota_server_context->download_url.url);
            ota_server_context->download_url.url = NULL;
        }
        free(ota_server_context);
        ota_server_context = NULL;
    }

    ota_server_context = malloc(sizeof(ota_server_context_t));
    require_action(ota_server_context, exit, err = kNoMemoryErr);
    memset(ota_server_context, 0x00, sizeof(ota_server_context_t));

    ota_server_context->download_url.url = malloc(strlen(url));
    require_action(ota_server_context->download_url.url, exit, err = kNoMemoryErr);
    memset(ota_server_context->download_url.url, 0x00, strlen(url));

    err = ota_server_set_url(url);
    require_noerr(err, exit);

    if( md5 != NULL ){
        ota_server_context->ota_check.is_md5 = true;
        memcpy(ota_server_context->ota_check.md5, md5, OTA_MD5_LENTH);
        upper2lower(ota_server_context->ota_check.md5, OTA_MD5_LENTH);
    }

    ota_server_context->ota_server_cb = call_back;

    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "OTA", ota_server_thread, OTA_SERVER_THREAD_STACK_SIZE, 0 );
exit:
    return err;
}

void ota_server_pause( void )
{
    ota_server_context->ota_control = OTA_CONTROL_PAUSE;
}

void ota_server_continue( void )
{
    ota_server_context->ota_control = OTA_CONTROL_CONTINUE;
}

void ota_server_stop( void )
{
    ota_server_context->ota_control = OTA_CONTROL_STOP;
}

OTA_CONTROL_E ota_server_get( void )
{
    return ota_server_context->ota_control;
}
