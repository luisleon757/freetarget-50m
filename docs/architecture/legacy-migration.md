# Legacy firmware migration matrix

This document classifies the responsibilities of the Arduino Mega / legacy freETarget firmware for the 50 m redesign.

It is intentionally a **functional migration matrix**, not yet a source-file-by-source-file audit. The source-level audit should be added once the exact legacy firmware revision used as reference is pinned in this repository.

## Classification

- **KEEP** — preserve the concept/algorithm with minimal semantic change.
- **ADAPT** — preserve the function but redesign its interfaces or implementation for STM32WB55/50 m.
- **REWRITE** — preserve the requirement, but the old implementation is tied closely enough to AVR/legacy transport that a new implementation is preferred.
- **REMOVE** — no requirement in the 50 m target.

## Matrix

| Legacy responsibility | Decision | 50 m implementation direction |
| --- | --- | --- |
| Four-sensor shot timing concept | KEEP | Retain TDOA principle and four directional sensor channels. |
| Raw N/E/S/W timing data | KEEP | Preserve as part of a replayable event record and diagnostics. |
| Hit-coordinate mathematics | ADAPT | Extract into hardware-independent C module; verify against legacy test vectors and new 50 m data. |
| Acoustic velocity / temperature correction | ADAPT | Retain concept; use explicit units, calibrated temperature input and testable functions. |
| Sensor geometry / offsets | ADAPT | Move to versioned configuration rather than scattered constants. |
| Shot validity checks | ADAPT | Preserve useful geometric/timing checks; add ADC-supported acoustic validation where measured data justifies it. |
| Diagnostic/test shot facilities | ADAPT | Expose through BLE and diagnostic modes instead of physical simulation switches. |
| Timer capture implementation | REWRITE | Replace AVR timer/register logic with one shared STM32 32-bit timer and four hardware input-capture channels. |
| Timer start/synchronization workaround | REMOVE | A single common timer timebase makes independent timer-start offset handling unnecessary. |
| Microphone waveform acquisition | REWRITE | Implement ADC+DMA capture window as a separate evidence/diagnostic path. |
| Main blocking acquisition loop | REWRITE | Use bounded event/state-machine design with interrupts/DMA and explicit timeouts. |
| Communications command parser | REWRITE | Define a versioned BLE GATT contract; do not carry forward ESP AT-command framing. |
| ESP-01 Wi-Fi transport | REMOVE | BLE is provided by STM32WB55 wireless subsystem. |
| Legacy display UART | REMOVE | Tablet communicates directly over BLE. A development UART may remain for debugging only. |
| EEPROM-style configuration handling | REWRITE | Use versioned persistent configuration with integrity checking and safe defaults. |
| Calibration commands | ADAPT | Keep useful calibration operations, but expose them through structured BLE commands/diagnostics. |
| Rapid-fire signalling logic | REWRITE | Implement an explicit local timer-driven state machine for red/green high-power outputs. |
| Witness-paper motor control | REMOVE | Not required for this 50 m design. |
| Target illumination PWM | REMOVE | Not required for this 50 m design. |
| Physical DIP/test switches | REMOVE | Replace useful functions with software configuration and diagnostics. |
| Arduino Mega pin abstractions | REMOVE | New pin map is derived from STM32 alternate functions and board constraints. |
| Arduino/AVR-specific macros and register code | REMOVE | STM32Cube HAL/LL/direct register code only where appropriate. |
| Power-on self-test / fault reporting | ADAPT | Preserve useful checks and make failures visible through status/diagnostics. |

## Migration rule

Do not port a legacy function merely because it exists. For each source module or function, answer in order:

1. What physical or user requirement does this code satisfy?
2. Is that requirement still present at 50 m?
3. Does the old algorithm contain experimentally useful knowledge?
4. Is the implementation coupled to AVR, Arduino, ESP-01, paper transport, illumination or simulation switches?
5. Can the result be unit-tested away from target hardware?

If the requirement is absent, remove the code. If the requirement remains but the implementation is strongly coupled to the old hardware, rewrite it around a small testable interface.

## Interfaces to preserve conceptually

The following boundaries should be made explicit even if the legacy firmware did not separate them cleanly:

```text
hardware capture
    -> raw event
    -> event validator
    -> calibrated timing event
    -> hit solver
    -> shot result
    -> BLE/reporting
```

Rapid-fire control is deliberately separate:

```text
BLE/profile request -> rapid-fire state machine -> local timer -> red/green outputs
```

## Source-level audit TODO

When the reference legacy firmware revision is pinned, extend this file with a second table containing:

- legacy source file/function;
- present responsibility;
- KEEP / ADAPT / REWRITE / REMOVE;
- replacement STM32 module;
- required regression test;
- notes about legacy constants or calibration behaviour that must not be lost.

No legacy code should be copied into the new firmware before that audit identifies why it is still required.
