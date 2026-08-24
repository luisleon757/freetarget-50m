# Architecture overview

## Scope

FreeTarget 50 m is a purpose-built electronic scoring target for ISSF 50 m use. It takes the existing freETarget implementation as a functional and algorithmic reference, but it does not preserve Arduino Mega hardware assumptions or AVR source compatibility.

The initial development platform is the **STMicroelectronics NUCLEO-WB55RG**, using the **STM32WB55RG**. This choice combines a capable Cortex-M4 application core with an integrated BLE radio subsystem and avoids adding an external Wi-Fi/BLE modem to the first prototype.

## System responsibilities

### Target electronics

The target is responsible for all functions that require deterministic or autonomous behaviour:

- timestamp the four acoustic trigger channels on one hardware timebase;
- acquire supporting microphone waveform data;
- validate and classify acoustic events;
- calculate shot coordinates;
- apply calibration and geometry corrections;
- execute rapid-fire red/green timing locally;
- retain configuration needed to operate after reset;
- expose diagnostics and status to the tablet;
- communicate shot results and commands over BLE.

### Tablet application

The tablet is responsible for supervisory and user-interface functions:

- connect and reconnect over BLE;
- configure the target;
- display shot position, score and status;
- select and start supported shooting sequences;
- request diagnostics and calibration operations;
- store or export session-level data as required.

The tablet must **not** be in the timing-critical path for acoustic timestamping or rapid-fire signal transitions.

## Hardware data paths

Each microphone channel is split into two logical paths:

1. **Comparator / digital trigger path** — deterministic edge timestamping through timer input capture.
2. **ADC path** — waveform acquisition using DMA for event validation, diagnostics and neighboring-shot rejection.

The four digital trigger inputs should use four channels of one common 32-bit timer, preferably TIM2 CH1..CH4 if the final pin map permits it. This removes the relative start-offset problem inherent in using multiple independently started timers.

The initial timing target is a 16 MHz timer counter, giving 62.5 ns/tick and preserving the useful resolution of the legacy implementation while providing a single shared timebase.

```text
MIC N -- AFE --+-- comparator --> timer capture CH1 --+
               +-- ADC ------------------------------+|
MIC E -- AFE --+-- comparator --> timer capture CH2 -+|
               +-- ADC ------------------------------+|
MIC S -- AFE --+-- comparator --> timer capture CH3 -++--> event validation
               +-- ADC ------------------------------+|        |
MIC W -- AFE --+-- comparator --> timer capture CH4 -+|        v
               +-- ADC -------------------------------+    hit solver
                                                               |
Temperature --> I2C -------------------------------------------+--> compensation
                                                               |
                                                               +--> BLE shot/status

STM32WB55 --> protected driver --> GREEN high-power signal
STM32WB55 --> protected driver --> RED high-power signal
STM32WB55 <-------------- BLE --------------> Android tablet
```

The analog front end, comparator thresholds, ADC sampling scheme and exact pins remain prototype-level decisions until they are measured on real 50 m acoustic data and checked against the NUCLEO-WB55RG alternate-function map.

## Shot-event pipeline

The firmware should treat shot processing as a bounded state machine rather than a collection of blocking functions.

```text
DISARMED
   |
   v
ARMED
   |
   +-- first plausible capture --> ACQUIRING
                                  |
                                  +-- collect N/E/S/W timestamps
                                  +-- retain ADC waveform window
                                  |
                                  v
                              VALIDATING
                               /      \
                         reject       accept
                           |             |
                           v             v
                         ARMED        SOLVING
                                         |
                                         v
                                     REPORTING
                                         |
                                         v
                                       ARMED
```

A malformed or incomplete event must time out cleanly and return the target to a known state. Diagnostic capture may preserve rejected-event data, but diagnostics must not permanently stall the acquisition path.

## Real-time boundary

The following functions are time-critical and should be implemented using hardware peripherals, interrupts, DMA and short deterministic code paths:

- timer input capture;
- acquisition-window control;
- ADC/DMA capture coordination;
- rapid-fire signal transitions;
- event timeout handling.

The following functions are not allowed to delay those paths:

- BLE notifications;
- BLE command parsing;
- logging;
- formatted diagnostic output;
- flash writes;
- tablet reconnection handling.

STM32 HAL is appropriate for general initialization and non-critical peripherals. LL or direct register access may be used when measurement shows that HAL overhead or abstraction prevents deterministic behaviour.

## Acoustic validation strategy

