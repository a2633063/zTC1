#ifndef __MAIN_H_
#define __MAIN_H_

#include "mico.h"
#include "MiCOKit_EXT.h"

#define Led         MICO_GPIO_5
#define Button      MICO_GPIO_23

#define Relay_0     MICO_GPIO_6
#define Relay_1     MICO_GPIO_7
#define Relay_2     MICO_GPIO_8
#define Relay_3     MICO_GPIO_9
#define Relay_4     MICO_GPIO_10
#define Relay_5     MICO_GPIO_18
#define Relay_NUM   6


//用户保存参数结构体
typedef struct {
    char val;
} user_config_t;

extern system_config_t * sys_config;
extern user_config_t * user_config;

extern mico_gpio_t Relay[Relay_NUM];

#endif
