
#define os_log(format, ...)  custom_log("KEY", format, ##__VA_ARGS__)

#include "main.h"
//#include "key.h"

void led(char x)
{
    if(x==-1)MicoGpioOutputTrigger(Led);
    else if(x) MicoGpioOutputHigh(Led);
    else MicoGpioOutputLow(Led);
}

static void key_long_press(void)
{
    os_log("key_long_press");
    MicoGpioOutputHigh(MICO_GPIO_5);
}
static void key_short_press(void)
{
    //os_log("test");
    MicoGpioOutputTrigger(MICO_GPIO_5);
}

void key_init(void)
{
    button_init_t button_config={
            .gpio=Button,
            .long_pressed_func=key_long_press,
            .pressed_func=key_short_press,
            .long_pressed_timeout=800,
        };

    button_init(IOBUTTON_USER_1,button_config);
}

