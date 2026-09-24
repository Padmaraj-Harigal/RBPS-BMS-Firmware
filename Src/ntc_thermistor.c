/**
 * @file    ntc_thermistor.c
 * @brief   RT1/RT2 (NCP18XH103F03RB) temperature conversion.
 */
#include "ntc_thermistor.h"
#include "bms_config.h"
#include <math.h>

float ntc_code_to_celsius(uint16_t adc_raw_code, const ntc_calib_t *calib)
{
    /* 1) Raw code -> divider voltage (volts), per BQ769x0 ADC equation:
     *    V = GAIN(uV/LSB) * code + OFFSET(mV)                            */
    float v_ts = ((float)calib->adc_gain_uv * (float)adc_raw_code) / 1000000.0f
                 + (calib->adc_offset_mv / 1000.0f);

    if (v_ts <= 0.0f || v_ts >= calib->regout_volts) {
        return -1000.0f; /* sentinel: open/shorted thermistor or bad read */
    }

    /* 2) Divider voltage -> thermistor resistance.
     *    TSx = REGOUT * Rntc / (Rntc + Rbias)  =>  solve for Rntc         */
    float r_ntc = calib->bias_resistor_ohms * v_ts / (calib->regout_volts - v_ts);

    /* 3) Beta equation: 1/T = 1/T25 + (1/B)*ln(R/R25)                    */
    float inv_t = (1.0f / NTC_T25_KELVIN)
                  + (1.0f / NTC_BETA_25_50) * logf(r_ntc / NTC_R25_OHMS);
    float temp_k = 1.0f / inv_t;

    return temp_k - 273.15f;
}
