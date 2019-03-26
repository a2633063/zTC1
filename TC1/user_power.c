#define os_log(format, ...)  custom_log("OTA", format, ##__VA_ARGS__)

#include "mico.h"
#include "ota_server.h"
#include "main.h"
#include "user_udp.h"
#include "user_mqtt_client.h"
#include "user_function.h"

mico_timer_t power_timer;

static uint32_t timer_count_last = 0;
static uint32_t timer_count = 0;
static uint32_t timer = 0;
static uint8_t pin_input_last = 0;
static void power_timer_handler( void* arg )
{

//    uint8_t pin_input = MicoGpioInputGet( POWER );

    if ( timer_count_last != timer_count )
    {
        os_log("power_irq_handler:%u-%u=%u",timer_count,timer_count_last,timer);
        timer_count_last = timer_count;
    }
//    if(timer_count==0)  os_log("power_timer_handler Hight:%d",timer_count_last);
//    timer_count++;
//    timer_count_last=timer_count;
}

static void power_irq_handler( void* arg )
{
//    timer_count_last=timer_count;
    timer_count = UpTicks( );
    timer=timer_count-timer_count_last;
}

void user_power_init( void )
{
    os_log("user_power_init");

    MicoGpioInitialize( POWER, INPUT_PULL_UP );
    mico_rtos_init_timer( &power_timer, 20, power_timer_handler, NULL );
    mico_rtos_start_timer( &power_timer );

    MicoGpioEnableIRQ( POWER, IRQ_TRIGGER_FALLING_EDGE, power_irq_handler, NULL );
}

