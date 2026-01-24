#include "application.h"

static void config(void) {
  // Add configuration setup here
}

static void init(void) {
  // Add initialization code here
}

static void run(void) {
  // Add main loop code here
}

const struct application Application = {
    .config = config,
    .init = init,
    .run = run,
};
