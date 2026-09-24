/**
 * @file    bq76940.c
 * @brief   BQ76940 (U1) driver implementation.
 */
#include "bq76940.h"
#include "bq76940_regs.h"
#include "i2c_bus.h"
#include "ntc_thermistor.h"
#include <string.h>

static ntc_calib_t s_ntc_calib = {
    .adc_gain_uv       = 365,   /* placeholder mid-range; overwritten at init */
    .adc_offset_mv     = 0,
    .regout_volts      = 3.3f,
    .bias_resistor_ohms = 10000.0f
};

static bq_status_t chk(i2c_bus_status_t st)
{
    return (st == I2C_BUS_OK) ? BQ_OK : BQ_ERR_I2C;
}

/* Decode factory-trimmed ADC gain/offset (SLUSBK2 section 8.5.2). */
static bq_status_t read_adc_calibration(void)
{
    uint8_t gain1 = 0, gain2 = 0, off = 0;
    i2c_bus_status_t st;

    st = i2c_bus_read_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_ADCGAIN1, &gain1);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;
    st = i2c_bus_read_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_ADCGAIN2, &gain2);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;
    st = i2c_bus_read_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_ADCOFFSET, &off);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;

    /* ADCGAIN<4:3> in ADCGAIN1[3:2], ADCGAIN<2:0> in ADCGAIN2[7:5].
     * GAIN (uV/LSB) = 365 + ADCGAIN<4:0>. OFFSET is signed, in mV. */
    uint8_t gain_hi = (gain1 >> 2) & 0x03U;
    uint8_t gain_lo = (gain2 >> 5) & 0x07U;
    uint8_t gain_field = (uint8_t)((gain_hi << 3) | gain_lo);

    s_ntc_calib.adc_gain_uv   = (int16_t)(365 + gain_field);
    s_ntc_calib.adc_offset_mv = (int8_t)off; /* two's complement, mV */

    return BQ_OK;
}

/* Translate desired cell thresholds to the BQ76940's calibrated ADC
 * threshold registers. OV is rounded down to avoid allowing a higher
 * voltage than requested; UV is rounded up to trip no lower than requested. */
static bq_status_t program_voltage_protection(void)
{
    int32_t gain = s_ntc_calib.adc_gain_uv;
    int32_t ov_num = ((int32_t)BMS_CELL_OV_MILLIVOLT -
                      s_ntc_calib.adc_offset_mv) * 1000;
    int32_t uv_num = ((int32_t)BMS_CELL_UV_MILLIVOLT -
                      s_ntc_calib.adc_offset_mv) * 1000;

    if (gain <= 0 || ov_num <= 0 || uv_num <= 0 ||
        BMS_CELL_UV_MILLIVOLT >= BMS_CELL_OV_MILLIVOLT) {
        return BQ_ERR_I2C;
    }

    uint32_t ov_adc = (uint32_t)(ov_num / gain);
    uint32_t uv_adc = (uint32_t)((uv_num + gain - 1) / gain);
    /* OV's low four bits are fixed to 1000; choose the greatest supported
     * threshold not above the requested voltage. UV's low four bits are
     * fixed to 0000; round upward so protection is not weaker than requested. */
    uint32_t ov_full = ((ov_adc - 8U) & ~15U) | 8U;
    uint32_t uv_full = (uv_adc + 15U) & ~15U;

    /* OV has fixed ADC bits 13:12 = 10; UV has 01. Reject thresholds
     * outside the representable ranges instead of silently wrapping. */
    if (ov_adc < 0x2008U || ov_full > 0x2FF8U ||
        uv_full < 0x1000U || uv_full > 0x1FF0U) {
        return BQ_ERR_I2C;
    }

    i2c_bus_status_t st = i2c_bus_write_byte(
        BQ76940_I2C_7BIT_ADDR, BQ_REG_OV_TRIP,
        (uint8_t)((ov_full >> 4) & 0xFFU));
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;

    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_UV_TRIP,
                            (uint8_t)((uv_full >> 4) & 0xFFU));
    return chk(st);
}

