/*
 * This is the example usage for the implementation of an i2c driver for a bmp280 sensor
 * starting from scratch using CMSIS headers only.
 * NO HAL
 */

#include "stm32f4xx.h"
#include <stdint.h>
#include <string.h>
#include "bmp280.h"
#include "i2c.h"

int main()
{
	int err = 0;
	//uint8_t  id = 0;
	//initializing i2c
	i2c1_init();

	// creating one instance
	bmp280_t sensor1;
	// initializing the sensor
	err = bmp280_init(&sensor1, BMP280_I2C_ADDR);
	if(err != 0){
		while(1) __NOP();
	}
	sensor1.addr = BMP280_I2C_ADDR;

	float temp = 0, pres = 0;

	temp = getTemperature(&sensor1);
	pres = getPressure(&sensor1);

}
