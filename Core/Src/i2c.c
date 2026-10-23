
#include "i2c.h"
#include "stdint.h"
#include "stm32f4xx.h"
// the timeout function
/*
This function is used to check hardware status-registers against a known bit mask that we want to obtain
for suitable condition

Example: Checking whether start condition is generated or not in SR1 after we set the start bit in CR1
(See first usage of timeout function in write function)
*/

static int timeout(volatile uint32_t *reg, uint16_t msk)
{
	uint16_t time = 10000;							// storing 250 in time variable as it is 8-bit variable
	for(uint16_t t = 0; t <= time * 4; t++)		// rough guessing of 1000 cycles for timeout completion
	{
		if((*reg & msk) == msk){return 1;}

	}
	return 0;
}


void i2c_clear_ADDR(void)
{
	int dummy = 0;
	//dummy sr1 and sr2 read to clear ADDR
	dummy = I2C1->SR1;
	dummy = I2C1->SR2;

}

#define I2C_TMO(reg, mask, err) do { \
	if(!timeout(reg, mask)) { \
		I2C1->CR1 |= 0X1 << 9; \
		return err; } \
}while(0)

void i2c1_init(void)
{
	//	SETUP
	RCC->AHB1ENR |= 0x1 << 1;		//Enable clock for AHB1 bus
	RCC->APB1ENR |= 0x1 << 21;		//Enable peripheral

	GPIOB->MODER &= ~((0x3 << 14) | (0x3 << 12));	//Put port to alternate function mode
	GPIOB->MODER |= (0x1 << 15) | (0x1 << 13);

	GPIOB->OTYPER |= (0X1 << 6) | (0X1 << 7);		// Mandatory Open-Drain for I2C, otherwise short circuit

	GPIOB->AFR[0] &= ~((0xF << 28) | (0XF << 24)); 	// Alternate function selection
	GPIOB->AFR[0] |= (0x4 << 28) | (0x4 << 24);

	GPIOB->OSPEEDR &= ~((0x3 << 14) | (0x3 << 12));	//Setting fast speed 10
	GPIOB->OSPEEDR |= (0x1 << 15) | (0x1 << 13);

	// I2C CONFIGURATION

	// setting bus speed
	// Setting the clock peripheral receives from bus
	I2C1->CR2 |= 16; //0x1 << 4; is not recommended for dirty bits

	// setting clock control
	I2C1->CCR &= ~0xFFFF;
	I2C1->CCR |= 0x50;

	// setting t_rise
	I2C1->TRISE = 0x11;

	// enabling peripheral
	I2C1->CR1 |= 0X1;
}


// i2c write function
i2c_status_t i2c_write(uint8_t addr, uint8_t reg, uint8_t value)
{

	//enabling ACK for acknowledgment of received data
	I2C1->CR1 |= I2C_CR1_ACK;

	//starting generation
	I2C1->CR1 |= I2C_CR1_START;

	// checking if start condition is generated or not
	I2C_TMO(&I2C1->SR1, I2C_SR1_SB_Msk, I2C_ERR_TMO_SB);

	//writing i2c address to data register
	I2C1->DR = addr << 1 | 0;

	//waiting for ADDR bit to set to finish transmitting address
	I2C_TMO(&I2C1->SR1, I2C_SR1_ADDR_Msk, I2C_ERR_TMO_ADDR);

	i2c_clear_ADDR();

	//writing the register address to the DR
	I2C1->DR = reg;

	//waiting for byte transfer finished flag for register address write completion
	I2C_TMO(&I2C1->SR1, I2C_SR1_BTF_Msk, I2C_ERR_TMO_TXE);

	// writing data to the register
	I2C1->DR = value;

	//waiting for byte transfer finished flag for data write completion
	I2C_TMO(&I2C1->SR1, I2C_SR1_BTF_Msk, I2C_ERR_TMO_TXE);


	I2C1->CR1 |= I2C_CR1_STOP_Msk; 	// stopping I2C
	return I2C_OK;				// if everything goes well
}


/* i2c read function */
/*
 * addr 	- the i2c address of the bmp280
 * reg 		- the register address inside the chip
 * value 	- the variable in which the value from the register is read
 * bytes 	- number of bytes to read
 */
