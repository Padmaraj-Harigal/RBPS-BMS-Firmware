# EMNODE / RBPS BMS Firmware

Firmware for the battery management system board defined by
`RBPS.kicad_sch` / `RBPS.kicad_pcb`. This is a **12S-wired Li-ion BMS**
built around a TI BQ76940 analog front-end supervised by an STM32C011
host MCU.

## Hardware -> firmware map

| Schematic ref | Part | Firmware module |
|---|---|---|
| U1 | BQ76940DBT (AFE: cell voltages, current, temp, FET drive) | `bq76940.c/.h`, `bq76940_regs.h` |
| U2 | STM32C011D6Yx (host MCU) | `main.c` (runs everything below) |
| RT1, RT2 | NCP18XH103F03RB NTC (via U1 TS1/TS2) | `ntc_thermistor.c/.h` |
| R16 | 1 mOhm shunt (via U1 SRP/SRN, coulomb counter) | handled inside `bq76940.c` (`read_pack_current`) |
| Q1/Q2 (discharge), Q4/Q5 (charge) | BSC010N04LS FETs, gate-driven by U1 | `bq76940_set_charge_fet()` / `bq76940_set_discharge_fet()` |
| D1 (PWR), D2 (FAULT) | Status LEDs; PCB nets currently do not reach U2 GPIO | `led_status.c/.h` placeholders only |
| J1 (14-pin) | Cell taps VC0..VC12 into U1 | wiring only, nothing to code |
| J2 (XT90PW) | Pack power out | passive |
| J3 (4-pin) | SWD programming header (SWDIO/SWCLK/MCPWR/GND) - **no UART on this board** | n/a |
| F2, D3-D7 | Fuse, Zeners, TVS | passive protection, nothing to code |

## File layout

```
EMNODE_BMS_Firmware/
├── Inc/
│   ├── bms_config.h      Board constants: cell count, shunt value, NTC
│   │                     constants, I2C address, protection thresholds
│   ├── bq76940_regs.h    BQ76940 register map + bit definitions
│   ├── bq76940.h         BQ76940 driver public API
│   ├── i2c_bus.h         Generic I2C read/write wrapper
│   ├── ntc_thermistor.h  ADC-code -> Celsius conversion
│   ├── protection.h      Software protection policy / FET decision logic
│   └── led_status.h      D1/D2 LED control
└── Src/
    ├── bq76940.c
    ├── i2c_bus.c         STM32 HAL I2C1 implementation
    ├── ntc_thermistor.c
    ├── protection.c
    ├── led_status.c      STM32 HAL GPIO implementation
    └── main.c            Application entry point / poll loop
```

## Why it's split this way

- **`i2c_bus`** is the only file that touches `HAL_I2C_*` directly, so if
  you ever move to LL drivers, a different MCU, or a mock for unit tests,
  only this one file changes.
- **`bq76940`** knows the register map and unit conversions but nothing
  about STM32 HAL or GPIO - it only calls `i2c_bus_*`.
- **`ntc_thermistor`** is pure math (no I2C, no HAL) so it's trivially
  unit-testable on a PC.
- **`protection`** contains the only policy decisions (thresholds,
  when to open the FETs) - the part you'll tune most often - kept apart
  from the low-level driver so changing a threshold never risks breaking
  the I2C plumbing.
- **`led_status`** is the only file touching `HAL_GPIO_*`.
- **`main.c`** just wires the above together in a simple poll loop.

## Protection and board values currently in firmware