bq_status_t bq76940_init(void)
{
    i2c_bus_status_t st;

    i2c_bus_init();

    /* Put both FET controls off before changing the AFE configuration. */
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_SYS_CTRL2,
                             BQ_SYS_CTRL2_CC_EN);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;

    /* Clear latched status bits. SYS_STAT is write-one-to-clear. */
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_SYS_STAT,
                             BQ_SYS_STAT_FAULT_MASK | BQ_SYS_STAT_OVRD_ALERT |
                             BQ_SYS_STAT_CC_READY);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;

    /* Required by TI for correct coulomb-counter operation. */
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_CC_CFG,
                             BQ_CC_CFG_VALUE);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;

    /* Enable the cell-voltage/current ADC and select external thermistor
     * inputs (RT1/RT2 on TS1/TS2), plus enable the Coulomb counter. */
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_SYS_CTRL1,
                             BQ_SYS_CTRL1_ADC_EN | BQ_SYS_CTRL1_TEMP_SEL);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;

    /* Explicitly retain the datasheet reset protection thresholds/delays.
     * These are conservative defaults, not application-rated current limits. */
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_PROTECT1,
                             BQ_PROTECT1_DEFAULT);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_PROTECT2,
                             BQ_PROTECT2_DEFAULT);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_PROTECT3,
                             BQ_PROTECT3_DEFAULT);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;

    bq_status_t bst = read_adc_calibration();
    if (bst != BQ_OK) return bst;

    bst = program_voltage_protection();
    if (bst != BQ_OK) return bst;

    /* Discard any conversion that completed during initialization. */
    return chk(i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_SYS_STAT,
                                  BQ_SYS_STAT_CC_READY));
}

static uint16_t raw_to_millivolts(uint16_t raw14)
{
    /* Cell-voltage ADC equation: V(uV) = GAIN * code + OFFSET(mV)*1000 */
    int32_t uv = (int32_t)s_ntc_calib.adc_gain_uv * (int32_t)raw14
                 + (int32_t)s_ntc_calib.adc_offset_mv * 1000;
    if (uv < 0) uv = 0;
    return (uint16_t)(uv / 1000);
}

static bq_status_t read_cell_voltages(bq_pack_data_t *out)
{
    uint32_t pack_mv_sum = 0;

    for (uint8_t i = 0; i < BMS_NUM_CELLS_WIRED; i++) {
        uint8_t buf[2];
        uint8_t reg = (uint8_t)(BQ_REG_VC1_HI + i * BQ_CELL_REG_STRIDE);

        i2c_bus_status_t st = i2c_bus_read(BQ76940_I2C_7BIT_ADDR, reg, buf, 2);
        if (st != I2C_BUS_OK) return BQ_ERR_I2C;

        uint16_t raw14 = (uint16_t)(((buf[0] & 0x3FU) << 8) | buf[1]);
        uint16_t mv = raw_to_millivolts(raw14);

        out->cell_mv[i] = mv;
        pack_mv_sum += mv;
    }
    out->pack_mv = (uint16_t)pack_mv_sum;
    return BQ_OK;
}

static bq_status_t read_pack_current(bq_pack_data_t *out)
{
    uint8_t buf[2];
    i2c_bus_status_t st = i2c_bus_read(BQ76940_I2C_7BIT_ADDR, BQ_REG_CC_HI, buf, 2);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;

    int16_t cc_raw = (int16_t)((buf[0] << 8) | buf[1]); /* signed */

    /* Current in mA is CC_raw * 8.44 / Rshunt(mOhm). Use integer
     * hundredths and symmetric rounding to avoid float truncation (which
     * otherwise turns 1000 counts at 1 mOhm into 8439 instead of 8440 mA). */
    int32_t denominator = 100 * BMS_SHUNT_RESISTANCE_MOHM;
    int32_t numerator = (int32_t)cc_raw * 844;
    if (denominator <= 0) return BQ_ERR_I2C;
    numerator += (numerator >= 0) ? (denominator / 2) : -(denominator / 2);
    out->pack_current_ma = numerator / denominator;
    return BQ_OK;
}

static bq_status_t read_temperatures(bq_pack_data_t *out)
{
    uint8_t buf[2];
    i2c_bus_status_t st;

    st = i2c_bus_read(BQ76940_I2C_7BIT_ADDR, BQ_REG_TS1_HI, buf, 2);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;
    uint16_t ts1_raw = (uint16_t)(((buf[0] & 0x3FU) << 8) | buf[1]);
    out->temp1_c = ntc_code_to_celsius(ts1_raw, &s_ntc_calib);

    st = i2c_bus_read(BQ76940_I2C_7BIT_ADDR, BQ_REG_TS2_HI, buf, 2);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;
    uint16_t ts2_raw = (uint16_t)(((buf[0] & 0x3FU) << 8) | buf[1]);
    out->temp2_c = ntc_code_to_celsius(ts2_raw, &s_ntc_calib);

    return BQ_OK;
}

