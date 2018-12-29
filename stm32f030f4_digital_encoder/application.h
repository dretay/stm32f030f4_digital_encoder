#pragma once

#include <stdlib.h>

#ifndef DEBUG
#include <string.h>
#endif

#include <stdbool.h>
#include <limits.h>

#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"
#include "stm32f0xx_it.h"

#include "minmax.h"

extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim14;

struct application {	
	void(*run)(void);		
};

extern const struct application Application;