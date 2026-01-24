#pragma once

#include "stm32f0xx_hal.h"

struct application {
  void (*config)(void);
  void (*init)(void);
  void (*run)(void);
};

extern const struct application Application;
