# Firmware

## Target environment

- MCU: STM32WB55RG
- Board: NUCLEO-WB55RG during development
- IDE: STM32CubeIDE
- Configuration/generation: STM32CubeMX / STM32CubeWB
- Language: C
- Framework: STM32 HAL for general peripherals, LL/direct register access only where deterministic timing requires it
- Wireless: STM32_WPAN BLE stack

A checked-in CubeMX `.ioc` will become the authoritative peripheral configuration once the timer, ADC, DMA and pin mapping have been validated on the actual NUCLEO-WB55RG.

## Design rules

1. **Generated Cube code is infrastructure, not application architecture.** Target logic must not be spread through `main.c` user-code sections.
2. **Interrupts stay short.** Capture ISR/DMA callbacks move data into bounded records/buffers and schedule work; they do not run the hit solver or BLE formatting.
3. **BLE is never in a time-critical path.** Loss or delay of a radio packet cannot alter shot timestamps or rapid-fire transitions.
4. **The solver has no MCU dependency.** Geometry/calibration/coordinate code must compile for host-side tests.
5. **Rejected events are explainable.** Validation returns explicit reason/status flags rather than silently dropping data.
6. **No blocking delays in acquisition or rapid-fire code.** Hardware timers/state machines provide timing.
7. **Persistent configuration is versioned.** Firmware must detect incompatible/corrupt configuration and fall back safely.

## Proposed application modules

The final Cube-generated directory structure may differ, but application responsibilities should remain separated approximately as follows:

```text
firmware/
  Core/                         Cube-generated startup/peripheral glue

  App/
    target_app.c                top-level state and scheduling
    event_pipeline.c            acquisition -> validation -> solve -> report

  Acquisition/
    capture_timer.c             four-channel common-timebase input capture
    acoustic_adc.c              ADC/DMA waveform capture
    event_record.c              bounded raw event representation

  Acoustic/
    event_validator.c           timing/waveform plausibility checks
    acoustic_features.c         optional waveform-derived features

  Solver/
    hit_solver.c                hardware-independent X/Y solution
    geometry.c                  sensor geometry
    sound_speed.c               temperature/acoustic compensation

  RapidFire/
    rapid_fire.c                local deterministic sequence state machine

  BLE/
    freetarget_service.c        GATT service/characteristics
    ble_codec.c                 versioned binary payload encode/decode

  Config/
    calibration.c               calibration model and validation
    storage.c                   persistent schema/integrity/defaults

  Drivers/
    temperature.c               temperature sensor abstraction
    signal_leds.c               red/green power-driver logic interface

  Diagnostics/
    diagnostics.c               counters, rejection reasons, fault/status data
    capture_export.c            selected raw-event/waveform diagnostic export

  Tests/
    host/                       solver/validator tests using recorded events
```

Names may change when the Cube project is generated; the important part is preserving the boundaries.

## Core data model

Peripheral callbacks should converge on a small number of application-level structures.

Conceptually:

```text
RawEvent
  shot/event id
  N/E/S/W capture ticks
  capture-present mask
  capture timeout/status
  temperature snapshot
  optional ADC buffer reference

ValidatedEvent
  normalized/calibrated timing values
  validation flags
  quality metrics

ShotResult
  X/Y coordinates
  solver/status flags
  quality/confidence
  selected diagnostic metadata
```

Fixed-width integer types and explicit units should be used at interfaces. Floating-point may be used internally where measurement demonstrates that it is appropriate, but protocol/storage formats should not depend on compiler-specific structure layout.

## Scheduling model

A simple cooperative/event-driven main loop is preferred initially over adding an RTOS without a demonstrated need. STM32WB wireless integration may impose its own sequencer requirements; target application work should integrate with that scheduler while preserving deterministic hardware capture.

An RTOS should only be introduced if measurements show a concrete benefit that cannot be achieved cleanly with hardware peripherals, DMA, interrupts and the STM32WB task/sequencer model.

## First firmware milestones

1. Generate a minimal STM32WB55 project with SWD and BLE operational.
2. Prove four-channel timestamp capture on one common timer using synthetic edges.
3. Add ADC+DMA circular/pre-trigger or bounded capture strategy and measure memory/CPU cost.
4. Define `RawEvent` and replay it in a host-side test harness.
5. Port/adapt the legacy hit solver into the hardware-independent `Solver/` layer and build regression vectors.
6. Implement target state machine and explicit event timeouts/rejection reasons.
7. Implement BLE shot/status/control skeleton with protocol versioning.
8. Add local timer-driven red/green rapid-fire state machine.
9. Integrate real acoustic datasets and tune validation only from measurements.

## Legacy code policy

The Arduino Mega firmware is a behavioural reference, not a source tree to mechanically port. Before copying a legacy function, classify it using [`docs/architecture/legacy-migration.md`](../docs/architecture/legacy-migration.md) and identify the regression test that will prove equivalent or improved behaviour.
