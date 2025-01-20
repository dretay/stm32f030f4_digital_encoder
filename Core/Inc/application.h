#pragma once

#include <stdlib.h>
#include<stdbool.h>
#ifndef DEBUG
#include <string.h>
#endif
#include "LOGGER.h"
#include <stdbool.h>
#include <limits.h>

#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"
#include "stm32f0xx_it.h"

#include "minmax.h"

extern TIM_HandleTypeDef htim3;
extern I2C_HandleTypeDef hi2c1;

struct application {
	void(*init)(void);
	void(*run)(void);
};

extern const struct application Application;