static void decode_sys_stat(bq_pack_data_t *out, uint8_t stat)
{
    out->sys_stat            = stat;
    out->fault_ov             = (stat & BQ_SYS_STAT_OV) != 0;
    out->fault_uv             = (stat & BQ_SYS_STAT_UV) != 0;
    out->fault_ocd            = (stat & BQ_SYS_STAT_OCD) != 0;
    out->fault_scd            = (stat & BQ_SYS_STAT_SCD) != 0;
    out->fault_device_xready  = (stat & BQ_SYS_STAT_DEVICE_XREADY) != 0;
}

bq_status_t bq76940_read_all(bq_pack_data_t *out)
{
    memset(out, 0, sizeof(*out));

    uint8_t stat = 0;
    i2c_bus_status_t st = i2c_bus_read_byte(BQ76940_I2C_7BIT_ADDR,
                                             BQ_REG_SYS_STAT, &stat);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;
    decode_sys_stat(out, stat);

    if (!(stat & BQ_SYS_STAT_CC_READY)) {
        /* A conversion just hasn't completed yet - not fatal, caller can
         * retry on the next scheduler tick. */
        return BQ_ERR_NOT_READY;
    }

    /* CC_READY is latched until explicitly cleared. Clear it immediately
     * after taking the status snapshot so the next poll waits for a new
     * 250 ms coulomb-counter conversion instead of reusing stale current. */
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_SYS_STAT,
                             BQ_SYS_STAT_CC_READY);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;

    bq_status_t bst;
    if ((bst = read_cell_voltages(out)) != BQ_OK) return bst;
    if ((bst = read_pack_current(out))  != BQ_OK) return bst;
    if ((bst = read_temperatures(out))  != BQ_OK) return bst;

    return BQ_OK;
}

bq_status_t bq76940_clear_faults(void)
{
    return chk(i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_SYS_STAT,
                                   BQ_SYS_STAT_FAULT_MASK));
}

bool bq76940_has_fault(const bq_pack_data_t *data)
{
    return data->fault_ov || data->fault_uv || data->fault_ocd ||
           data->fault_scd || data->fault_device_xready;
}

static bq_status_t modify_sys_ctrl2(uint8_t set_mask, uint8_t clear_mask)
{
    uint8_t val = 0;
    i2c_bus_status_t st = i2c_bus_read_byte(BQ76940_I2C_7BIT_ADDR,
                                             BQ_REG_SYS_CTRL2, &val);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;

    val = (uint8_t)((val & ~clear_mask) | set_mask);

    return chk(i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_SYS_CTRL2, val));
}

bq_status_t bq76940_set_charge_fet(bool enable)
{
    return enable ? modify_sys_ctrl2(BQ_SYS_CTRL2_CHG_ON, 0)
                  : modify_sys_ctrl2(0, BQ_SYS_CTRL2_CHG_ON);
}

bq_status_t bq76940_set_discharge_fet(bool enable)
{
    return enable ? modify_sys_ctrl2(BQ_SYS_CTRL2_DSG_ON, 0)
                  : modify_sys_ctrl2(0, BQ_SYS_CTRL2_DSG_ON);
}

bq_status_t bq76940_set_balance_mask(uint16_t cell_bitmask)
{
    /* CELLBAL1 = cells 1-5, CELLBAL2 = cells 6-10, CELLBAL3 = cells 11-15.
     * NOTE: per datasheet, balancing FETs must not be enabled on the same
     * cell as an ongoing ADC conversion used for protection decisions -
     * a production firmware should pause balancing before fault-critical
     * reads. Kept simple here. */
    uint8_t b1 = (uint8_t)(cell_bitmask & 0x1FU);
    uint8_t b2 = (uint8_t)((cell_bitmask >> 5) & 0x1FU);
    uint8_t b3 = (uint8_t)((cell_bitmask >> 10) & 0x1FU);

    i2c_bus_status_t st;
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_CELLBAL1, b1);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_CELLBAL2, b2);
    if (st != I2C_BUS_OK) return BQ_ERR_I2C;
    st = i2c_bus_write_byte(BQ76940_I2C_7BIT_ADDR, BQ_REG_CELLBAL3, b3);
    return chk(st);
}
