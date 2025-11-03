#include "meu_sensor.h"

static float fake_temp = 20.0f;

float get_sensor_temp(void)
{
    fake_temp += 0.5f;

    if (fake_temp > 30.0f) {
        fake_temp = 20.0f;
    }

    return fake_temp;
}