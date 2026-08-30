# Single-sensor 3.3 V AFE — Rev A

Status: **experimental starting point for measurements**, not a frozen production design.

## Reference sensor

The original freETarget sensor schematic (`ten-point-nine/freETarget`, `KiCad/Sensors/Sensors.sch`) identifies:

- microphone: **CUI Devices CMC-2742PBJ-A**;
- original microphone load/bias resistor: 2.2 kOhm;
- original coupling capacitor: 0.1 uF;
- original amplifier: OPA1641.

CMC-2742PBJ-A manufacturer specifications include:

- electret condenser microphone;
- omnidirectional;
- nominal operating voltage: 2 V;
- maximum operating voltage: 10 V;
- nominal output impedance: 2.2 kOhm;
- sensitivity: about -42 dB re 1 V/Pa;
- nominal frequency range: 100 Hz to 20 kHz;
- current consumption up to about 0.4 mA under the specified 2 V / 2.2 kOhm load condition.

The original OPA1641 circuit is **not** copied directly into the Pico W test rig. The Pico ADC requires a 0-3.3 V-compatible waveform, so Rev A deliberately uses a low-voltage rail-to-rail amplifier and a midrail reference.

## Proposed Rev A circuit

Recommended prototype op amp: **TLV9062** (dual member of the TLV906x family).

Reasons:

- 1.8-5.5 V operation;
- rail-to-rail input and output;
- approximately 10 MHz gain-bandwidth;
- one amplifier can buffer the midrail reference while the other amplifies the microphone.

### Block diagram

```text
Pico 3V3
  |
  +-- 100R -- MIC_3V3 ---- 10uF ---- GND
  |            |          100nF ---- GND
  |            |
  |           2k2
  |            |
  |         MIC_SIGNAL
  |            |
  |      CMC-2742PBJ-A
  |            |
  |           GND
  |
  +-- 10k --+-- 10k -- GND
            |
          VREF_RAW
            |
           10uF
            |
           GND
            |
       TLV9062-A buffer
            |
           VREF = ~1.65 V

MIC_SIGNAL -- 100nF --+----> TLV9062-B (+)
                      |
                     100k
                      |
                     VREF

TLV9062-B (-) -- 2k2 -- VREF
TLV9062-B OUT -- 6k8 -- TLV9062-B (-)

TLV9062-B OUT -- 100R -- GP26 / ADC0
```

Nominal non-inverting AC gain around VREF:

```text
Av = 1 + 6.8k / 2.2k = 4.09
```

The output should therefore idle near 1.65 V and swing above/below that midpoint while remaining inside the Pico ADC range when not saturated.

## Parts for one hand-built channel

| Ref | Value / part | Purpose |
|---|---|---|
| MIC1 | CMC-2742PBJ-A | reference electret microphone |
| U1 | TLV9062 | VREF buffer + microphone amplifier |
| R1 | 100 Ohm | RC isolation for microphone supply |
| R2 | 2.2 kOhm | microphone bias/load |
| R3, R4 | 10 kOhm | 1.65 V divider |
| R5 | 100 kOhm | biases AC-coupled op-amp input to VREF |
| R6 | 2.2 kOhm | amplifier gain resistor to VREF |
| R7 | 6.8 kOhm | amplifier feedback resistor; Rev A gain ~4.09 |
| R8 | 100 Ohm | ADC input isolation |
| C1 | 10 uF | filtered microphone supply |
| C2 | 100 nF | microphone supply HF decoupling |
| C3 | 10 uF | VREF divider filtering |
| C4 | 100 nF | microphone AC coupling |
| C5 | 100 nF | op-amp supply decoupling, close to U1 |

Use ordinary 1% resistors where convenient. Capacitor dielectric is not critical for the first test except that the 100 nF supply decoupler should be a ceramic part located close to the op amp.

## Why the microphone supply is filtered

The Pico W radio creates bursty digital current. A 100 Ohm series resistor followed by 10 uF + 100 nF forms a simple local filter before the microphone bias resistor. This is intended to reduce Wi-Fi noise entering directly through the microphone supply.

If captures show strong radio-correlated interference, Rev B should consider a dedicated low-noise analog LDO or stronger supply filtering rather than trying to remove the noise in software.

## Why the signal is centered at 1.65 V

The microphone produces an AC waveform. The Pico ADC cannot digitize negative voltage. AC coupling removes the microphone's DC bias and the 100 kOhm resistor establishes a new DC operating point at the buffered midrail reference.

This gives roughly equal positive and negative headroom around 1.65 V.

## Gain is deliberately modest

The first purpose is **measurement**, not maximum detection range. An impulsive .22 LR event close to the microphone may be much larger than normal audio and can saturate an amplifier.

Rev A starts near gain 4. If real shots clip at 0 or 4095 ADC counts, reduce R7. Examples:

| R7 | Approx. gain |
|---:|---:|
| 2.2 kOhm | 2.00 |
| 4.7 kOhm | 3.14 |
| 6.8 kOhm | 4.09 |
| 10 kOhm | 5.55 |

If real signals are too small, increase R7 only after checking the raw noise floor.

## Connection to Pico W

Only three connections are required between the AFE and Pico W:

```text
AFE 3V3  -> Pico 3V3
AFE GND  -> Pico GND
AFE OUT  -> Pico GP26 / ADC0
```

For the first field test, keep the analog wiring short and keep the microphone/AFE physically away from the Pico W antenna area as far as the mechanical setup permits.

## Bench checks before connecting GP26

Use a multimeter and, preferably, an oscilloscope:

1. verify AFE supply is near 3.3 V;
2. verify buffered VREF is near 1.65 V;
3. verify amplifier output at silence is near 1.65 V;
4. verify tapping/clapping near the microphone produces a waveform around 1.65 V;
5. verify the output never exceeds approximately 0-3.3 V;
6. only then connect AFE OUT to GP26.

## First data questions

The first range session should not attempt hit-position calculation. For each real shot, record:

- whether the waveform clips;
- peak deviation from baseline;
- rise time;
- approximate useful event duration;
- amount of post-event ringing;
- idle noise level with Wi-Fi active;
- signatures from adjacent firing points;
- whether 250 kS/s adds information beyond the microphone's nominal audio bandwidth.

These measurements will determine Rev B gain/filtering and whether the final four-channel target needs a separate fast comparator path in addition to ADC waveform acquisition.
