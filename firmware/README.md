# Firmware

Target environment:

- MCU: STM32WB55RG
- Board: NUCLEO-WB55RG during development
- IDE: STM32CubeIDE
- Language: C
- Framework: STM32CubeWB HAL/LL + STM32_WPAN BLE stack

## Planned modules

```text
Core/
Target/
  acquisition.c
  acoustic_buffer.c
  event_classifier.c
  hit_solver.c
  rapid_fire.c
BLE/
  freetarget_service.c
Config/
  calibration.c
  storage.c
Drivers/
  microphones.c
  signal_leds.c
```

The firmware should be generated from a checked-in CubeMX `.ioc` once the final peripheral mapping is validated.
