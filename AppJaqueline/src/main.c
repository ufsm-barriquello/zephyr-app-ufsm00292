#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

const struct device *sensor;

static struct sensor_value temperature;

void main(void)
{
    int err;

    printk("=== Iniciando leitura do BNO055 ===\n");

 
    sensor = DEVICE_DT_GET_ANY(bosch_bno055);
    

    if (sensor == NULL) {
        sensor = device_get_binding("BNO055");
    }

    if (sensor == NULL) {
        printk("ERRO: Sensor BNO055 não encontrado\n");
        return;
    }

    if (!device_is_ready(sensor)) {
        printk("ERRO: Sensor BNO055 não está pronto\n");
        return;
    }

    printk("SUCESSO: Sensor BNO055 encontrado e pronto\n");

    while (1) {
        err = sensor_sample_fetch(sensor);
        if (err) {
            printk("Erro ao ler sensor: %d\n", err);
            k_sleep(K_MSEC(1000));
            continue;
        }

        err = sensor_channel_get(sensor, SENSOR_CHAN_DIE_TEMP, &temperature);
        if (err) {
            printk("Erro ao ler temperatura: %d\n", err);
        } else {
            printk("Temperatura: %d.%02d C\n", temperature.val1, 
                   abs(temperature.val2 / 10000));
        }

        k_sleep(K_MSEC(2000));
    }
}