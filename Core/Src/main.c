/*
 * This is the implementation of an i2c driver for a bmp280 sensor starting from scratch
 * using CMSIS headers only.
 */

#include "stm32f4xx.h"
#include "stdint.h"


#define BMP280_I2C_ADDR 		0x76	//7 bit address

#define BMP280_REG_ID			0xD0	// contains chip identification number 0x58
#define BMP280_REG_RESET		0xE0
#define BMP280_REG_STATUS		0xF3
#define BMP280_REG_CTRL_MEAS	0xF4
#define BMP280_REG_CONFIG		0xF5
/*
#define
#define
*/

// the timeout function
/*
This function is used to check hardware status-registers against a known bit mask that we want to obtain
for suitable condition

Ex: Checking whether start condition is generated or not in SR1 after we set the start bit in CR1
(See first usage of timeout function in write funciton)
*/

static int timeout(volatile uint32_t *reg, uint16_t mask)
{
	uint8_t time = 250;				// storing 250 in time var as it is 8-bit variable
	for(uint16_t t = 0; t <= time * 4; t++)		// rough guessing of 1000 cycles for timeout completion
	{
		if((*reg & mask) == mask){return 1;}

	}
	return 0;
}

// i2c write function
int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
	volatile uint8_t dummy = 0;
	//dummy variable not used. explained more after 23 lines

	//enabling ACK for acknowledgment of received data
	I2C1->CR1 |= 0x1 << 10;

	//starting generation
	I2C1->CR1 |= 0x1 << 8;

	if(timeout(&I2C1->SR1, (1 << 0)) == 0){ // checking if start condition is generated
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	//writing i2c address to data register
	I2C1->DR = addr << 1 | 0;

	//waiting for addr bit to set to finish transmitting address
	if(timeout(&I2C1->SR1, (1 << 1)) == 0){
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	//dummy status register read(stm32 hardware quirk)
	//reading 
	dummy = I2C1->SR1;
	dummy = I2C1->SR2;

	//writing the register address to the DR
	I2C1->DR = reg;

	//waiting for byte transfer finished flag for register address write completion
	if(timeout(&I2C1->SR1, (1 << 2)) == 0){
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	//writing data to the register
	I2C1->DR = value;

	//waiting for byte transfer finished flag for data write completion
	if(timeout(&I2C1->SR1, (1 << 2)) == 0){
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	I2C1->CR1 |= 0X1 << 9; //stopping I2C
	return 1;	// if everything goes well
}


// i2c read function
int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value)
{
	volatile uint8_t dummy = 0;// dummy variable to read sr1 and sr2
	//enabling ACK for acknowledgment of received data
	I2C1->CR1 |= 0x1 << 10;

	//starting generation
	I2C1->CR1 |= 0x1 << 8;

	if(timeout(&I2C1->SR1, (1 << 0)) == 0){ // checking if start condition is generated
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	//writing i2c address to data register
	I2C1->DR = addr << 1 | 0;

	//waiting for addr bit to set to finish transmitting address
	//while(!(I2C1->SR1 & (1 << 1)));
	if(timeout(&I2C1->SR1, (1 << 1)) == 0){
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	//dummy sr1 and sr2 read to clear addr
	dummy = I2C1->SR1;
	dummy = I2C1->SR2;

	//writing register value to the data register
	I2C1->DR = reg;

	//waiting for byte transfer finished flag for data write completion
	if(timeout(&I2C1->SR1, (1 << 2)) == 0){
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	//repeated start condition
	I2C1->CR1 |= 0x1 << 8;

	if(timeout(&I2C1->SR1, (1 << 0)) == 0){ // checking if start condition is generated
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	//writing address to data register for read operation
	I2C1->DR = addr << 1 | 1;

	if(timeout(&I2C1->SR1, (1 << 1)) == 0){
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	I2C1->CR1 &= ~(1<<10);  // clear the ACK bit as we only need one byte

	//dummy sr1 and sr2 read to clear addr
	dummy = I2C1->SR1;
	dummy = I2C1->SR2;

	I2C1->CR1 |= 0X1 << 9; //stopping I2C

	if(timeout(&I2C1->SR1, (1<<6)) == 0){  // wait for RxNE to set receive not empty
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}
	*value = I2C1->DR;
	I2C1->CR1 |= (1 << 10);//renabling ack for future transfers

	return 1;
}

int main()
{

	//	SETUP
	RCC->AHB1ENR |= 0x1 << 1;		//Enable clock for AHB1 bus
	RCC->APB1ENR |= 0x1 << 21;		//Enable peripheral

	GPIOB->MODER &= ~((0x3 << 14) | (0x3 << 12));	//Put port to alternate function mode
	GPIOB->MODER |= (0x1 << 15) | (0x1 << 13);

	GPIOB->OTYPER |= (0X1 << 6) | (0X1 << 7);	//Mandatory Open-Drain for I2C, otherwise short circuit

	GPIOB->AFR[0] &= ~((0xF << 28) | (0XF << 24)); //Alternate function selection
	GPIOB->AFR[0] |= (0x4 << 28) | (0x4 << 24);

	GPIOB->OSPEEDR &= ~((0x3 << 14) | (0x3 << 12));		//Setting fast speed 10
	GPIOB->OSPEEDR |= (0x1 << 15) | (0x1 << 13);

	// I2C CONFIGURATION

	//setting bus speed
	//Setting the clock peripheral receives from bus
	I2C1->CR2 |= 16; //0x1 << 4; is not recommended for dirty bits

	//setting clock control
	I2C1->CCR &= ~0xFFFF;
	I2C1->CCR |= 0x50;

	//setting t_rise
	I2C1->TRISE = 0x11;

	//enabling peripheral
	I2C1->CR1 |= 0X1;



	/*// test for the read function
	uint8_t id = 0;

	if(!i2c_read_reg(BMP280_I2C_ADDR, BMP280_REG_ID, &id)){
		while(1){__NOP();}
	}
	*/


	/*//test for the write function
	uint8_t ctrl_meas_status;
	if(!i2c_write_reg(BMP280_I2C_ADDR, BMP280_REG_CTRL_MEAS, 0x4B)){
		while(1){__NOP();}
	}
	if(!i2c_read_reg(BMP280_I2C_ADDR, BMP280_REG_CTRL_MEAS, &ctrl_meas_status)){
		while(1){__NOP();}
	}
	*/
	return 0;
}
