/**
 ******************************************************************************
 * @file    ota_sever.h
 * @author  QQ ding
 * @version V1.0.0
 * @date    19-Oct-2016
 * @brief   Provide ota server header files.
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2014 MXCHIP Inc.
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
#ifndef __ota_server_H
#define __ota_server_H

#define OTA_DEBUG     (1)
#define OTA_USE_HTTPS (0)

#define OTA_MD5_LENTH 32
#define OTA_SEND_HEAD_SIZE 256
#if OTA_USE_HTTPS
#define OTA_SERVER_THREAD_STACK_SIZE  0x2000
#else
#define OTA_SERVER_THREAD_STACK_SIZE 0x800
#endif

typedef enum _OTA_STATE_E{
    OTA_LOADING,
    OTA_SUCCE,
    OTA_FAIL
}OTA_STATE_E;

typedef enum _HTTP_SECURITY_E{
  HTTP_SECURITY_HTTP,
  HTTP_SECURITY_HTTPS
} HTTP_SECURITY_E;

typedef enum _OTA_CONTROL_E{
    OTA_CONTROL_IDLE,
    OTA_CONTROL_START,
    OTA_CONTROL_PAUSE,
    OTA_CONTROL_CONTINUE,
    OTA_CONTROL_STOP,
} OTA_CONTROL_E;

typedef struct _download_url_t{
    char              *url;
    HTTP_SECURITY_E   HTTP_SECURITY;
    char              host[30];
    char              ip[16];
    int               port;
    int               ota_fd;
#if OTA_USE_HTTPS
    mico_ssl_t        ota_ssl;
#endif
} download_url_t;

typedef struct _download_state_t{
    int               download_len;
    int               download_begin_pos;
    int               download_end_pos;
} download_state_t;

typedef struct _ota_check_t{
    bool              is_md5;
    char              md5[OTA_MD5_LENTH + 1];
} ota_check_t;

typedef void (*ota_server_cb_fn) (OTA_STATE_E state, float progress);

typedef struct _ota_server_context_t{
  download_url_t      download_url;
  download_state_t    download_state;
  ota_check_t         ota_check;
  OTA_CONTROL_E       ota_control;
  ota_server_cb_fn    ota_server_cb;
} ota_server_context_t;


/** @addtogroup OTA_SERVER_DAEMONS_APIs
  * @{
  */


/** @brief    Start OTA server, Support resume from break point, MD5 check
  *
  * @param    url        : Download address, URL breakdown from RFC 3986
  * @param    md5        : MD5 checksum result, must sting type, can be NULL
  * @param    call_back  : call back function, can be NULL
  *
  * @return   kNoErr        : on success.
  * @return   kGeneralErr   : if an error occurred
  */
OSStatus ota_server_start( char *url, char *md5, ota_server_cb_fn call_back );


/** @brief    Get OTA server state
 *
 *  @return   OTA_CONTROL_E  : state
 */
OTA_CONTROL_E ota_server_state_get( void );


/** @brief    Pause OTA server daemons
 *
 *  @return   No
 */
void ota_server_pause( void );


/** @brief    Continue OTA server daemons
 *
 *  @return   No
 */
void ota_server_continue( void );


/** @brief    Stop OTA server daemons
 *
 *  @return   No
 */
void ota_server_stop( void );

#endif
