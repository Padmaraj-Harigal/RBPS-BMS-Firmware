/**
 * @file    bq76940.h
 * @brief   Driver for U1 (BQ76940DBT) - the actual "sensor MCU/IC" on the
 *          EMNODE/RBPS board. Provides cell voltages (VC0..VC12), pack
 *          current (via R16 shunt / Coulomb counter), temperature
 *          (RT1/RT2 via TS1/TS2), and charge/discharge FET control
 *          (Q1/Q2, Q4/Q5).
 */
#ifndef BQ76940_H
#define BQ76940_H

#include <stdint.h>
#include <stdbool.h>
#include "bms_config.h"

typedef enum {
    BQ_OK = 0,
    BQ_ERR_I2C,
    BQ_ERR_NOT_READY
} bq_status_t;

typedef struct {
    uint16_t cell_mv[BMS_NUM_CELLS_WIRED]; /* per-cell voltage, millivolts */
    uint16_t pack_mv;                      /* total pack voltage, mV       */
    int32_t  pack_current_ma;              /* signed: + = charge, - = discharge */
    float    temp1_c;                      /* RT1, e.g. near FETs/shunt    */
    float    temp2_c;                      /* RT2, e.g. near cell stack    */
    uint8_t  sys_stat;                     /* raw SYS_STAT register        */
    bool     fault_ov;
    bool     fault_uv;
    bool     fault_ocd;   /* overcurrent in discharge */
    bool     fault_scd;   /* short circuit in discharge */
    bool     fault_device_xready;
} bq_pack_data_t;

/* Bring-up: turns FET controls off, enables ADC/thermistors, loads the
 * datasheet CC_CFG and discharge protection reset settings, reads factory
 * ADC trim, programs OV/UV trip registers from bms_config.h, and clears
 * stale CC_READY. Call once after HAL/I2C initialization. */
bq_status_t bq76940_init(void);

/* Pulls SYS_STAT + all wired cell voltages + pack V/I/temp in one pass. */
bq_status_t bq76940_read_all(bq_pack_data_t *out);

/* Fault handling */
bq_status_t bq76940_clear_faults(void);
bool        bq76940_has_fault(const bq_pack_data_t *data);

/* FET control - SYS_CTRL2 CHG_ON / DSG_ON bits, which gate the
 * high-side/low-side drivers feeding Q4/Q5 (charge) and Q1/Q2 (discharge). */
bq_status_t bq76940_set_charge_fet(bool enable);
bq_status_t bq76940_set_discharge_fet(bool enable);

/* Cell balancing (CELLBAL1/2/3) - pass a bitmask of cells 1..12 to bleed. */
bq_status_t bq76940_set_balance_mask(uint16_t cell_bitmask);

#endif /* BQ76940_H */
