#include "stm32f4xx.h"
#include "stdint.h"


#define BMP280_I2C_ADDR 	0x76	//7 bit address

#define BMP280_REG_ID		0xDO	// contains chip identification number
/*
#define
#define
#define
#define
#define
#define
*/

// the timeout function
static int timeout(volatile uint32_t *reg, uint16_t mask)
{
	uint8_t time = 250;	//rough guess of time 1000 iterations
	for(uint16_t t = 0; t <= time * 4; t++)
	{
		if((*reg & mask) == mask){return 1;}

	}
	return 0;
}

// i2c write function
int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
	volatile uint8_t dummy = 0;

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

	//dummy status register read(stm32 hardware quirk)
	//reading sr1 and sr2 as it is required to clear the addr flag(required by hardware)
	//unused values of dummy
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

const uint8_t address = 0x76;//BMP280 7 bit address
uint8_t buf[8];
volatile uint8_t dummy = 0;
//uint32_t
/*void read()
{
	//enabling ACK for acknowledgment of received data
	I2C1->CR1 |= 0x1 << 10;
	//starting generation
	I2C1->CR1 |= 0x1 << 8;



	//stopping generation
	I2C1->CR1 |= 0X1 << 9;
}*/



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




	// READING AND WRITING
	//enabling ACK for acknowledgment of received data
	I2C1->CR1 |= 0x1 << 10;
	//starting generation
	I2C1->CR1 |= 0x1 << 8;
	//waiting for start condition to be generated
	while(!(I2C1->SR1 & (1 << 0)));

	//waiting for DR to be empty, checking if TxE bit is set or not
	//while(!(I2C1->SR1 & (1 << 7)));
	//writing address to data register
	I2C1->DR = address << 1 | 0;
	//waiting for addr bit to set to finish transmitting address
	while(!(I2C1->SR1 & (1 << 1)));
	//dummy SR1 and SR2 read
	dummy = I2C1->SR1;
	dummy = I2C1->SR2;
	//writing the "ID" register address to the DR
	I2C1->DR = 0xD0;
	//waiting for byte transfer finished flag
	while(!(I2C1->SR1 & (1 << 2)));

	/*//waiting again for finishing transmit
	while(!(I2C1->SR1 & (1 << 1)));

	The ADDR flag is only set when a Slave Address match occurs on the bus.
	It will never set after you transmit a standard data byte like 0xD0.
	Waiting for ADDR here causes an infinite hang.
	*/
	//
	//generating a repeated start condition
	I2C1->CR1 |= 0x1 << 8;
	//waiting for start condition to be generated
	while(!(I2C1->SR1 & (1 << 0)));

	//waiting for DR to be empty, checking if TxE bit is set or not
	//while(!(I2C1->SR1 & (1 << 7)));

	//writing address to data register for read operation
	I2C1->DR = address << 1 | 1;
	//waiting for addr bit to set to finish transmitting address
	while(!(I2C1->SR1 & (1 << 1)));

	I2C1->CR1 &= ~(1<<10);  // clear the ACK bit
	//stopping generation



	dummy = I2C1->SR1;  // read SR1 and SR2 to clear the ADDR bit.... EV6 condition
	dummy = I2C1->SR2;	// sequential read better than reading at once
	//because stm32 requires to read sr1 and then sr2 strictly

	I2C1->CR1 |= 0X1 << 9; //stopping I2C
	/*//stopping generation
	I2C1->CR1 |= 0X1 << 9; //stopping I2C*/
	//ON STM32 hardware the stop bit must be set before clearing the addr flag


	while (!(I2C1->SR1 & (1<<6)));  // wait for RxNE to set receive not empty

	//reading data from data register
	buf[0] = I2C1->DR;		//should be 0x58

	I2C1->CR1 |= (1 << 10);//renabling ack for future transfers

	return 0;
}
