# Hardware

Initial hardware blocks:

1. Four microphone / acoustic sensor channels.
2. Per-channel analog front-end.
3. Comparator output for deterministic timestamp capture.
4. ADC feed for waveform capture.
5. Temperature sensor interface.
6. Red and green high-power LED drivers.
7. STM32WB55 controller / development board.
8. Power regulation and protection.

## LED outputs

The STM32 GPIO must not drive the high-power LEDs directly. Use suitable MOSFET/current-driver stages and optics sized for clear visibility at 50 m.

## 3.3 V rule

All analog signals entering STM32 ADC pins must remain inside the permitted STM32WB55 analog voltage range. Legacy 5 V freETarget analog levels must not be connected directly without adaptation.
