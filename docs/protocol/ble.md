# BLE protocol draft

A custom GATT service is planned.

## Characteristics

### SHOT
- Direction: target -> tablet
- Property: Notify
- Suggested payload: shot id, X, Y, N/E/S/W timestamps, status flags, optional quality/confidence value

### STATUS
- Direction: target -> tablet
- Property: Notify / Read
- Suggested payload: armed state, BLE state, temperature, diagnostics, rapid-fire state

### CONTROL
- Direction: tablet -> target
- Property: Write
- Commands: arm, disarm, clear, diagnostic capture, calibration actions

### CONFIG
- Direction: bidirectional
- Property: Read / Write
- Parameters: geometry, acoustic thresholds, calibration values, capture-window settings

### RAPID_FIRE
- Direction: tablet -> target
- Property: Write
- The tablet supplies a sequence/profile; the STM32 executes timing locally.

## Principle

BLE transport must not be part of the time-critical shot timestamp path or red/green transition timing.
