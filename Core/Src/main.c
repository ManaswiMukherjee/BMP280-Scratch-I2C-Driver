/*
 * This is the implementation of an i2c driver for a bmp280 sensor starting from scratch
 * using CMSIS headers only.
 */

#include "stm32f4xx.h"
#include "stdint.h"
#include "string.h"


#define BMP280_I2C_ADDR 		0x76	//7 bit address

#define BMP280_REG_ID			0xD0	// contains chip identification number 0x58
#define BMP280_REG_RESET		0xE0	//
#define BMP280_REG_STATUS		0xF3	//
#define BMP280_REG_CTRL_MEAS	0xF4	//
#define BMP280_REG_CONFIG		0xF5	//
#define BMP280_REG_PRESS_MSB	0xF7	//
#define BMP280_REG_PRESS_LSB	0xF8	//
#define BMP280_REG_PRESS_XLSB	0xF9	//
#define BMP280_REG_TEMP_MSB		0xFA	//
#define BMP280_REG_TEMP_LSB		0xFB	//
#define BMP280_REG_TEMP_XLSB	0xFC	//


// BMP280 type definitions
typedef int32_t BMP280_S32_t;    // Signed 32-bit integer
typedef uint32_t BMP280_U32_t;   // Unsigned 32-bit integer
typedef int64_t BMP280_S64_t;    // Signed 64-bit integer

// struct for the calibration block
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_t;




// function to convert the calibration data into the struct
void bmp280_parse_calib(const uint8_t *buf, bmp280_calib_t *c)
{
    c->dig_T1 = (uint16_t)(buf[0]  | (buf[1]  << 8));
    c->dig_T2 = (int16_t)(buf[2]   | (buf[3]  << 8));
    c->dig_T3 = (int16_t)(buf[4]   | (buf[5]  << 8));

    c->dig_P1 = (uint16_t)(buf[6]  | (buf[7]  << 8));
    c->dig_P2 = (int16_t)(buf[8]   | (buf[9]  << 8));
    c->dig_P3 = (int16_t)(buf[10]  | (buf[11] << 8));
    c->dig_P4 = (int16_t)(buf[12]  | (buf[13] << 8));
    c->dig_P5 = (int16_t)(buf[14]  | (buf[15] << 8));
    c->dig_P6 = (int16_t)(buf[16]  | (buf[17] << 8));
    c->dig_P7 = (int16_t)(buf[18]  | (buf[19] << 8));
    c->dig_P8 = (int16_t)(buf[20]  | (buf[21] << 8));
    c->dig_P9 = (int16_t)(buf[22]  | (buf[23] << 8));
}



// the timeout function
/*
This function is used to check hardware status-registers against a known bit mask that we want to obtain
for suitable condition

Ex: Checking whether start condition is generated or not in SR1 after we set the start bit in CR1
(See first usage of timeout function in write function)
*/

