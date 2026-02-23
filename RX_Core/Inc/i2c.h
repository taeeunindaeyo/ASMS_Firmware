#ifndef INC_I2C_H_
#define INC_I2C_H_

#include <stdint.h>

void i2c1_init(uint32_t i2c_hz, uint32_t pclk1_hz);
int  i2c1_write(uint8_t addr7, const uint8_t *buf, uint16_t len);
int  i2c1_ping(uint8_t addr7);

#endif
