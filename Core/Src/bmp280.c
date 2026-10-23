#include "bmp280.h"
#include "stdint.h"
#include "i2c.h"
#include "stm32f4xx.h"

BMP280_S32_t t_fine; // variable required for pressure calculations

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



int bmp280_init(void)
{
	uint8_t id = 0;

	if(i2c_read(BMP280_I2C_ADDR, BMP280_REG_ID, &id, 1) != I2C_OK){
		return -1;
		while(1){__NOP();}
	}

	if(id != 0x58)
		return -1;

	return 0;
}


int bmp280_read_raw(int32_t *raw_temp, int32_t *raw_pres)
{
	// reading raw values from the registers
	// defining variable to put the value in
	uint8_t val[6];
	// putting the sensor into normal mode by writing 11 into the ctrl_meas register into 0 and 1 bit
	if(i2c_write(BMP280_I2C_ADDR, BMP280_REG_CTRL_MEAS, 0xFF) != I2C_OK){
		return -1;
	}
	if(i2c_read(BMP280_I2C_ADDR, BMP280_REG_PRESS_MSB, val, 6) != I2C_OK){
		return -1;
	}


	*raw_pres = (val[0] << 12) | (val[1] << 4) | val[2] >> 4;
	*raw_temp = (val[3] << 12) | (val[4] << 4) | val[5] >> 4;
	return 0;

}

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

