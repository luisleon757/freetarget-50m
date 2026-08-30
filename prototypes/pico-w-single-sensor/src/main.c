#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/opt.h"

#include "config.h"

#define FT50_MAGIC_0 'F'
#define FT50_MAGIC_1 'T'
#define FT50_MAGIC_2 '5'
#define FT50_MAGIC_3 '0'
#define FT50_PROTOCOL_VERSION 1u
#define NO_TRIGGER_INDEX 0xFFFFu

_Static_assert(SAMPLE_RATE_HZ > 0u && SAMPLE_RATE_HZ <= 500000u,
               "SAMPLE_RATE_HZ must be between 1 and 500000");
_Static_assert(CAPTURE_SAMPLES > 0u && CAPTURE_SAMPLES <= 65535u,
               "CAPTURE_SAMPLES must fit trigger_index field");
_Static_assert(UDP_SAMPLES_PER_PACKET > 0u && UDP_SAMPLES_PER_PACKET <= 700u,
               "Keep UDP payload below typical Ethernet MTU");

typedef struct __attribute__((packed)) {
    uint8_t magic[4];
    uint8_t version;
    uint8_t flags;
    uint16_t header_bytes;
    uint32_t capture_id;
    uint32_t sample_rate_hz;
    uint32_t total_samples;
    uint32_t sample_offset;
    uint16_t sample_count;
    uint16_t trigger_index;
} ft50_udp_header_t;

static uint16_t capture_buffer[CAPTURE_SAMPLES];
static uint8_t tx_buffer[sizeof(ft50_udp_header_t) + UDP_SAMPLES_PER_PACKET * sizeof(uint16_t)];
static uint dma_chan;
static struct udp_pcb *udp;
static ip_addr_t udp_destination;
static uint32_t capture_id = 0;

static void set_led(bool on) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on ? 1 : 0);
}

static bool wifi_connect(void) {
    if (cyw43_arch_init()) {
        return false;
    }

    cyw43_arch_enable_sta_mode();

    while (cyw43_arch_wifi_connect_timeout_ms(
               WIFI_SSID,
               WIFI_PASSWORD,
               CYW43_AUTH_WPA2_AES_PSK,
               15000) != 0) {
        set_led(true);
        sleep_ms(150);
        set_led(false);
        sleep_ms(850);
    }

    set_led(true);
    return true;
}

static bool udp_init(void) {
    if (!ipaddr_aton(UDP_DEST_IP, &udp_destination)) {
        return false;
    }

    cyw43_arch_lwip_begin();
    udp = udp_new();
    if (udp != NULL) {
        ip_set_option(udp, SOF_BROADCAST);
    }
    cyw43_arch_lwip_end();

    return udp != NULL;
}

static void adc_dma_init(void) {
    adc_init();
    adc_gpio_init(ADC_GPIO);
    adc_select_input(ADC_INPUT);

    adc_fifo_setup(
        true,   // Enable FIFO.
        true,   // Enable DMA requests.
        1,      // DREQ when at least one sample is present.
        false,  // Do not include ERR bit.
        false   // Keep full 12-bit samples in 16-bit words.
    );

    // ADC clock is 48 MHz. Sample period is (1 + clkdiv) ADC clock cycles,
    // clamped to at least 96 cycles by the RP2040 ADC hardware.
    const float clkdiv = (48000000.0f / (float)SAMPLE_RATE_HZ) - 1.0f;
    adc_set_clkdiv(clkdiv);

    dma_chan = dma_claim_unused_channel(true);
}

static void acquire_block(void) {
    adc_run(false);
    adc_fifo_drain();

    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(
        dma_chan,
        &cfg,
        capture_buffer,
        &adc_hw->fifo,
        CAPTURE_SAMPLES,
        true
    );

    adc_run(true);
    dma_channel_wait_for_finish_blocking(dma_chan);
    adc_run(false);
    adc_fifo_drain();
}

