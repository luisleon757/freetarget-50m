# Pico W single-sensor field prototype

Purpose: capture real acoustic waveforms at the 50 m target using one sensor channel and send triggered captures wirelessly to the firing line.

This is an **experimental measurement rig**, not the final FreeTarget controller architecture.

## Why this prototype exists

Before designing the four-channel analog front end and final STM32WB55 firmware, we need real measurements from the 50 m installation:

- useful signal amplitude;
- event duration and ringing;
- background noise;
- neighboring-shot signatures;
- required analog gain and bandwidth;
- practical trigger threshold;
- whether the ADC waveform adds useful classification information.

One sensor cannot calculate X/Y. It can only characterize the acoustic event and communications link.

## Field topology

```text
sensor -> AFE -> GP26 / ADC0 -> Pico W
                               |
                               +-- 2.4 GHz Wi-Fi
                                      |
                         laptop/tablet hotspot
                                      |
                             UDP receiver script
```

There is no 50 m USB connection. USB may still be used locally only to power the Pico W from a power bank or to flash firmware before deployment.

## Important electrical rule

**Do not connect an unknown sensor directly to GP26.**

The RP2040 ADC input must remain inside its allowed analog input range. The eventual AFE must therefore limit/protect the signal and, for a bipolar acoustic waveform, bias it to a suitable DC midpoint (typically around half the ADC supply range).

The exact sensor/AFE is intentionally not frozen in this prototype yet.

## Initial acquisition settings

- ADC input: GPIO26 / ADC0
- sample rate: 250 kS/s
- ADC resolution: 12 bit, stored as 16-bit words
- capture block: 4096 samples
- block duration: approximately 16.384 ms
- trigger: maximum absolute deviation from the measured block mean
- default threshold: 400 ADC counts
- event holdoff: 250 ms

The RP2040 ADC supports up to 500 kS/s and DMA. The first test deliberately starts at 250 kS/s to provide a simple, conservative acquisition point.

### Limitation of v0

The ADC is captured block-by-block. A block is transmitted only after the block completes and a threshold crossing is found. Therefore:

- there is no guaranteed fixed pre-trigger interval yet;
- acquisition pauses while an event block is transmitted;
- a second event during transmission/holdoff can be missed.

That is acceptable for the first measurement campaign. A ping-pong/circular DMA design will be added only if the real data demonstrates the need.

## Wi-Fi plan

The laptop or tablet at the firing line creates a **2.4 GHz hotspot**. The Pico W joins that hotspot in station mode and sends UDP packets to port 5005.

Default destination is `255.255.255.255` broadcast so the receiver IP does not have to be hard-coded. Some hotspot/firewall configurations block broadcast; in that case set `UDP_DEST_IP` in `include/config.h` to the receiver's IPv4 address.

The 50 m link must be field-tested. Keep the Pico W antenna area clear of metal, cabling and the analog board as much as practical.

## Configure

Edit:

```text
include/config.h
```

At minimum replace:

```c
#define WIFI_SSID       "CHANGE_ME"
#define WIFI_PASSWORD   "CHANGE_ME"
```

Do not commit real private Wi-Fi credentials to the repository.

## Build

Requirements:

- Raspberry Pi Pico SDK
- CMake
- ARM GCC toolchain supported by the Pico SDK
- `PICO_SDK_PATH` environment variable pointing to the SDK

From this directory:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

The target board is fixed to `pico_w` in `CMakeLists.txt`.

The resulting UF2 can be copied to the Pico W while it is in BOOTSEL mode.

## Receiver

The receiver uses only the Python standard library.

Run at the firing line:

```bash
python tools/receive_capture.py
```

It listens on UDP port 5005 and writes each complete event to:

```text
captures/
  ft50_capture_XXXXXX_<timestamp>.csv
  ft50_capture_XXXXXX_<timestamp>.json
```

The CSV contains:

- sample index;
- time in microseconds;
- raw ADC counts;
- approximate voltage using 3.3 V as the conversion reference.

The JSON contains acquisition metadata and basic peak statistics.

If Windows Firewall asks for permission, allow the Python receiver on the hotspot/private network. If no packets arrive, test with a fixed receiver IP before changing the firmware architecture.

## UDP packet format v1

Each packet contains a 28-byte little-endian header followed by unsigned 16-bit ADC samples.

```text
magic[4]          "FT50"
version           1
flags             0
header_bytes      28
capture_id        uint32
sample_rate_hz    uint32
total_samples     uint32
sample_offset     uint32
sample_count      uint16
trigger_index     uint16
samples[]         uint16 little-endian
```

Captures are split into 512-sample packets by default. The receiver reconstructs them by `capture_id` and `sample_offset`.

UDP is intentional for this first diagnostic rig: simple, low-overhead and easy to inspect. Missing packets are detectable because a capture will remain incomplete. If field tests show meaningful packet loss, the next revision can add acknowledgements or move capture transfer to TCP while keeping the sampling side unchanged.

## First bench test before the range

1. Flash the Pico W with hotspot credentials configured.
2. Start the hotspot and the Python receiver.
3. Power the Pico W from a power bank.
4. Verify that the Pico W joins the hotspot (on-board LED stays on).
5. Feed GP26 only from a known safe 0-3.3 V test source biased around midscale.
6. Produce a transient large enough to cross the configured threshold.
7. Confirm that a CSV/JSON capture appears at the receiver.
8. Plot the CSV and verify sample timing and amplitude.

Only after this digital path works should the real sensor/AFE be connected.

## Next decisions

The next hardware task is to choose or reproduce the actual acoustic sensor and one-channel analog front end. Real captures from that setup will then drive gain, filtering, comparator and four-channel design decisions.
