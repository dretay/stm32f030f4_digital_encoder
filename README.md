- [Overview](#overview)
- [Example Usage](#example-usage)

# Overview
Simple project that allows you to present a digital rotary encoder as a i2c slave perhiperal. Useful when prototyping circuts since you can just drop this onto the i2c bus of the project you're developing.  

# Example Usage
For an STM32 master, since you're using 7b addressing left shift the address by one. The command to request the encoder's current value is `0x08` and it will return 3 bytes in response. 
```c
	uint16_t SLAVE_ADDR = 0x10<<1;
	uint8_t TxData[] = {0x08};
	HAL_I2C_Master_Transmit(&hi2c2, SLAVE_ADDR, TxData, 1, 1000);
	HAL_Delay (100);

	uint8_t RxData[3] = {0};
	HAL_I2C_Master_Receive(&hi2c2, SLAVE_ADDR, RxData, 3, 1000);
	asm("nop");
```