| Setting | Current firmware value | Implementation / note |
|---|---:|---|
| Cell overvoltage | 4,200 mV per cell | Software check and BQ76940 `OV_TRIP`; uses measured ADC gain/offset and rounds down to the nearest supported trip code |
| Cell undervoltage | 2,800 mV per cell | Software check and BQ76940 `UV_TRIP`; uses measured ADC gain/offset and rounds up to the nearest supported trip code |
| Charge overtemperature | 45 C | Software check only |
| Discharge overtemperature | 60 C | Software check only |
| Charge undertemperature | 0 C | Software check only; no discharge low-temperature limit is configured |
| Wired cells / AFE maximum | 12 / 15 | Firmware reads VC1 through VC12 |
| Current shunt / current scale | 1 mOhm / 8.44 mA per CC count | Schematic value for R16; verify assembled part and tolerance |
| Discharge overcurrent (OCD) | 8 A nominal, 8 ms | BQ76940 reset code 0, RSNS=0; 1 mOhm assumed |
| Discharge short circuit (SCD) | 22 A nominal, 70 us | BQ76940 reset code 0, RSNS=0; 1 mOhm assumed |
| OV / UV hardware delay | 1 s / 1 s | BQ76940 reset PROTECT3 delay codes |
| Current sample interval | 250 ms | BQ76940 coulomb-counter interval; driver clears latched CC_READY after reading |
| Thermistors | 10 kOhm at 25 C, Beta 3380 K | RT1/RT2; BQ76940 bias is nominally 10 kOhm from 3.3 V |
| FET auto-enable | Disabled | `BMS_ALLOW_FET_ENABLE` defaults to 0 pending board and pack validation |

The voltage and temperature values above are firmware settings, not limits
specified by the BQ76940. Confirm them against the exact cell datasheet and
battery design. No under-current protection is configured. The 8 A OCD
and 22 A SCD figures are the AFE reset threshold codes converted using the
assumed 1 mOhm shunt; they may be too low for the intended load. Select
current thresholds using the pack's allowed current, shunt tolerance, and
FET design before use. The driver programs OV_TRIP and UV_TRIP at startup
using the BQ76940's factory ADC gain and offset calibration. OV is rounded
down and UV up to a representable ADC code. The thermistor values match
Murata's NCP18XH103F03RB specification.

## Before this builds

1. **Fix the PCB signal routing before expecting this firmware to run.**
   In the shared `RBPS.kicad_pcb`, BQ76940 `/SDA` and `/SCL` are distinct
   nets from MCU `PB7` and `PB6`; `/ALERT` also does not reach an MCU pad.
   D1 and D2 LED nets do not reach MCU GPIO pads either. The source pin
   assumptions describe intended wiring, not the current routed board.
2. Generate the real STM32CubeMX clock, GPIO, and I2C1 initialization
   for STM32C011D6Yx after the board nets/pins are corrected. The current
   `SystemClock_Config`, `MX_GPIO_Init`, and `MX_I2C1_Init` are empty
   placeholders, so this folder is not a flashable firmware project yet.
3. Confirm the external NTC bias network and REGOUT voltage against the
   schematic and assembled board; the current conversion assumes a 10k
   bias resistor and 3.3 V bias.
4. Set cell voltage and temperature limits for the actual chemistry and
   verify the firmware-programmed OV/UV registers on the assembled device.
5. Select OCD/SCD settings for the pack's allowed current, shunt tolerance,
   and FET design. The code explicitly programs the BQ76940 reset settings
   (nominally 8 A OCD / 22 A SCD for 1 mOhm). FET auto-enable remains off
   (`BMS_ALLOW_FET_ENABLE` is 0) until routing and protection are verified.
6. This board has no UART - if you want live telemetry, either add a
   UART/CAN transceiver, or read data out over SWD (e.g. via RTT/ITM).

## Firmware verification completed

All firmware C sources pass a host syntax compile with GCC using
`-std=c99 -Wall -Wextra -Werror` and a temporary STM32 HAL stub. A
temporary mock BQ76940 run passed checks for initialization, calibrated
OV/UV register values, cell/current/NTC conversion, CC_READY clearing, and
the invalid-thermistor fail-safe path. These are host checks only. Target
linking, CubeMX/HAL integration, electrical bring-up, and pack safety have
not been verified; the PCB signal routing issue above prevents a meaningful
hardware test.

## Not covered (hardware only, no code needed)

Fuse (F2), Zener diodes (D3-D6), TVS diode (D7), test points (TP1/3/4/5),
XT90 connector (J2) - all passive protection/interconnect with nothing
for firmware to drive or read.
