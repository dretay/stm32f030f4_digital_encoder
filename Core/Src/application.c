/*
 * TIM3 = rotary encoder timer
 */

#include "application.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* --- Pin / I2C Register Definitions --- */
#define IRQ_PIN GPIO_PIN_5
#define SWITCH_PIN GPIO_PIN_1
#define I2C_GET_ENCODER 0x08
#define BUFFER_SIZE 5

/* Debounce time in milliseconds */
#define SWITCH_DEBOUNCE_MS 50

/* --- Enums --- */
typedef enum { MODE_RECEIVING, MODE_TRANSMITTING, MODE_LISTENING } SLAVE_MODE_T;

typedef enum {
  MODE_IRQ_LOW,
  MODE_IRQ_HIGH,
  MODE_SET_LOW,
  MODE_SET_HIGH
} IRQ_PIN_MODE_T;

/* --- Static Variables --- */
static volatile SLAVE_MODE_T slaveMode;
static volatile IRQ_PIN_MODE_T irqMode;

static uint8_t rxBuff[BUFFER_SIZE] = {0};
static uint8_t rxBuffDataSize = 0;

static uint8_t txBuff[BUFFER_SIZE] = {0};
static uint8_t txBuffDataSize = 0;

static volatile uint8_t requestedReg;
static int16_t encoder_val = 0;
static volatile bool switch_dirty = false;

static int irq_timer = 0;

/* Timestamp for debouncing */
static volatile uint32_t last_switch_tick = 0;

/* --- Helper Functions --- */
static inline void set_irq_pin_high(void);
static inline void set_irq_pin_low(void);
static inline void force_irq_low(void);
static void update_irq_pin(void);
static void prepare_tx_data(void);
static inline bool switch_debounce(void);

static inline void force_irq_low(void) {
  irqMode = MODE_SET_LOW;
  update_irq_pin();
}
static inline void set_irq_pin_high(void) {
  HAL_GPIO_WritePin(GPIOA, IRQ_PIN, GPIO_PIN_SET);
  app_log_debug("raising IRQ pin");
  // if 5s has gone by and the pin has not been set low, force it low so we
  // don't get stuck in a loop
  irq_timer = Timer.in(1000, set_irq_pin_low);
}

static inline void set_irq_pin_low(void) {
  HAL_GPIO_WritePin(GPIOA, IRQ_PIN, GPIO_PIN_RESET);
  app_log_debug("lowering IRQ pin");
  Timer.cancel(irq_timer);
}

static void update_irq_pin(void) {
  switch (irqMode) {
    case MODE_SET_HIGH:
      irqMode = MODE_IRQ_HIGH;
      set_irq_pin_high();
      break;
    case MODE_SET_LOW:
      irqMode = MODE_IRQ_LOW;
      set_irq_pin_low();
      break;
    default:
      break;
  }
}

static void prepare_tx_data(void) {
  if (requestedReg == I2C_GET_ENCODER) {
    txBuff[0] = (encoder_val >> 8) & 0xFF;
    txBuff[1] = encoder_val & 0xFF;
    txBuff[2] = switch_dirty;
    txBuffDataSize = 3;
  } else {
    memset(txBuff, 0xFF, 3);
    txBuffDataSize = 3;
  }
}

static inline bool switch_debounce(void) {
  uint32_t now = HAL_GetTick();
  if (now - last_switch_tick > SWITCH_DEBOUNCE_MS) {
    last_switch_tick = now;
    return true;
  }
  return false;
}

/* --- Initialization --- */
static void init(void) {
  HAL_Delay(100);
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
  HAL_I2C_EnableListen_IT(&hi2c1);

  slaveMode = MODE_LISTENING;
  irqMode = MODE_IRQ_LOW;
  HAL_GPIO_WritePin(GPIOA, IRQ_PIN, GPIO_PIN_RESET);

  Timer.init();
}

/* --- GPIO Interrupt --- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == SWITCH_PIN && switch_debounce()) {
    switch_dirty = true;
    if (irqMode == MODE_IRQ_LOW) {
      irqMode = MODE_SET_HIGH;
    }
  }
}

/* --- Main Run Loop --- */
static void run(void) {
  int16_t raw_encoder_val = (int16_t)TIM3->CNT;

  if (raw_encoder_val != encoder_val) {
    encoder_val = raw_encoder_val;
    if (irqMode == MODE_IRQ_LOW) {
      irqMode = MODE_SET_HIGH;
    }
  }

  update_irq_pin();
}

/* --- I2C Callbacks --- */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection,
                          uint16_t AddrMatchCode) {
  if (TransferDirection == I2C_DIRECTION_RECEIVE) {
    slaveMode = MODE_TRANSMITTING;
    prepare_tx_data();

    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, txBuff, txBuffDataSize, I2C_LAST_FRAME);

    irqMode = MODE_SET_LOW;
    requestedReg = 0;
    switch_dirty = false;
  } else {
    slaveMode = MODE_RECEIVING;
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, rxBuff + rxBuffDataSize, 1,
                                 I2C_NEXT_FRAME);
  }
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
  slaveMode = MODE_LISTENING;

  if (rxBuffDataSize == 1) {
    requestedReg = rxBuff[0];
  } else if (rxBuffDataSize > 1) {
    for (uint8_t i = 0; i < rxBuffDataSize; i++) {
      app_log_debug("Rx Buff[%d]: %d\r\n", i, rxBuff[i]);
    }
  }

  memset(rxBuff, 0, sizeof(rxBuff));
  rxBuffDataSize = 0;
  HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  rxBuffDataSize =
      (rxBuffDataSize < BUFFER_SIZE) ? rxBuffDataSize + 1 : BUFFER_SIZE;
  if (slaveMode == MODE_RECEIVING) {
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, rxBuff + rxBuffDataSize, 1,
                                 I2C_NEXT_FRAME);
  }
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  txBuffDataSize = 0;
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
  app_log_debug("I2C Error callback\r\n");
  HAL_I2C_EnableListen_IT(hi2c);
}

/* --- Application Structure --- */
const struct application Application = {
    .init = init,
    .run = run,
};