static bool find_trigger(uint16_t *trigger_index, uint16_t *peak_delta) {
    uint64_t sum = 0;
    for (uint32_t i = 0; i < CAPTURE_SAMPLES; ++i) {
        sum += capture_buffer[i] & 0x0FFFu;
    }

    const uint16_t mean = (uint16_t)(sum / CAPTURE_SAMPLES);
    uint16_t peak = 0;
    uint16_t peak_index = NO_TRIGGER_INDEX;

    for (uint32_t i = 0; i < CAPTURE_SAMPLES; ++i) {
        const uint16_t sample = capture_buffer[i] & 0x0FFFu;
        const uint16_t delta = (sample >= mean) ? (sample - mean) : (mean - sample);
        if (delta > peak) {
            peak = delta;
            peak_index = (uint16_t)i;
        }
    }

    *trigger_index = peak_index;
    *peak_delta = peak;
    return peak >= TRIGGER_DELTA_COUNTS;
}

static err_t send_packet(uint32_t id,
                         uint32_t sample_offset,
                         uint16_t sample_count,
                         uint16_t trigger_index) {
    ft50_udp_header_t header = {
        .magic = {FT50_MAGIC_0, FT50_MAGIC_1, FT50_MAGIC_2, FT50_MAGIC_3},
        .version = FT50_PROTOCOL_VERSION,
        .flags = 0,
        .header_bytes = (uint16_t)sizeof(ft50_udp_header_t),
        .capture_id = id,
        .sample_rate_hz = SAMPLE_RATE_HZ,
        .total_samples = CAPTURE_SAMPLES,
        .sample_offset = sample_offset,
        .sample_count = sample_count,
        .trigger_index = trigger_index,
    };

    const size_t sample_bytes = (size_t)sample_count * sizeof(uint16_t);
    const size_t packet_bytes = sizeof(header) + sample_bytes;

    memcpy(tx_buffer, &header, sizeof(header));
    memcpy(tx_buffer + sizeof(header), &capture_buffer[sample_offset], sample_bytes);

    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, (u16_t)packet_bytes, PBUF_RAM);
    if (p == NULL) {
        cyw43_arch_lwip_end();
        return ERR_MEM;
    }

    err_t err = pbuf_take(p, tx_buffer, packet_bytes);
    if (err == ERR_OK) {
        err = udp_sendto(udp, p, &udp_destination, UDP_DEST_PORT);
    }

    pbuf_free(p);
    cyw43_arch_lwip_end();
    return err;
}

static bool send_capture(uint16_t trigger_index) {
    const uint32_t id = ++capture_id;

    for (uint32_t offset = 0; offset < CAPTURE_SAMPLES; offset += UDP_SAMPLES_PER_PACKET) {
        uint32_t remaining = CAPTURE_SAMPLES - offset;
        uint16_t count = (remaining > UDP_SAMPLES_PER_PACKET)
                             ? (uint16_t)UDP_SAMPLES_PER_PACKET
                             : (uint16_t)remaining;

        if (send_packet(id, offset, count, trigger_index) != ERR_OK) {
            return false;
        }

        // Small spacing reduces burst loss on simple hotspots.
        sleep_ms(2);
    }

    return true;
}

int main(void) {
    if (!wifi_connect()) {
        while (true) {
            sleep_ms(1000);
        }
    }

    if (!udp_init()) {
        while (true) {
            set_led(false);
            sleep_ms(200);
            set_led(true);
            sleep_ms(200);
        }
    }

    adc_dma_init();

    while (true) {
        acquire_block();

        uint16_t trigger_index = NO_TRIGGER_INDEX;
        uint16_t peak_delta = 0;
        if (!find_trigger(&trigger_index, &peak_delta)) {
            continue;
        }

        (void)peak_delta; // Reserved for protocol metadata in a later revision.

        set_led(false);
        (void)send_capture(trigger_index);
        set_led(true);

        sleep_ms(EVENT_HOLDOFF_MS);
    }
}
