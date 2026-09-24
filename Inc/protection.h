/**
 * @file    protection.h
 * @brief   Software-level protection policy layered on top of BQ76940's
 *          own hardware protections (which trip PROTECT1/2/3 thresholds
 *          autonomously even if the MCU is asleep/crashed). This layer
 *          adds cell-imbalance and temperature-window checks that the
 *          AFE hardware doesn't do on its own, and decides FET state.
 */
#ifndef PROTECTION_H
#define PROTECTION_H

#include "bq76940.h"

typedef enum {
    PACK_STATE_NORMAL = 0,
    PACK_STATE_FAULT_LATCHED
} pack_state_t;

/* Evaluate one fresh reading; updates FET outputs and returns the
 * resulting state (NORMAL or FAULT_LATCHED). Call this every time
 * bq76940_read_all() succeeds. */
pack_state_t protection_evaluate(const bq_pack_data_t *data);

/* Operator/host requests a fault reset (e.g. after removing short,
 * cooling down). Clears BQ76940 latch and re-enables FETs if healthy. */
bq_status_t protection_attempt_reset(void);

#endif /* PROTECTION_H */
