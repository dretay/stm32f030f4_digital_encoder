#include "application.h"

#define I2C_RX_BUFFER_SIZE  1
static unsigned char i2c_rx_buffer[I2C_RX_BUFFER_SIZE];

#define I2C_TX_BUFFER_SIZE 4
static unsigned char i2c_tx_buffer[I2C_TX_BUFFER_SIZE];

static short encoder_val = 0;
static volatile bool switch_val = false;
static volatile bool dirty = false;
static bool xmit = false;

#define ENCODER_AND_SWITCH_QUERY 0x00
#define ENCODER_AND_SWITCH_RESET  0x01

#define UART_BUFFER_SIZE 32
static char uart_tx_buffer[UART_BUFFER_SIZE];
static char* uart_tx_cr = "\r\n";

enum IRQ_ATTN_STATUS
{
OFF = 0,
PENDING_ON =1,
PENDING_OFF=2,
ON = 3
};

static enum IRQ_ATTN_STATUS irq_attn_status = OFF;

//todo: for some reason I don't understand snprintf is messing with \r\n sequence... so
//it needs to be echo'd separate from the string. if it becomes a problem maybe make this flaggable?
static void println(char* line)
{
#ifndef DEBUG
	int str_len = 0;
	snprintf(uart_tx_buffer, UART_BUFFER_SIZE, "%s",line);
	str_len = strcspn(uart_tx_buffer, "\0");
	if (HAL_UART_Transmit(&huart1, (uint8_t*)line, str_len, 5000) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UART_Transmit(&huart1, (uint8_t*)uart_tx_cr, 2, 5000) != HAL_OK)
	{
		Error_Handler();
	}
#endif
}

//check the state of irq_attn_status and flap pin if appropriate
static void poll_irq_attn_status(void)
{
	if (irq_attn_status == PENDING_ON)
	{

		println("raising IRQ pin");
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
		irq_attn_status = ON;

	}
	else if (irq_attn_status == PENDING_OFF)
	{
		println("lowering IRQ pin");
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
		irq_attn_status = OFF;

	}
}

//make state change if data has become dirty
static void set_dirty(void)
{
	if (irq_attn_status == OFF)
	{
		irq_attn_status = PENDING_ON;
	}
}

//make state change if data has been xmitted to master
static void unset_dirty(void)
{
	if (irq_attn_status == ON)
	{
		irq_attn_status = PENDING_OFF;
	}
}

//loop for polling data structures when not busy with other stuff
static void busy_wait_loop(void)
{
	static unsigned short raw_encoder_val;
	static short temp_encoder_val;
	while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
	{
		raw_encoder_val = TIM3->CNT;
		temp_encoder_val = raw_encoder_val > SHRT_MAX ? (raw_encoder_val+ 2 - USHRT_MAX)>>2 : raw_encoder_val>>2;
		if (temp_encoder_val != encoder_val)
		{
			encoder_val = temp_encoder_val;
			set_dirty();
		}
		poll_irq_attn_status();
	}
}

//main application loop
static void run(void)
{

	println("waiting for i2c cmd");
	xmit = false;
	//recieve data from master
	if(HAL_I2C_Slave_Receive_IT(&hi2c1, (uint8_t *)i2c_rx_buffer, I2C_RX_BUFFER_SIZE) != HAL_OK)
	{
		println("HAL_I2C_Slave_Receive_IT error");
	}

	busy_wait_loop();

	switch (i2c_rx_buffer[0])
	{
	case ENCODER_AND_SWITCH_QUERY:
		println("encoder dump cmd rcvd");
		i2c_tx_buffer[0] = (encoder_val >> 8) & 0xFF;
		i2c_tx_buffer[1] = encoder_val & 0xFF;
		i2c_tx_buffer[2] = switch_val;
		xmit = true;
		break;

	case ENCODER_AND_SWITCH_RESET:
		println("encoder RESET cmd rcvd");
		TIM3->CNT = 0;
		break;

	default:
		println("default cmd case reached!");
		i2c_tx_buffer[0] = 0xFF;
		xmit = true;
	}
	if (xmit)
	{
		xmit = false;
		println("xmit i2c response");
		HAL_TIM_Base_Start_IT(&htim14);
		if (HAL_I2C_Slave_Transmit_IT(&hi2c1, i2c_tx_buffer, I2C_TX_BUFFER_SIZE) != HAL_OK)
		{
			println("HAL_I2C_Slave_Transmit_IT error");
		}
		busy_wait_loop();
		HAL_TIM_Base_Stop_IT(&htim14);
		unset_dirty();
	}

}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *I2cHandle)
{
	if (HAL_I2C_GetError(I2cHandle) != HAL_I2C_ERROR_AF)
	{
		Error_Handler();
	}
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == 2)
	{
		switch_val = !switch_val;
		set_dirty();
	}
}
//if i2c xmit times out re-init i2c phy
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim14)
{
	println("i2c timeout!");
	if(HAL_I2C_Init(&hi2c1) != HAL_OK)
	{
		println("failed i2c reinit!");
	}

}


const struct application Application = {
	.run = run,
};
