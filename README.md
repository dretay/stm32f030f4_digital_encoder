- [Overview](#overview)
- [Hardware Values](#hardware-values)
- [Example Usage](#example-usage)

# Overview
Simple project that allows you to present a digital rotary encoder as a i2c slave perhiperal. Useful when prototyping circuts since you can just drop this onto the i2c bus of the project you're developing.  

# Hardware Values
| Component          | Value |
| ------------------ | ----- |
| R1, R2, R3, R7, R8 | 4.7K  |
| R4, R5, R6         | 1K    |
| C1, C2, C3         | 0.1uF |
| C4                 | 10uF  |

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