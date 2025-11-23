#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_lux);

#define SLEEP_TIME_MS 1000

int main(void)
{
    const struct device *dev;

    LOG_INF("=== Sensor de Luminosidade V2 ===");

    /*
     * IMPORTANTE:
     * Verifique o label no DTS da sua placa
     * Por exemplo:
     *  - "bh1750@23"
     *  - "opt3001@44"
     *
     * Use o nome declarado no device tree (.overlay)
     */
    dev = DEVICE_DT_GET_ONE(rohm_bh1750);

    if (!device_is_ready(dev)) {
        LOG_ERR("Sensor BH1750 não está pronto!");
        return -1;
    }

    struct sensor_value lux;

    while (1) {
        int ret = sensor_sample_fetch(dev);

        if (ret != 0) {
            LOG_ERR("Erro lendo sensor: %d", ret);
            continue;
        }

        ret = sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &lux);

        if (ret != 0) {
            LOG_ERR("Erro ao acessar canal de luz: %d", ret);
            continue;
        }

        LOG_INF("Luminosidade: %.2f lux", sensor_value_to_double(&lux));

        k_msleep(SLEEP_TIME_MS);
    }
}

