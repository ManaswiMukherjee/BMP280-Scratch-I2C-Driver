#ifndef I2C_H
#define I2C_H

#include "stdint.h"
// enumerator for i2c status
typedef enum {
    I2C_OK            =  0,
    I2C_ERR_ARG       = -1,   /* bad length / null pointer */
    I2C_ERR_BUSY      = -2,   /* bus busy before START */
    I2C_ERR_TMO_SB    = -3,   /* START not generated */
    I2C_ERR_TMO_ADDR  = -4,   /* address phase never finished */
    I2C_ERR_TMO_TXE   = -5,   /* tx register/BTF never ready */
    I2C_ERR_TMO_RXNE  = -6,   /* rx data never arrived */
    I2C_ERR_NACK      = -7,   /* AF set: slave did not ACK */
} i2c_status_t;

static int timeout(volatile uint32_t *reg, uint16_t msk);
void i2c1_init(void);
void i2c_clear_ADDR(void);
i2c_status_t i2c_write(uint8_t addr, uint8_t reg, uint8_t value);
i2c_status_t i2c_read(uint8_t addr, uint8_t reg, uint8_t *value, uint32_t bytes);
#endif
