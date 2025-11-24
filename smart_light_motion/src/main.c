#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

#define ADC_NODE DT_ALIAS(ldr0)
#define PIR_NODE DT_ALIAS(pir0)
#define LED_NODE DT_ALIAS(led0)

static const struct device *pir_dev;
static const struct device *led_dev;
static const struct device *adc_dev;

#define ADC_RESOLUTION 12
#define ADC_CHANNEL_ID 1
#define ADC_GAIN        ADC_GAIN_1
#define ADC_REFERENCE   ADC_REF_INTERNAL

static int16_t adc_sample;

static const struct adc_channel_cfg adc_cfg = {
    .gain = ADC_GAIN,
    .reference = ADC_REFERENCE,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = ADC_CHANNEL_ID,
};

void main()
{
    printk("=== SMART LIGHT: PIR + LDR ===\n");

    pir_dev = DEVICE_DT_GET(PIR_NODE);
    led_dev = DEVICE_DT_GET(LED_NODE);
    adc_dev = DEVICE_DT_GET(ADC_NODE);

    gpio_pin_configure(pir_dev, 10, GPIO_INPUT);
    gpio_pin_configure(led_dev, 15, GPIO_OUTPUT_INACTIVE);

    adc_channel_setup(adc_dev, &adc_cfg);

    struct adc_sequence seq = {
        .channels = BIT(ADC_CHANNEL_ID),
        .buffer = &adc_sample,
        .buffer_size = sizeof(adc_sample),
        .resolution = ADC_RESOLUTION,
    };

    while (1) {
        adc_read(adc_dev, &seq);

        int ldr = adc_sample;
        bool movimento = gpio_pin_get(pir_dev, 10);

        printk("LDR=%d | PIR=%d\n", ldr, movimento);

        if (ldr < 1600 && movimento) {
            gpio_pin_set(led_dev, 15, 1);
        } else {
            gpio_pin_set(led_dev, 15, 0);
        }

        k_msleep(200);
    }
}
