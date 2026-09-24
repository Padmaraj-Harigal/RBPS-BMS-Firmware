/**
 * @file    protection.c
 * @brief   Software protection policy (see protection.h).
 */
#include "protection.h"
#include "bms_config.h"
#include "led_status.h"
#include <math.h>

static pack_state_t s_state = PACK_STATE_NORMAL;

static bool any_cell_out_of_window(const bq_pack_data_t *d)
{
    for (uint8_t i = 0; i < BMS_NUM_CELLS_WIRED; i++) {
        if (d->cell_mv[i] >= BMS_CELL_OV_MILLIVOLT ||
            d->cell_mv[i] <= BMS_CELL_UV_MILLIVOLT) {
            return true;
        }
    }
    return false;
}

static bool temp_out_of_window(const bq_pack_data_t *d)
{
    /* ntc_code_to_celsius() uses -1000 C for an open/shorted probe or an
     * invalid ADC voltage. Never interpret that sentinel as a real temp. */
    if (!isfinite(d->temp1_c) || !isfinite(d->temp2_c) ||
        d->temp1_c < -40.0f || d->temp1_c > 125.0f ||
        d->temp2_c < -40.0f || d->temp2_c > 125.0f) {
        return true;
    }

    bool charging = d->pack_current_ma > 0;
    float hottest = (d->temp1_c > d->temp2_c) ? d->temp1_c : d->temp2_c;
    float coldest = (d->temp1_c < d->temp2_c) ? d->temp1_c : d->temp2_c;

    if (charging && hottest >= (float)BMS_OTP_CHARGE_C)    return true;
    if (!charging && hottest >= (float)BMS_OTP_DISCHARGE_C) return true;
    if (charging && coldest <= (float)BMS_UTP_CHARGE_C)     return true;
    return false;
}

pack_state_t protection_evaluate(const bq_pack_data_t *data)
{
    bool hw_fault = bq76940_has_fault(data);
    bool sw_fault = any_cell_out_of_window(data) || temp_out_of_window(data);

    if (hw_fault || sw_fault) {
        s_state = PACK_STATE_FAULT_LATCHED;
        (void)bq76940_set_charge_fet(false);
        (void)bq76940_set_discharge_fet(false);
        led_fault_set(true);
    } else if (s_state == PACK_STATE_NORMAL && BMS_ALLOW_FET_ENABLE) {
        /* Only auto-enable FETs while already in a known-good state;
         * recovering from a fault requires protection_attempt_reset(). */
        if (bq76940_set_charge_fet(true) != BQ_OK ||
            bq76940_set_discharge_fet(true) != BQ_OK) {
            /* A failed FET command is itself a fault. Try to leave both
             * outputs disabled and latch the fault for explicit recovery. */
            s_state = PACK_STATE_FAULT_LATCHED;
            (void)bq76940_set_charge_fet(false);
            (void)bq76940_set_discharge_fet(false);
            led_fault_set(true);
        } else {
            led_fault_set(false);
        }
    }

    return s_state;
}

bq_status_t protection_attempt_reset(void)
{
    bq_status_t st = bq76940_clear_faults();
    if (st == BQ_OK) {
        s_state = PACK_STATE_NORMAL;
        led_fault_set(false);
    }
    return st;
}
