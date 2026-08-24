# Calibration

Calibration strategy will reuse useful concepts from freETarget but is not constrained by the AVR implementation.

Expected calibration domains:

- sensor positions / target geometry
- fixed per-channel timing offsets
- temperature / speed-of-sound compensation
- comparator thresholds
- analog gain and offset
- classification/rejection thresholds

Calibration data should be stored in STM32 nonvolatile memory with a versioned schema and integrity check.