static int timeout(volatile uint32_t *reg, uint16_t mask)
{
	uint16_t time = 10000;							// storing 250 in time variable as it is 8-bit variable
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
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	//writing i2c address to data register
	I2C1->DR = addr << 1 | 0;

	//waiting for ADDR bit to set to finish transmitting address
	if(timeout(&I2C1->SR1, (1 << 1)) == 0){
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
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
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	// writing data to the register
	I2C1->DR = value;

	//waiting for byte transfer finished flag for data write completion
	if(timeout(&I2C1->SR1, (1 << 2)) == 0){
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	I2C1->CR1 |= 0X1 << 9; 	// stopping I2C
	return 1;				// if everything goes well
}


// i2c read function
/*
 * addr - the i2c address of the bmp280
 * reg - the register address inside the chip
 * value - the value to write to the register
 */
int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value)
{
	volatile uint8_t dummy = 0;// dummy variable to read sr1 and sr2
	
	//enabling ACK for acknowledgment of received data
	I2C1->CR1 |= 0x1 << 10;

	//starting generation
	I2C1->CR1 |= 0x1 << 8;

	if(timeout(&I2C1->SR1, (1 << 0)) == 0){ // checking if start condition is generated
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	//writing i2c address to data register
	I2C1->DR = addr << 1 | 0;

	// waiting for ADDR bit to set to finish transmitting address
	if(timeout(&I2C1->SR1, (1 << 1)) == 0){
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	//dummy sr1 and sr2 read to clear ADDR
	dummy = I2C1->SR1;
	dummy = I2C1->SR2;

	//writing register value to the data register
	I2C1->DR = reg;

	//waiting for byte transfer finished flag for register address write completion
	if(timeout(&I2C1->SR1, (1 << 2)) == 0){
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	//repeated start condition
	I2C1->CR1 |= 0x1 << 8;

	if(timeout(&I2C1->SR1, (1 << 0)) == 0){ // checking if start condition is generated
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	//writing address to data register for read operation
	I2C1->DR = addr << 1 | 1;

	if(timeout(&I2C1->SR1, (1 << 1)) == 0){
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	I2C1->CR1 &= ~(1<<10);  // clear the ACK bit as we only need one byte

	//dummy sr1 and sr2 read to clear ADDR
	dummy = I2C1->SR1;
	dummy = I2C1->SR2;

	I2C1->CR1 |= 0X1 << 9; // stopping I2C

	if(timeout(&I2C1->SR1, (1<<6)) == 0){  	// wait for RxNE to set receive not empty
		I2C1->CR1 |= 0X1 << 9; 				// stopping I2C if timeout exceeds
		return 0;
	}
	*value = I2C1->DR;
	I2C1->CR1 |= (1 << 10);					//re-enabling ACK for future transfers

	return 1;
}

// i2c multi byte read function
/*
 * addr - the i2c address of the bmp280
 * reg - the register address inside the chip
 * value - the variable in which the value from the register is read
 * bytes - no of bytes to read
 */
int i2c_multi_read_reg(uint8_t addr, uint8_t reg, uint8_t *value, uint32_t bytes)
{
	if(!(bytes >= 3))
	{
		return 0;
	}
	volatile uint8_t dummy = 0;// dummy variable to read sr1 and sr2

	// enabling ACK for acknowledgment of received data
	I2C1->CR1 |= 0x1 << 10;

	// starting generation
	I2C1->CR1 |= 0x1 << 8;

	if(timeout(&I2C1->SR1, (1 << 0)) == 0){ // checking if start condition is generated
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	// writing i2c address to data register
	I2C1->DR = addr << 1 | 0;

	// waiting for ADDR bit to set to finish transmitting address
	if(timeout(&I2C1->SR1, (1 << 1)) == 0){
		I2C1->CR1 |= 0X1 << 9; //stopping I2C
		return 0;
	}

	// dummy sr1 and sr2 read to clear ADDR
	dummy = I2C1->SR1;
	dummy = I2C1->SR2;

	//writing register value to the data register
	I2C1->DR = reg;

	//waiting for byte transfer finished flag for register address write completion
	if(timeout(&I2C1->SR1, (1 << 2)) == 0){
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	// repeated start condition
	I2C1->CR1 |= 0x1 << 8;

	if(timeout(&I2C1->SR1, (1 << 0)) == 0){ // checking if start condition is generated
		I2C1->CR1 |= 0X1 << 9; 				// stopping I2C if timeout exceeds
		return 0;
	}

	// writing address to data register for read operation
	I2C1->DR = addr << 1 | 1;

	if(timeout(&I2C1->SR1, (1 << 1)) == 0){
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	// clearing ADDR without clearing ACK, this is one difference in multiple byte read
	// dummy sr1 and sr2 read to clear ADDR
	dummy = I2C1->SR1;
	dummy = I2C1->SR2;

	// bulk byte read until n-3
	uint32_t remaining = bytes;
	// this loop is to run until when 3 bytes are left
	while(remaining > 3)
	{
		// RxNE is set when data register has data, so checking
		if(timeout(&I2C1->SR1, (1<<6)) == 0){  	// wait for RxNE to set receive not empty
			I2C1->CR1 |= 0X1 << 9; 				// stopping I2C if timeout exceeds
			return 0;
		}
		value[ bytes - remaining ] = I2C1->DR;
		remaining--; // decrementing remaining when a byte is read
	}

	// last 3 bytes
	//waiting for byte transfer finished flag to apply configurations for next transfer
	if(timeout(&I2C1->SR1, (1 << 2)) == 0){
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	I2C1->CR1 &= ~(1<<10);  // clear the ACK bit so that a NACK is send for the last byte

	// reading n-2 byte
	value[ bytes - remaining ] =  I2C1->DR;
	remaining--;

	//waiting for byte transfer finished flag to apply configurations for next transfer
	if(timeout(&I2C1->SR1, (1 << 2)) == 0){
		I2C1->CR1 |= 0X1 << 9; 				//stopping I2C if timeout exceeds
		return 0;
	}

	I2C1->CR1 |= 0X1 << 9; // stopping I2C

	// reading n-1 byte
	value[ bytes - remaining ] =  I2C1->DR;
	remaining--;

	if(timeout(&I2C1->SR1, (1<<6)) == 0){  	// wait for RxNE to set receive not empty
		I2C1->CR1 |= 0X1 << 9; 				// stopping I2C if timeout exceeds
		return 0;
	}
	// reading last byte
	value[ bytes - remaining ] =  I2C1->DR;
	remaining--;

	// re-enabling ACK for future transfers
	I2C1->CR1 |= (1 << 10);

	return 1;
}

int bmp280_read_raw(int32_t *raw_temp, int32_t *raw_pres)
{
	// reading raw values from the registers
	// defining variable to put the value in
	uint8_t val[6];
	// putting the sensor into normal mode by writing 11 into the ctrl_meas register into 0 and 1 bit
	if(i2c_write_reg(BMP280_I2C_ADDR, BMP280_REG_CTRL_MEAS, 0x27) == 0){
		return 0;
	}
	if(i2c_multi_read_reg(BMP280_I2C_ADDR, BMP280_REG_PRESS_MSB, val, 6) == 0){
		return 0;
	}


	*raw_pres = (val[0] << 12) | (val[1] << 4) | val[2] >> 4;
	*raw_temp = (val[3] << 12) | (val[4] << 4) | val[5] >> 4;
	return 1;

}

BMP280_S32_t t_fine;
BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T, bmp280_calib_t *c)
{
	BMP280_S32_t var1, var2, T;
	var1 = ((((adc_T >> 3) - ((BMP280_S32_t)c->dig_T1 << 1))) * ((BMP280_S32_t)c->dig_T2)) >> 11;
	var2 = (((((adc_T >> 4) - ((BMP280_S32_t)c->dig_T1)) * ((adc_T >> 4) - ((BMP280_S32_t)c->dig_T1))) >> 12) *((BMP280_S32_t)c->dig_T3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
}

BMP280_U32_t bmp280_compensate_P_int64(BMP280_S32_t adc_P, bmp280_calib_t *c)
{
	BMP280_S64_t var1, var2, p;
	var1 = ((BMP280_S64_t)t_fine) - 128000;
	var2 = var1 * var1 * (BMP280_S64_t)c->dig_P6;
	var2 = var2 + ((var1*(BMP280_S64_t)c->dig_P5) << 17);
	var2 = var2 + (((BMP280_S64_t)c->dig_P4) << 35);
	var1 = ((var1 * var1 * (BMP280_S64_t)c->dig_P3)>>8) + ((var1 * (BMP280_S64_t)c->dig_P2) << 12);
	var1 = (((((BMP280_S64_t)1) << 47) + var1)) * ((BMP280_S64_t)c->dig_P1) >> 33;
	if (var1 == 0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576 - adc_P;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((BMP280_S64_t)c->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	var2 = (((BMP280_S64_t)c->dig_P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((BMP280_S64_t)c->dig_P7) << 4);
	return (BMP280_U32_t)p;
}

int main()
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


	/*
	// test for the read function
	uint8_t id = 0;

	if(!i2c_read_reg(BMP280_I2C_ADDR, BMP280_REG_ID, &id)){
		while(1){__NOP();}
	}
	*/

	/*
	//test for the write function
	uint8_t ctrl_meas_status;
	if(!i2c_write_reg(BMP280_I2C_ADDR, BMP280_REG_CTRL_MEAS, 0x03)){
		while(1){__NOP();}
	}
	if(!i2c_read_reg(BMP280_I2C_ADDR, BMP280_REG_CTRL_MEAS, &ctrl_meas_status)){
		while(1){__NOP();}
	}
	*/


	/*
	// test for the multiple byte read function
	uint8_t buf[24];
	uint8_t flag = 0;
	bmp280_calib_t calib;

	for (int i = 0; i < 24; i++) {
	    buf[i] = 7;
	}

	if(i2c_multi_read_reg(BMP280_I2C_ADDR, 0x88, buf, 24))
	{
		flag = 0;
		bmp280_parse_calib(buf, &calib);
		while(1){__NOP();}
	}
	else{flag = 1;}
	*/
	/*
	int8_t flag = 0,op = 0;
	int32_t temp = 0, pres = 0;
	while(1){
		op = bmp280_read_raw(&temp, &pres);
		if(op == 0){flag = 1;}
		else if(op == 2){flag = 2;}
		else if(op == 3){flag = 3;}
		else{flag = 0;}
	}
	*/
	int8_t flag = 0;
	int32_t temp = 0, pres = 0;
	uint8_t buf[24];
	bmp280_calib_t cali;
	bmp280_read_raw(&temp, &pres);
	if(!i2c_multi_read_reg(BMP280_I2C_ADDR, 0x88, buf, 24)){
		flag = 1;
	}
	bmp280_parse_calib(buf, &cali);

	float final_temp = 0, final_pres = 0;
	final_temp = bmp280_compensate_T_int32(temp, &cali);
	final_pres = bmp280_compensate_P_int64(pres, &cali);
	final_pres = final_pres / 256 / 100;

	while(1){__NOP();}
}
