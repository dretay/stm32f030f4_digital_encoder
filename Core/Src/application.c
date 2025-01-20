/*
 * TIM3 = rotary encoder timer
 */

#include "application.h"

enum SLAVE_MODE_T {
	MODE_RECEIVING,
	MODE_TRANSMITTING,
	MODE_LISTENING,
} slaveMode;
enum IRQ_PIN_MODE_T {
	MODE_IRQ_LOW,
	MODE_IRQ_HIGH,
	MODE_SET_LOW,
	MODE_SET_HIGH,
} irqMode;

#define BUFFER_SIZE		5

static uint8_t rxBuff[BUFFER_SIZE] = {0};
static uint8_t rxBuffDataSize = 0;

static uint8_t txBuff[BUFFER_SIZE] = {0};
static uint8_t txBuffDataSize = 0;
#define GET_ENCODER		0x08
static uint8_t requestedReg;

static short encoder_val = 0;
static volatile bool switch_val = false;

static void init(){
	//start encoder timer
	HAL_Delay(100);
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
	HAL_I2C_EnableListen_IT(&hi2c1);
	slaveMode = MODE_LISTENING;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if (GPIO_Pin == GPIO_PIN_1){
		switch_val = !switch_val;
		if(irqMode == MODE_IRQ_LOW){
			irqMode = MODE_SET_HIGH;
		}
	}
}

static void run(void){
	static unsigned short raw_encoder_val;
	static short temp_encoder_val;
	raw_encoder_val = TIM3->CNT;
	temp_encoder_val = raw_encoder_val > SHRT_MAX ? (raw_encoder_val+ 2 - USHRT_MAX)>>2 : raw_encoder_val>>2;
	if (temp_encoder_val != encoder_val){
		encoder_val = temp_encoder_val;
		if(irqMode == MODE_IRQ_LOW){
			irqMode = MODE_SET_HIGH;
		}
	}
	if(irqMode == MODE_SET_HIGH){
		irqMode = MODE_IRQ_HIGH;
		app_log_debug("raising IRQ pin");
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	}
	else if(irqMode == MODE_SET_LOW){
		irqMode = MODE_IRQ_LOW;
		app_log_debug("lowering IRQ pin");
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	}
}

//Slave Address Match callback.
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode){
	if (TransferDirection == I2C_DIRECTION_RECEIVE) {
		app_log_debug("Addr callback Rx\r\n");
		slaveMode = MODE_TRANSMITTING;
		switch (requestedReg) {
			case GET_ENCODER:
				txBuff[0] = (encoder_val >> 8) & 0xFF;
				txBuff[1] = encoder_val & 0xFF;
				txBuff[2] = switch_val;
				txBuffDataSize = 3;
				break;
			default:
				txBuff[0] = 0xFF;
				txBuff[1] = 0xFF;
				txBuff[2] = 0xFF;
				txBuffDataSize = 3;
				break;
		}

		HAL_I2C_Slave_Seq_Transmit_IT(hi2c, txBuff, txBuffDataSize, I2C_LAST_FRAME);
		irqMode = MODE_SET_LOW;
		requestedReg = 0;

	} else {
		app_log_debug("Addr callback Tx\r\n");
		slaveMode = MODE_RECEIVING;
		HAL_I2C_Slave_Seq_Receive_IT(hi2c, rxBuff + rxBuffDataSize, 1, I2C_NEXT_FRAME);
	}
}
//Callback when Listen mode has completed (STOP)
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
	app_log_debug("Listening callback\r\n");
	slaveMode = MODE_LISTENING;
	if (rxBuffDataSize == 1) {
		/* Just want register value */
		app_log_debug("Just register requested\r\n");
		requestedReg = rxBuff[0];
	} else if (rxBuffDataSize > 1) {
		/* Data sent */
		for (uint8_t i = 0; i != rxBuffDataSize; i++) {
			app_log_debug("Rx Buff[%d]: %d\r\n", i, rxBuff[i]);
		}
//		requestedReg = rxBuff[0];
//		fakeVoltage = (rxBuff[1] << 8 | rxBuff[2] & 0xFF);
//		app_log_debug("Voltage set to %d\r\n", fakeVoltage);
	}

	memset(rxBuff, 0, BUFFER_SIZE);
	rxBuffDataSize = 0;

	HAL_I2C_EnableListen_IT(hi2c);
}

//Callback when Receive complete (master -> slave)
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	app_log_debug("Rx Complete Callback\r\n");
	app_log_debug("Data in: %d\r\n", rxBuff[rxBuffDataSize]);
	rxBuffDataSize++;
	if (slaveMode == MODE_RECEIVING) {
		HAL_I2C_Slave_Seq_Receive_IT(hi2c, rxBuff + rxBuffDataSize, 1, I2C_NEXT_FRAME);
	}
}
//Transfer complete (slave -> master)

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	app_log_debug("Tx Complete callback\r\n");
	txBuffDataSize = 0;
}

//Callback when error condition occurs

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
	app_log_debug("Error callback\r\n");
	HAL_I2C_EnableListen_IT(hi2c);
}

const struct application Application = {
	.init = init,
	.run = run,
};
