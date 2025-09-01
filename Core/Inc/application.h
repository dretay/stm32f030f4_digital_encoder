#pragma once

#include <stdbool.h>
#include <stdlib.h>
#ifndef DEBUG
#include <string.h>
#endif
#include <limits.h>
#include <stdbool.h>

#include "LOGGER.h"
#include "Timer.h"
#include "minmax.h"
#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_it.h"

extern TIM_HandleTypeDef htim3;
extern I2C_HandleTypeDef hi2c1;


struct application {
  void (*init)(void);
  void (*run)(void);
};

extern const struct application Application;