The first prototype should keep the **hardware timestamps as the primary TDOA measurement**. ADC waveforms are supplementary evidence and can be used for:

- rejecting electrical noise;
- distinguishing a projectile event from unrelated mechanical impulses;
- checking whether all four channels contain a compatible event;
- detecting clipping or marginal thresholds;
- developing classifiers for neighboring firing-point disturbances;
- offline analysis and threshold tuning.

A waveform classifier should not be placed in the mandatory scoring path until measured data shows that it improves reliability without introducing unacceptable latency or false rejection.

## Hit solver

The hit solver consumes validated N/E/S/W timestamps plus the active geometry/calibration set. Its public interface should not depend on STM32 peripheral registers. This keeps the mathematical solver testable on a PC using recorded datasets.

Recommended separation:

```text
capture hardware -> event record -> validator -> solver -> shot result
```

The `event record` should contain raw timestamps and enough metadata to replay the calculation offline. The `shot result` should contain coordinates, status flags and a quality/confidence indication rather than exposing internal peripheral state.

## Rapid-fire signalling

Red/green visual signalling is a local real-time function.

BLE may select a profile and request that a sequence start, but after acceptance the target owns the schedule. Signal changes must be driven from local timers and must continue correctly through ordinary BLE packet latency or temporary radio congestion.

The output stage must use a protected driver suitable for the selected high-power LED or lamp. The STM32 pin is a logic control signal only; the load must not be driven directly from the MCU GPIO.

A rapid-fire sequence should have explicit states such as:

```text
IDLE -> PREPARE -> RED -> GREEN -> RED -> COMPLETE
```

Exact durations and competition profiles belong in configuration/protocol documentation, not as scattered magic constants in GPIO code.

## BLE architecture

BLE is the supervisory transport. A custom GATT service is planned with characteristics for:

- shot notifications;
- target status;
- control commands;
- configuration/calibration;
- rapid-fire profile/control;
- diagnostic data where practical.

The on-air protocol should be versioned from the beginning. Binary payloads should use fixed-width integer fields and explicit units. A future protocol change must be detectable by both the target and tablet.

## Persistent configuration

Persistent data is expected to include at least:

- target geometry;
- sensor/channel calibration;
- acoustic thresholds;
- temperature-compensation parameters;
- validated acquisition-window settings;
- device/protocol configuration.

Configuration storage should include a schema/version value and integrity check so incompatible or corrupt data can fall back to known defaults.

## Diagnostics and testability

Diagnostics are a design feature, not an afterthought. At minimum the firmware should be able to expose or record:

- raw four-channel timestamps;
- event rejection reason;
- temperature used for a calculation;
- signal/threshold status;
- selected waveform snippets when diagnostic capture is enabled;
- solver result before display-side scoring transformations;
- rapid-fire state and timing faults.

The solver and event-validation logic should be written so recorded events under `test-data/acoustic/` can be replayed in host-side tests without STM32 hardware.

## Functions intentionally removed from the 50 m design

The following legacy freETarget functions are not requirements for this project:

- witness-paper motor control;
- target illumination PWM;
- physical DIP switches used for simulated shots/test modes;
- ESP-01 Wi-Fi / AT-command transport;
- legacy display UART;
- Arduino Mega-specific timer/register code.

Useful diagnostic and calibration functions formerly reached through switches or serial commands should be re-exposed through firmware diagnostics and BLE where appropriate.

## Current design decisions

Considered selected for the first prototype unless testing demonstrates a problem:

- NUCLEO-WB55RG / STM32WB55RG development platform;
- C with STM32CubeIDE/CubeMX/CubeWB;
- BLE using the STM32WB wireless stack;
- four hardware timestamp channels sharing one timer timebase;
- comparator timestamp path plus ADC/DMA waveform path;
- local rapid-fire timing;
- separate red and green high-power driver control outputs;
- architecture that permits solver/classifier host-side testing.

## Decisions still open

The following must not be treated as frozen hardware commitments yet:

- exact microphone/sensor type and mechanical mounting;
- analog front-end topology and gain;
- comparator implementation and threshold strategy;
- ADC sample rate, trigger mode, buffer length and memory budget;
- exact STM32 pin allocation;
- temperature-sensor part and placement;
- high-power signal LED/driver electrical design;
- final GATT UUIDs and binary payload layouts;
- persistent-storage implementation;
- event-classification algorithm for neighboring shots;
- final PCB form factor and power architecture.

These items should be closed by measurement, datasheet checks and prototype tests rather than by preserving legacy implementation details.
