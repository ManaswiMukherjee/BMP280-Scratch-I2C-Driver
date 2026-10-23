/*
 * This is the example usage for the implementation of an i2c driver for a bmp280 sensor
 * starting from scratch using CMSIS headers only.
 * NO HAL
 */

#include "stm32f4xx.h"
#include "stdint.h"
#include "string.h"
#include "bmp280.h"
#include "i2c.h"

int main()
{
	i2c1_init();
	bmp280_init();
	int8_t flag = 0;
	int32_t temp = 0, pres = 0;
	uint8_t buf[24];
	bmp280_calib_t cali;
	bmp280_read_raw(&temp, &pres);
	if(!i2c_read(BMP280_I2C_ADDR, 0x88, buf, 24)){
		flag = 1;
	}
	bmp280_parse_calib(buf, &cali);

	float final_temp = 0, final_pres = 0;
	final_temp = bmp280_compensate_T_int32(temp, &cali);
	final_pres = bmp280_compensate_P_int64(pres, &cali);
	final_pres = final_pres / 256 / 100;


	while(1){__NOP();}
}
