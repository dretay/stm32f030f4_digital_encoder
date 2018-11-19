#pragma once

#include <stdlib.h>
#include "string.h"
#include "stdbool.h"

#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"
#include "stm32f0xx_it.h"


extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;

struct application {	
	void(*run)(void);		
};

extern const struct application Application;