/**
  @page " http_sever "  demo
  
  @verbatim
  ******************** (C) COPYRIGHT 2016 MXCHIP MiCO SDK*******************
  * @file    http_sever/readme.txt 
  * @author  MDWG (MiCO Documentation Working Group)
  * @version v2.4.x
  * @date    11-April-2016
  * @brief   Description of the  "http_sever"  demo.
  ******************************************************************************

  @par Demo Description 
  This demo shows:  how to start http sever to realize  network configuration through webpage on PC or Mobile Phone.


@par Directory contents 
    - Demos/http/http_sever/http_sever.c      HTTP server demo entrance
    - Demos/http/http_sever/app_httpd.c       HTTP server demo
    - Demos/http/http_sever/web_data.c        Webpage applicaiton program
    - Demos/http/http_sever/mico_config.h     MiCO function header file
    - Demos/http/http_sever/app_httpd.h       app_httpd.c header file


@par Hardware and Software environment         
    - This demo has been tested on MiCOKit-3165 board.
    - This demo can be easily tailored to any other supported device and development board.


@par How to use it ? 
In order to make the program work, you must do the following :
 - Open your preferred toolchain, 
    - IDE:  IAR 7.30.4 or Keil MDK 5.13.       
    - Debugging Tools: JLINK or STLINK
 - Modify header file path of  "mico_config.h".   Please referring to "http://mico.io/wiki/doku.php?id=confighchange"
 - Rebuild all files and load your image into target memory.   Please referring to "http://mico.io/wiki/doku.php?id=debug"
 - Run the demo.
 - View operating results and system serial log (Serial port: Baud rate: 115200, data bits: 8bit, parity: No, stop bits: 1).   Please referring to http://mico.io/wiki/doku.php?id=com.mxchip.basic

**/

