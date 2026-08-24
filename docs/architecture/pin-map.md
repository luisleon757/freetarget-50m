# Preliminary pin map

This is a **working allocation**, not yet a PCB commitment. It must be checked against the exact NUCLEO-WB55RG board revision, solder bridges, debug/VCP routing and STM32 alternate-function table before hardware manufacture.

## Required resources

- 4 hardware input-capture channels on one common timer, preferably TIM2 CH1..CH4
- 4 ADC channels for microphone waveform capture
- I2C for temperature sensor
- 2 GPIO outputs for red/green high-power LED drivers
- SWD/ST-LINK debugging
- BLE radio via STM32WB stack

## Design rule

Do not sacrifice SWD debugging for production I/O. VCP UART is desirable during development but secondary to correct timer/ADC routing.
