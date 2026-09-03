# DAY-1 23/08/2026

### Some part of the code has been written previously but `CHANGELOG` starts from 23/08/26
## 1. Putting defined numbers for addresses instead of hardcoding in the middle of nowhere

## 2. Making a timeout function to check if bits are set is status registers
### 1st try
```c
static int timeout(*reg, uint8_t mask)
{
	time = 1000;	//rough guess of time
	for(int t = 0; t < 1000; t++)
	{
		if(&reg == mask){return 1;}
		time --;
	}
	return 0;
}
```

Problems with the above code block
1. `*reg` in the parameter list has no type. Also hardware registers are volatile.
* Fix - Changed the type of `*reg` parameter to volatile uint8_t.
2. `if(&reg == mask)` does not check.
`&var` gives address of a variable not it's contents.
* Fix - Changed `&` to `*` for contents
3. `time` and `t` are doing the same job, and time is never actually used.\
Plus time has no data time when it is being declared.
* Fix - Used time and fixed issues with t.
4. No include for data types like uint8_t.
* Fix - Added the line `include #stdint.h"`

```c
static int timeout(volatile uint8_t *reg, uint8_t mask)
{
	uint8_t time = 1000;	//rough guess of time
	for(uint8_t t = 0; t <= time; t++)
	{
		if(*reg == mask){return 1;}

	}
	return 0;
}
```
### 2nd try
```c
static int timeout(volatile uint8_t *reg, uint8_t mask)
{
	uint8_t time = 1000;	//rough guess of time
	for(uint8_t t = 0; t <= time; t++)
	{
		if(*reg == mask){return 1;}

	}
	return 0;
}
```
Problems with the above code block
1. `time` overflows immediately
* Fix - put `t` as 250 and multiply by 4
2. `*reg == mask` still checks for exact equality not if the bit is set.
* Fix - Do `(*reg | mask) == 0`.
3. One more thing to check, not a syntax bug but a correctness one: what's the actual C type of `I2C1->SR1` in the CMSIS header?
* Fix - `I2C1->SR1` is 16 bit wide so we should use `uint16_t` for the register and the mask.

### 3rd try
```c
static int timeout(volatile uint16_t *reg, uint16_t mask)
{
	uint8_t time = 250;	//rough guess of time 1000 iterations
	for(uint8_t t = 0; t <= time * 4; t++)
	{
		if((*reg | mask) == 0){return 1;}

	}
	return 0;
}
```
Problems in the above code block
1. `t` is `uint8_t` but you're comparing it against time * 4 (which can be up to 1000).
uint8_t maxes out at 255. So t <= time * 4 is comparing a value that can never exceed 255 against a target of up to 1000
* Fix - change the data type of t to `uint16_t`
2. 2. The bitwise logic is backwards.
* Fix - The logical operation will be `&`.
3. Checked the actual register type in the reference manual

### 4th try
```c
static int timeout(volatile uint16_t *reg, uint16_t mask)
{
	uint8_t time = 250;	//rough guess of time 1000 iterations
	for(uint16_t t = 0; t <= time * 4; t++)
	{
		if((*reg & mask) == 0){return 1;}

	}
	return 0;
}
```
1. `(*reg & mask) == 0` is true when the bit is not set.\
Fix - change to `(*reg & mask) == 1`.

### The final line is `(*reg & mask) == mask` as we want to check if the specific bit is set or not.

## 2. Making a write function for i2c `i2c_write_reg`

### 1. I2C1->DR = BMP280_I2C_ADDR << 1 | 0; shows error
* Fix - for defines ; should not be used\
changing `#define BMP280_I2C_ADDR 	0x76;` to `#define BMP280_I2C_ADDR 	0x76`

### 2. The `timeout` function accepts pointer not values of registers\
### The CMSIS definitions are of `uint32_t` although the register is 16bit wide
* Fix - Changing the `*reg` parameter in timeout function

### 1st try
```c
void i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
	//enabling ACK for acknowledgment of received data
	I2C1->CR1 |= 0x1 << 10;

	//starting generation
	I2C1->CR1 |= 0x1 << 8;

	if(timeout(&I2C1->SR1, (1 << 0)) == 0){} // checking if start condition is generated

	//writing address to data register
	I2C1->DR = BMP280_I2C_ADDR << 1 | 0;

	//waiting for addr bit to set to finish transmitting address
	//while(!(I2C1->SR1 & (1 << 1)));
	if(timeout(&I2C1->SR1, (1 << 1)) == 0){}
	
	//dummy status register read
	dummy = I2C1->SR1;
	dummy = I2C1->SR2;
	
	//writing the "ID" register address to the DR
	I2C1->DR = BMP280_REG_ID;

	//waiting for byte transfer finished flag
	timeout(I2C1->SR1, (1 << 2));
	
}
```
Problems in the above code
1. The if(timeout(...) == 0){} pattern does nothing.
* Fix - The check should return 0 if the condition is not passed.
2. Last timeout() call has regressed — missing the & again.
* Fix - Added `&` for akvddress.
3. The function ignores its own parameters.
* Fix - Used function parameters instead of hardcoded values where required.
4. value is never written anywhere.
* Fix - Same as above. Use function parameters.
5. No STOP condition.
* Fix - Add stop condition
6. Return type: void vs int.
* Fix - Add stop condition.

7. Did not write `value` to DR. `value` parameter unused.
* Fix - add write `value` to DR line and check BTF(byte transfer flag)

### 8. At fix in problem 1 we `return`ed from the write function without stopping i2c


# DAY-2 29/08/2026

### Added write function to the code. Works as intended.

# DAY-3 03/09/2026

## Why repeated start?
### We make a repeated start condition in `i2c_reg_read` because after we write to the address, the sensor's internal pointer is set to the address of the register `reg`. Now as we want to read from the address we have to send the 7-bit address along with a write(`1`) as we cannot change from write to read mid transmission and have to transmit the address all over again but this time the internal pointer of the sensor is set to the desired register address from where we want to read. 
### And repeated start avoids releasing the bus while changing from read to write. 

## Why ACK is disabled for 1 byte read?
### For reading only one byte, we disable NACK so as the sensor does not keep sending more data
### Meaning `ACK - byte received send more` and `NACK - byte received do not send more`

## Why dummy read is required?
### When the I2C1 peripheral finishes shifting out the address byte and sees an ACK from the slave, it sets ADDR (SR1 bit 1) to tell that "the address phase is done, direction is locked in." Until this is acknowledged in software, the peripheral stretches SCL low — the whole bus is stalled waiting.
### And in stm32 hardware, the addr can only be cleared by reading the status in a specific sequence SR1 then SR2.