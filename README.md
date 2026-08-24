# FreeTarget 50m

Experimental electronic scoring target for ISSF 50 m rifle, derived conceptually from the freETarget acoustic target but redesigned for 50 m requirements.

## Project goals

- Detect .22 LR projectile passage/impact events on an ISSF 50 m target using four acoustic sensors.
- Measure N/E/S/W arrival times on a single hardware timebase.
- Reject or classify disturbances and shots from neighboring firing points.
- Send shot and status data directly to an Android tablet over Bluetooth Low Energy (BLE).
- Provide local red/green high-power visual signalling for rapid-fire timing.
- Keep timing-critical decisions local to the target; BLE is supervisory, not time-critical.

## Selected development platform

Initial controller: **STMicroelectronics NUCLEO-WB55RG** / STM32WB55RG.

Firmware environment:

- STM32CubeIDE
- STM32CubeMX / STM32CubeWB
- C
- STM32 HAL for general peripherals
- STM32 LL/direct register access where deterministic timing is required

## Initial architecture

```text
MIC N -- AFE --+-- comparator --> TIM2_CH1
               +-- ADC --------> DMA ----+
MIC E -- AFE --+-- comparator --> TIM2_CH2 |
               +-- ADC --------> DMA      |
MIC S -- AFE --+-- comparator --> TIM2_CH3 +--> acoustic validation --> hit solver --> BLE
               +-- ADC --------> DMA      |
MIC W -- AFE --+-- comparator --> TIM2_CH4 |
               +-- ADC --------> DMA ----+

Temperature --> I2C

STM32WB55 --> driver --> high-power GREEN signal
STM32WB55 --> driver --> high-power RED signal

STM32WB55 <---- BLE ----> Android / .NET MAUI tablet app
```

## Timing principle

The four acoustic trigger inputs are intended to share a single 32-bit TIM2 counter. The initial target timer scale is 16 MHz (62.5 ns/tick), matching the legacy freETarget timing scale while eliminating the four-independent-timer start offsets used by the ATmega2560 implementation.

## What is intentionally removed from the 50 m design

The following legacy freETarget functions are not design requirements for this project:

- witness-paper motor control
- target illumination PWM
- physical DIP switches used for simulated shots/test modes
- ESP-01 Wi-Fi / AT-command transport
- legacy display UART
- Arduino Mega-specific timer/register code

Diagnostic and calibration functions should instead be exposed through firmware diagnostics and BLE where practical.

## Repository layout

```text
firmware/             STM32WB55 firmware
hardware/             analog front-end, LED drivers, PCB and schematics
app/                  BLE changes/additions for the .NET MAUI tablet app
docs/                 architecture, protocol, calibration and test notes
test-data/acoustic/    recorded real-world acoustic datasets and metadata
```

## Design status

This repository begins at architecture stage. Pin allocation and analog front-end details remain subject to validation on the actual NUCLEO-WB55RG and target electronics before PCB freeze.

## Reference implementation

The existing freETarget implementation remains a functional and algorithmic reference. This repository is not intended to preserve AVR source compatibility; timing, acquisition, communications and diagnostics may be redesigned when that improves the 50 m system.
