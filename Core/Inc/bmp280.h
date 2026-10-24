#ifndef BMP280_H
#define BMP280_H

#include "stdint.h"

// BMP280 type definitions
typedef int32_t BMP280_S32_t;    // Signed 32-bit integer
typedef uint32_t BMP280_U32_t;   // Unsigned 32-bit integer
typedef int64_t BMP280_S64_t;    // Signed 64-bit integer

// error handling typedef
typedef enum {
    BMP280_OK = 0,
    BMP280_ERR_I2C,
    BMP280_ERR_ID,
    BMP280_ERR_NOT_READY
} bmp280_status_t;

// structure for the calibration block
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

// instance structure for a sensor
typedef struct {
	uint8_t addr;
	bmp280_calib_t calib;
	BMP280_S32_t t_fine;
} bmp280_t;



#define BMP280_I2C_ADDR 		0x76	// 7 bit address of bmp280

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
#define BMP280_REG_CALIB		0X88	// start of calibration block

void bmp280_parse_calib(const uint8_t *buf, bmp280_calib_t *c);
bmp280_status_t bmp280_init(bmp280_t *inst, uint8_t addr);
float getTemperature(bmp280_t *inst);
float getPressure(bmp280_t *inst);
bmp280_status_t bmp280_read_raw(bmp280_t *inst, int32_t *raw_temp, int32_t *raw_pres);
BMP280_S32_t bmp280_compensate_T_int32(bmp280_t *inst, BMP280_S32_t adc_T, bmp280_calib_t *c);
BMP280_U32_t bmp280_compensate_P_int64(bmp280_t *inst, BMP280_S32_t adc_P, bmp280_calib_t *c);

#endif
