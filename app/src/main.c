#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_lux, LOG_LEVEL_INF);

// ---------------- ADC CONFIG ----------------
#define ADC_NODE DT_PATH(zephyr_user, sensor_luminosidade)
#define ADC_CHANNEL 1
#define ADC_RESOLUTION 12

#define ADC_GAIN ADC_GAIN_1
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT

static const struct device *adc_dev;
static int16_t sample_buf;

// ---------------- LED CONFIG ----------------
#define LED_NODE DT_ALIAS(led0)
static const struct device *led_dev;

// ---------------- SENSING CONFIG ----------------
#define AVG_WINDOW 10
static uint16_t adc_window[AVG_WINDOW];
static uint8_t index_avg = 0;
static uint32_t avg_sum = 0;

// *** Calibração ***
#define R_REF 10000       // resistor fixo (em ohms)
#define MAX_ADC 4095.0
#define VCC 3.3

// Intervalo ajustável
static uint32_t read_period_ms = 500;

// ------------- Funções auxiliares -------------

float adc_to_voltage(int raw) {
    return (raw / MAX_ADC) * VCC;
}

float ldr_to_lux(float voltage) {
    if (voltage <= 0.05) return 0.1;
    float r_ldr = (R_REF * (VCC - voltage)) / voltage;
    return 500 * (10000.0 / r_ldr);
}

uint16_t avg_filter(uint16_t value) {
    avg_sum -= adc_window[index_avg];
    adc_window[index_avg] = value;
    avg_sum += value;

    index_avg = (index_avg + 1) % AVG_WINDOW;
    return avg_sum / AVG_WINDOW;
}

// -------- INITIALIZATION --------

void init_adc(void) {
    adc_dev = DEVICE_DT_GET(ADC_NODE);
    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC não disponível");
        return;
    }

    struct adc_channel_cfg cfg = {
        .gain = ADC_GAIN,
        .reference = ADC_REFERENCE,
        .acquisition_time = ADC_ACQUISITION_TIME,
        .channel_id = ADC_CHANNEL,
        .differential = false,
    };

    if (adc_channel_setup(adc_dev, &cfg) != 0) {
        LOG_ERR("Erro ADC setup");
    }
}

int read_adc_raw(void) {
    struct adc_sequence seq = {
        .channels = BIT(ADC_CHANNEL),
        .buffer = &sample_buf,
        .buffer_size = sizeof(sample_buf),
        .resolution = ADC_RESOLUTION,
    };
    adc_read(adc_dev, &seq);
    return sample_buf;
}

// -------- LED --------

void led_update(float lux) {
    gpio_pin_set(led_dev, 0, lux < 100 ? 1 : 0);
}

// -------- MAIN --------

int main() {
    LOG_INF("=== Sensor de Luminosidade Avançado ===");

    init_adc();

    led_dev = DEVICE_DT_GET(LED_NODE);
    gpio_pin_configure(led_dev, 0, GPIO_OUTPUT_INACTIVE);

    while (1) {
        int raw = read_adc_raw();
        uint16_t smooth = avg_filter(raw);

        float v = adc_to_voltage(smooth);
        float lux = ldr_to_lux(v);

        LOG_INF("[ADC=%d] Voltage=%.3fV | Luminosidade aprox=%.1f lux", smooth, v, lux);

        led_update(lux);

        k_sleep(K_MSEC(read_period_ms));
    }
}
