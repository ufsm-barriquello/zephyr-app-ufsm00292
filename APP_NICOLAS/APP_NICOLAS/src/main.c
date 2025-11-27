#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

void process_cmd(const char *cmd)
{
    if (strncmp(cmd, "ping", 4) == 0) {
        printk("pong\n");
    }

    else if (strncmp(cmd, "led on", 6) == 0) {
        gpio_pin_set_dt(&led0, 1);   // ACTIVE_LOW → 1 = OFF
        printk("LED ON\n");
    }

    else if (strncmp(cmd, "led off", 7) == 0) {
        gpio_pin_set_dt(&led0, 0);   // ACTIVE_LOW → 0 = ON
        printk("LED OFF\n");
    }

    else {
        printk("unknown command: %s\n", cmd);
    }
}

int main(void)
{
    printk("SAMR21 BLE Remote Control\n");

    if (!device_is_ready(led0.port)) {
        printk("LED0 not ready!\n");
        return 0;
    }

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);

    printk("Ready.\n");

    while (1) {
        k_msleep(1000);
    }
}

