# Architecture overview

## Controller

Initial target: STM32WB55RG on NUCLEO-WB55RG.

The application core performs acquisition, event validation, hit computation, rapid-fire state control, configuration and diagnostics. BLE is handled through the STM32WB wireless stack.

## Acquisition paths

Each microphone channel is split into two logical paths:

1. **Comparator / digital trigger path**: deterministic hardware timestamping through timer input capture.
2. **ADC path**: waveform acquisition using DMA for classification, diagnostics and neighboring-shot rejection.

The digital timestamp remains the primary timing source for TDOA. ADC data is supplementary evidence rather than a replacement for deterministic capture.

## Event pipeline

```text
armed
  -> first valid capture
  -> capture N/E/S/W timestamps
  -> collect waveform window
  -> acoustic/geometric validation
  -> reject or accept
  -> solve X,Y
  -> BLE notification
  -> log diagnostics if enabled
```

## Rapid-fire timing

BLE may command a rapid-fire sequence, but the target owns all subsequent timing locally. Red/green transitions must not depend on BLE packet latency.