i2c_status_t i2c_read(uint8_t addr, uint8_t reg, uint8_t *value, uint32_t bytes)
{
	if(bytes == 1)
	{
		// READ PROCESS FOR 1 BYTE

		//enabling ACK for acknowledgment of received data
		I2C1->CR1 |= 0x1 << 10;

		//starting generation
		I2C1->CR1 |= 0x1 << 8;

		I2C_TMO(&I2C1->SR1, (1 << 0), I2C_ERR_TMO_SB);

		//writing i2c address to data register
		I2C1->DR = addr << 1 | 0;

		// waiting for ADDR bit to set to finish transmitting address
		I2C_TMO(&I2C1->SR1, (1 << 1), I2C_ERR_TMO_ADDR);

		i2c_clear_ADDR();

		//writing register value to the data register
		I2C1->DR = reg;

		//waiting for byte transfer finished flag for register address write completion
		I2C_TMO(&I2C1->SR1, (1 << 2), I2C_ERR_TMO_TXE);

		//repeated start condition
		I2C1->CR1 |= 0x1 << 8;

		I2C_TMO(&I2C1->SR1, (1 << 0), I2C_ERR_TMO_SB);

		//writing address to data register for read operation
		I2C1->DR = addr << 1 | 1;

		I2C_TMO(&I2C1->SR1, (1 << 1), I2C_ERR_TMO_ADDR);

		I2C1->CR1 &= ~I2C_CR1_ACK_Msk;  // clear the ACK bit as we only need one byte

		i2c_clear_ADDR();

		I2C1->CR1 |= I2C_CR1_STOP_Msk; // stopping I2C

		// waiting for RXNE
		I2C_TMO(&I2C1->SR1, I2C_SR1_RXNE_Msk, I2C_ERR_TMO_RXNE);

		*value = I2C1->DR;
		I2C1->CR1 |= I2C_CR1_ACK_Msk;					//re-enabling ACK for future transfers

		return I2C_OK;
	}

	else if(bytes == 2)
	{
		// NOT REQUIRED FOR BMP280 HENCE RETURNING ARGUMENT ERROR
		return I2C_ERR_ARG;
	}
	else if(bytes >= 3)
	{
		// enabling ACK for acknowledgment of received data
		I2C1->CR1 |= I2C_CR1_ACK_Msk;

		// starting generation
		I2C1->CR1 |= I2C_CR1_START_Msk;

		I2C_TMO(&I2C1->SR1, I2C_SR1_SB_Msk, I2C_ERR_TMO_SB);

		// writing i2c address to data register
		I2C1->DR = addr << 1 | 0;

		// waiting for ADDR bit to set to finish transmitting address
		I2C_TMO(&I2C1->SR1, I2C_SR1_ADDR_Msk, I2C_ERR_TMO_ADDR);

		i2c_clear_ADDR();

		//writing register value to the data register
		I2C1->DR = reg;

		//waiting for byte transfer finished flag for register address write completion
		I2C_TMO(&I2C1->SR1, I2C_SR1_BTF_Msk, I2C_ERR_TMO_TXE);

		// repeated start condition
		I2C1->CR1 |= I2C_CR1_START_Msk;

		I2C_TMO(&I2C1->SR1, I2C_SR1_SB_Msk, I2C_ERR_TMO_SB);

		// writing address to data register for read operation
		I2C1->DR = addr << 1 | 1;

		I2C_TMO(&I2C1->SR1, I2C_SR1_ADDR_Msk, I2C_ERR_TMO_ADDR);

		// clearing ADDR without clearing ACK, this is one difference in multiple byte read
		i2c_clear_ADDR();

		// bulk byte read until n-3
		uint32_t remaining = bytes;
		// this loop is to run until when 3 bytes are left
		while(remaining > 3)
		{
			// RxNE is set when data register has data, so checking
			I2C_TMO(&I2C1->SR1, I2C_SR1_RXNE_Msk, I2C_ERR_TMO_RXNE);

			value[ bytes - remaining ] = I2C1->DR;
			remaining--; // decrementing remaining when a byte is read
		}

		// last 3 bytes
		//waiting for byte transfer finished flag to apply configurations for next transfer
		I2C_TMO(&I2C1->SR1, I2C_SR1_BTF_Msk, I2C_ERR_TMO_TXE);

		I2C1->CR1 &= ~I2C_CR1_ACK_Msk;  // clear the ACK bit so that a NACK is send for the last byte
		// reading n-2 byte
		value[ bytes - remaining ] =  I2C1->DR;
		remaining--;

		//waiting for byte transfer finished flag to apply configurations for next transfer
		I2C_TMO(&I2C1->SR1, I2C_SR1_BTF_Msk, I2C_ERR_TMO_TXE);

		I2C1->CR1 |= I2C_CR1_STOP_Msk; // stopping I2C

		// reading n-1 byte
		value[ bytes - remaining ] =  I2C1->DR;
		remaining--;

		I2C_TMO(&I2C1->SR1, I2C_SR1_RXNE_Msk, I2C_ERR_TMO_RXNE);

		// reading last byte
		value[ bytes - remaining ] =  I2C1->DR;
		remaining--;

		// re-enabling ACK for future transfers
		I2C1->CR1 |= (1 << 10);

		return I2C_OK;
	 }
	else
	{
		return I2C_ERR_ARG;
	}
}

