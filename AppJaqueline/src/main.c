#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define BNO055_ADDR 0x29

#define REG_CHIP_ID          0x00
#define REG_OPR_MODE         0x3D
#define REG_PWR_MODE         0x3E
#define REG_SYS_TRIGGER      0x3F
#define OPR_MODE_CONFIG      0x00
#define OPR_MODE_NDOF        0x0C
#define REG_EULER_H_LSB      0x1A
#define REG_ACCEL_DATA_LSB   0x08
#define REG_MAG_DATA_LSB     0x0E   
#define REG_GYRO_DATA_LSB    0x14
#define REG_TEMP             0x34


static int bno_write(const struct device *i2c, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    return i2c_write(i2c, buf, 2, BNO055_ADDR);
}

static int bno_read(const struct device *i2c, uint8_t reg, uint8_t *buf, uint8_t len)
{
    return i2c_write_read(i2c, BNO055_ADDR, &reg, 1, buf, len);
}



void main(void)
{
    const struct device *i2c = DEVICE_DT_GET(DT_NODELABEL(sercom1));

    if (!device_is_ready(i2c)) {
        LOG_ERR("I2C não está pronto!");
        return;
    }

    LOG_INF("I2C pronto: %s", i2c->name);

   
    uint8_t chip_id = 0;
    bno_read(i2c, REG_CHIP_ID, &chip_id, 1);
    LOG_INF("Chip ID: 0x%02X", chip_id);

    if (chip_id != 0xA0) {
        LOG_ERR("BNO055 NÃO identificado!");
        return;
    }

    LOG_INF("BNO055 identificado!");


    bno_write(i2c, REG_OPR_MODE, OPR_MODE_CONFIG);
    k_msleep(25);

    bno_write(i2c, REG_SYS_TRIGGER, 0x20); 
    k_msleep(700);

    bno_write(i2c, REG_OPR_MODE, OPR_MODE_CONFIG);
    k_msleep(25);

    bno_write(i2c, REG_OPR_MODE, OPR_MODE_NDOF);
    k_msleep(20);

    LOG_INF("BNO055 configurado em modo NDOF.");
    LOG_INF("Iniciando leituras...");

 
    while (1) {
        LOG_INF("------------------------");
        /* --- EULER --- */
        uint8_t euler_raw[6];
        bno_read(i2c, REG_EULER_H_LSB, euler_raw, 6);

        int16_t heading = (int16_t)((euler_raw[1] << 8) | euler_raw[0]) / 16;
        int16_t roll    = (int16_t)((euler_raw[3] << 8) | euler_raw[2]) / 16;
        int16_t pitch   = (int16_t)((euler_raw[5] << 8) | euler_raw[4]) / 16;
        printf("      ");
        LOG_INF("Euler - H=%d, R=%d, P=%d", heading, roll, pitch);
        printf("      ");


        LOG_INF("------------------------");
        /* --- ACELERAÇÃO --- */
        uint8_t accel_raw[6];
        bno_read(i2c, REG_ACCEL_DATA_LSB, accel_raw, 6);

        int16_t ax = (accel_raw[1] << 8) | accel_raw[0];
        int16_t ay = (accel_raw[3] << 8) | accel_raw[2];
        int16_t az = (accel_raw[5] << 8) | accel_raw[4];
        
        printf("      ");
        LOG_INF("Aceleração - X=%d, Y=%d, Z=%d", ax, ay, az);
        printf("      ");


        LOG_INF("---------------------------");
        /* --- GIROSCÓPIO --- */
        uint8_t gyro_raw[6];
        bno_read(i2c, REG_GYRO_DATA_LSB, gyro_raw, 6);

        int16_t gx = (gyro_raw[1] << 8) | gyro_raw[0];
        int16_t gy = (gyro_raw[3] << 8) | gyro_raw[2];
        int16_t gz = (gyro_raw[5] << 8) | gyro_raw[4];
        printf("      ");
        LOG_INF("Giroscopio - X=%d, Y=%d, Z=%d", gx, gy, gz);
        printf("      ");


        LOG_INF("------------------------");
        /* --- MAGNETÔMETRO  --- */
        uint8_t mag_raw[6];
        bno_read(i2c, REG_MAG_DATA_LSB, mag_raw, 6);

        int16_t mx = (mag_raw[1] << 8) | mag_raw[0];
        int16_t my = (mag_raw[3] << 8) | mag_raw[2];
        int16_t mz = (mag_raw[5] << 8) | mag_raw[4];
        printf("      ");
        LOG_INF("Magnetometro - X=%d, Y=%d, Z=%d", mx, my, mz);
        printf("      ");

        LOG_INF("---------------------------");
        /* --- TEMPERATURA --- */
        int8_t temp_raw = 0;
        bno_read(i2c, REG_TEMP, (uint8_t *)&temp_raw, 1);
        printf("      ");
        LOG_INF("Temperatura: %d °C", temp_raw);
        printf("      ");

        LOG_INF("---------------------------");

        k_msleep(300);
    }
}