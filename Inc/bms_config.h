/**
 * @file    bms_config.h
 * @brief   Board-level configuration for the EMNODE / RBPS BMS.
 *
 * Values below are taken directly from RBPS.kicad_sch / RBPS.kicad_pcb:
 *   U1  = BQ76940DBT   (Analog Front End, up to 15S)
 *   U2  = STM32C011D6Yx (host MCU)
 *   RT1/RT2 = NCP18XH103F03RB NTC (10k @ 25C, B25/50 = 3380K)
 *   R16 = 1 mOhm current shunt (SRP/SRN inputs of U1)
 *   J1  = 14-pin cell tap connector -> U1 VC0..VC12 (12S wired, board
 *         supports jumper-configurable cell count via VC5B/VC5X and
 *         VC10B/VC10X per TI app note "Configuring Alternative Cell Counts")
 *   D1  = PWR LED, D2 = FAULT LED (plain GPIO, driven by U2)
 *   J3  = 4-pin SWD header (SWDIO / SWCLK / MCPWR / GND) - no UART on this board
 *
 * PCB review finding (RBPS.kicad_pcb): /SDA and /SCL currently do not
 * connect to U2 PB7/PB6, /ALERT does not connect to an MCU input, and the
 * D1/D2 LED nets do not connect to MCU GPIO. Firmware cannot use these
 * signals until the PCB/schematic is corrected and rerouted.
 */
#ifndef BMS_CONFIG_H
#define BMS_CONFIG_H

#include <stdint.h>

/* ---------------- Pack / cell topology ---------------- */
#define BMS_NUM_CELLS_WIRED     12      /* VC0..VC12 taps populated on J1 */
#define BMS_NUM_CELLS_MAX       15      /* BQ76940 hardware maximum        */

/* ---------------- Current shunt ------------------------ */
#define BMS_SHUNT_RESISTANCE_MOHM   1   /* R16 = 1 mOhm */

/* ---------------- NTC thermistor (RT1/RT2) -------------- */
#define NTC_R25_OHMS            10000.0f   /* NCP18XH103F03RB: 10k @ 25C */
#define NTC_BETA_25_50          3380.0f    /* B25/50 constant, datasheet */
#define NTC_T25_KELVIN          298.15f

/* ---------------- I2C bus (U1 <-> U2) ------------------- */
#define BQ76940_I2C_7BIT_ADDR   0x08U   /* fixed device address (SMBus-style) */

/* Do not assign firmware GPIOs based on net names until the PCB routes
 * those nets to MCU pads. See the PCB review finding above. */

/* ---------------- Protection thresholds (tune per cell chemistry) */
#define BMS_CELL_OV_MILLIVOLT      4200   /* cell overvoltage trip     */
#define BMS_CELL_UV_MILLIVOLT      2800   /* cell undervoltage trip    */
#define BMS_OTP_CHARGE_C           45     /* over-temp, charging       */
#define BMS_OTP_DISCHARGE_C        60     /* over-temp, discharging    */
#define BMS_UTP_CHARGE_C           0      /* under-temp, charging      */

/* Keep FET turn-on disabled until the board wiring and AFE protection
 * thresholds are verified on the actual pack. Set to 1 only after review. */
#define BMS_ALLOW_FET_ENABLE        0

#endif /* BMS_CONFIG_H */
