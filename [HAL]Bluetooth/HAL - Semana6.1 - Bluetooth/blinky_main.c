#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define BLINK_PERIOD_MS 500

/* Usa o alias 'led0' definido no Devicetree da placa */
static const struct gpio_dt_spec led =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static int setup_led(void)
{
    if (!device_is_ready(led.port)) {
        return -ENODEV;
    }

    return gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
}

int main(void)
{
    if (setup_led() < 0) {
        /* Em um projeto real, poderíamos logar o erro aqui */
        return 0;
    }

    while (true) {
        gpio_pin_toggle_dt(&led);
        k_msleep(BLINK_PERIOD_MS);
    }

    return 0;
}
