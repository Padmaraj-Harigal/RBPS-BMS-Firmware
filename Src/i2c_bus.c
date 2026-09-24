/**
 * @file    i2c_bus.c
 * @brief   I2C wrapper implementation (STM32C0 HAL, I2C1).
 *
 * NOTE: This assumes STM32CubeMX generated `hi2c1` in main.c/i2c.c and
 * that I2C1 is configured on PB6 (SCL) / PB7 (SDA). The current PCB
 * routes the BQ76940 SDA/SCL nets separately from these MCU nets, so
 * the physical bus will not work until the board is corrected.
 */
#include "i2c_bus.h"
#include "stm32c0xx_hal.h"   /* provided by STM32CubeC0 HAL package */

extern I2C_HandleTypeDef hi2c1;

#define I2C_BUS_TIMEOUT_MS   100U

void i2c_bus_init(void)
{
    /* HAL_I2C_Init(&hi2c1) is normally called from CubeMX-generated
     * MX_I2C1_Init(); nothing extra required here today. Kept as a
     * hook for future bus-recovery / clock-stretch handling. */
}

static i2c_bus_status_t map_hal_status(HAL_StatusTypeDef st)
{
    switch (st) {
        case HAL_OK:      return I2C_BUS_OK;
        case HAL_TIMEOUT:  return I2C_BUS_ERR_TIMEOUT;
        case HAL_ERROR:    return I2C_BUS_ERR_NACK;
        default:           return I2C_BUS_ERR_BUS;
    }
}

i2c_bus_status_t i2c_bus_write(uint8_t addr7, uint8_t reg,
                                const uint8_t *data, size_t len)
{
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(
        &hi2c1, (uint16_t)(addr7 << 1), reg,
        I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, (uint16_t)len,
        I2C_BUS_TIMEOUT_MS);
    return map_hal_status(st);
}

i2c_bus_status_t i2c_bus_read(uint8_t addr7, uint8_t reg,
                               uint8_t *data, size_t len)
{
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(
        &hi2c1, (uint16_t)(addr7 << 1), reg,
        I2C_MEMADD_SIZE_8BIT, data, (uint16_t)len,
        I2C_BUS_TIMEOUT_MS);
    return map_hal_status(st);
}

i2c_bus_status_t i2c_bus_write_byte(uint8_t addr7, uint8_t reg, uint8_t value)
{
    return i2c_bus_write(addr7, reg, &value, 1);
}

i2c_bus_status_t i2c_bus_read_byte(uint8_t addr7, uint8_t reg, uint8_t *value)
{
    return i2c_bus_read(addr7, reg, value, 1);
}
