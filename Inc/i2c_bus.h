/**
 * @file    i2c_bus.h
 * @brief   Thin I2C wrapper used by the BQ76940 driver.
 *          Implemented on top of STM32 HAL I2C1 (intended PB7/PB6).
 *          Current PCB does not route BQ76940 SDA/SCL to those pads.
 *          Swap i2c_bus.c internals if you move
 *          to LL drivers or a different MCU family.
 */
#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    I2C_BUS_OK = 0,
    I2C_BUS_ERR_TIMEOUT,
    I2C_BUS_ERR_NACK,
    I2C_BUS_ERR_BUS
} i2c_bus_status_t;

/* Call once at startup after HAL_I2C_Init() / MX_I2C1_Init(). */
void i2c_bus_init(void);

/* Write `len` bytes from `data` starting at register `reg` on device `addr7`. */
i2c_bus_status_t i2c_bus_write(uint8_t addr7, uint8_t reg,
                                const uint8_t *data, size_t len);

/* Read `len` bytes into `data` starting at register `reg` on device `addr7`. */
i2c_bus_status_t i2c_bus_read(uint8_t addr7, uint8_t reg,
                               uint8_t *data, size_t len);

/* Convenience single-byte helpers. */
i2c_bus_status_t i2c_bus_write_byte(uint8_t addr7, uint8_t reg, uint8_t value);
i2c_bus_status_t i2c_bus_read_byte(uint8_t addr7, uint8_t reg, uint8_t *value);

#endif /* I2C_BUS_H */
