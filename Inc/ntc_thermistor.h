/**
 * @file    ntc_thermistor.h
 * @brief   Converts BQ76940 TS1/TS2 ADC readings (RT1/RT2 thermistors on
 *          the schematic) into degrees Celsius.
 *
 * The BQ76940 biases each TSx pin through an internal ~10k resistor from
 * its REGOUT (LDO) rail, then digitizes the divider voltage with its
 * 14-bit ADC (same ADC used for cell voltages, same gain/offset trim).
 * We reconstruct the thermistor resistance from that voltage, then apply
 * the Beta equation for the NCP18XH103F03RB (10k @ 25C, B25/50=3380K).
 */
#ifndef NTC_THERMISTOR_H
#define NTC_THERMISTOR_H

#include <stdint.h>

typedef struct {
    int16_t adc_gain_uv;     /* from BQ76940 ADCGAIN1/ADCGAIN2, in uV/LSB */
    int16_t adc_offset_mv;   /* from BQ76940 ADCOFFSET, in mV             */
    float   regout_volts;    /* REGOUT LDO rail biasing the TS divider,
                                 nominally 3.3 V - verify on your board   */
    float   bias_resistor_ohms; /* internal BQ76940 TS bias, nominally 10k */
} ntc_calib_t;

/**
 * Convert a raw 14-bit TSx ADC code to temperature in Celsius.
 * Returns 0xFFFF-safe float; caller should range-check (-40..125 C).
 */
float ntc_code_to_celsius(uint16_t adc_raw_code, const ntc_calib_t *calib);

#endif /* NTC_THERMISTOR_H */
