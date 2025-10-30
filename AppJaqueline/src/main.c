#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>

const struct device *sensor;

static struct sensor_value temperature, humidity;

void main(void)
{
    int err;

    printk("Iniciando leitura do BNO055...\n");

  
    sensor = device_get_binding("BNO055");
    if(sensor == NULL || !device_is_ready(sensor)){
        printk("Sensor não encontrado ou não pronto\n");
        return;
    }

    while(1){
        err = sensor_sample_fetch(sensor);
        if(err){
            printk("Erro ao ler sensor: %d\n", err);
            return;
        }

        sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
        sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &humidity);

        printk("Temperatura: %d.%06d C\n", temperature.val1, temperature.val2);
        printk("Humidade: %d.%06d %%\n", humidity.val1, humidity.val2);

        k_sleep(K_MSEC(2000));
    }
}