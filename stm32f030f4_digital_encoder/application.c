#include "application.h"

#define I2C_RX_BUFFER_SIZE  1
static unsigned char i2c_rx_buffer[I2C_RX_BUFFER_SIZE];

#define I2C_TX_BUFFER_SIZE 1
static unsigned char i2c_tx_buffer[I2C_TX_BUFFER_SIZE];

static int encoder_val = 0;
static volatile bool switch_val = false;
static volatile bool dirty = false;

#define TEMP_OUT_INT_REGISTER   0x0
#define TEMP_OUT_FRAC_REGISTER  0x1
#define WHO_AM_I_REGISTER       0xF
#define WHO_AM_I_VALUE          0xBC

static char tx_buffer[8];
static void check_encoder(void)
{
	if (dirty)
	{
		dirty = false;
		snprintf(tx_buffer, 8, "%d %d \r\n", (int)switch_val, encoder_val);
		if (HAL_UART_Transmit(&huart1, (uint8_t*)tx_buffer, 8, 5000) != HAL_OK)
		{
			Error_Handler();
		}
	}
	if (TIM3->CNT != encoder_val)
	{
		encoder_val = TIM3->CNT;	
		dirty = true;
	}			
}
static void run(void)
{
	while (true)
	{	
  
		//recieve data from master
		if(HAL_I2C_Slave_Receive_IT(&hi2c1, (uint8_t *)i2c_rx_buffer, I2C_RX_BUFFER_SIZE) != HAL_OK)
		{		
			Error_Handler();        
		}
  
		//wait for master xfer to complete
		while(HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
		{
			check_encoder();
		}

		switch (i2c_rx_buffer[0]) 
		{
			case WHO_AM_I_REGISTER:
				i2c_tx_buffer[0] = WHO_AM_I_VALUE;
				break;
			case TEMP_OUT_INT_REGISTER:
				i2c_tx_buffer[0] = encoder_val;
				break;
			case TEMP_OUT_FRAC_REGISTER:
				i2c_tx_buffer[0] = switch_val;
				break;
			default:
				i2c_tx_buffer[0] = 0xFF;
		}

		HAL_I2C_Slave_Transmit_IT(&hi2c1, i2c_tx_buffer, 1);
		while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) 
		{
			check_encoder();
		}
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
		dirty = true;
	}
}


const struct application Application = { 
	.run = run,		
};