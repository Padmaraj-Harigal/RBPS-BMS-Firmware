/**
 * @file    bq76940_regs.h
 * @brief   Register map for the TI BQ76940 Analog Front End (U1).
 *          Register addresses/bit layout per BQ76920/30/40 datasheet
 *          (TI SLUSBK2). BQ76940 = 15-cell variant, VC1..VC15 populated,
 *          this board wires VC0..VC12 (12S).
 */
#ifndef BQ76940_REGS_H
#define BQ76940_REGS_H

/* ---- Status / control ---- */
#define BQ_REG_SYS_STAT      0x00U
#define BQ_REG_CELLBAL1      0x01U
#define BQ_REG_CELLBAL2      0x02U
#define BQ_REG_CELLBAL3      0x03U
#define BQ_REG_SYS_CTRL1     0x04U
#define BQ_REG_SYS_CTRL2     0x05U
#define BQ_REG_PROTECT1      0x06U
#define BQ_REG_PROTECT2      0x07U
#define BQ_REG_PROTECT3      0x08U
#define BQ_REG_OV_TRIP       0x09U
#define BQ_REG_UV_TRIP       0x0AU
#define BQ_REG_CC_CFG        0x0BU

/* TI requires CC_CFG = 0x19 for optimal coulomb-counter operation. */
#define BQ_CC_CFG_VALUE      0x19U

/* ---- Cell voltage registers: VC1_HI @0x0C ... VC15_LO @0x29 ---- */
#define BQ_REG_VC1_HI        0x0CU
#define BQ_CELL_REG_STRIDE   2U      /* 2 bytes per cell (HI/LO)      */

/* ---- Pack / thermistor / coulomb counter ---- */
#define BQ_REG_BAT_HI        0x2AU
#define BQ_REG_BAT_LO        0x2BU
#define BQ_REG_TS1_HI        0x2CU
#define BQ_REG_TS1_LO        0x2DU
#define BQ_REG_TS2_HI        0x2EU
#define BQ_REG_TS2_LO        0x2FU
#define BQ_REG_TS3_HI        0x30U
#define BQ_REG_TS3_LO        0x31U
#define BQ_REG_CC_HI         0x32U
#define BQ_REG_CC_LO         0x33U

/* ---- ADC calibration (factory-trimmed, read at boot) ---- */
#define BQ_REG_ADCGAIN1      0x50U
#define BQ_REG_ADCOFFSET     0x51U
#define BQ_REG_ADCGAIN2      0x59U

/* ---- SYS_STAT bits ---- */
#define BQ_SYS_STAT_CC_READY      (1U << 7)
#define BQ_SYS_STAT_DEVICE_XREADY (1U << 5)
#define BQ_SYS_STAT_OVRD_ALERT    (1U << 4)
#define BQ_SYS_STAT_UV            (1U << 3)
#define BQ_SYS_STAT_OV            (1U << 2)
#define BQ_SYS_STAT_SCD           (1U << 1)
#define BQ_SYS_STAT_OCD           (1U << 0)
#define BQ_SYS_STAT_FAULT_MASK    (BQ_SYS_STAT_UV | BQ_SYS_STAT_OV | \
                                    BQ_SYS_STAT_SCD | BQ_SYS_STAT_OCD | \
                                    BQ_SYS_STAT_DEVICE_XREADY)

/* ---- SYS_CTRL1 bits ---- */
#define BQ_SYS_CTRL1_ADC_EN       (1U << 4)
#define BQ_SYS_CTRL1_TEMP_SEL     (1U << 3)  /* 1 = use external TS thermistors */

/* ---- SYS_CTRL2 bits ---- */
#define BQ_SYS_CTRL2_DSG_ON       (1U << 1)
#define BQ_SYS_CTRL2_CHG_ON       (1U << 0)
#define BQ_SYS_CTRL2_CC_EN        (1U << 6)

/* Datasheet reset configuration: RSNS=0, SCD threshold code 0 / 70 us,
 * OCD threshold code 0 / 8 ms, and OV/UV delays of 1 s. With a 1 mOhm
 * shunt, the current thresholds are nominally 22 A SCD and 8 A OCD. */
#define BQ_PROTECT1_DEFAULT       0x00U
#define BQ_PROTECT2_DEFAULT       0x00U
#define BQ_PROTECT3_DEFAULT       0x00U

#endif /* BQ76940_REGS_H */
