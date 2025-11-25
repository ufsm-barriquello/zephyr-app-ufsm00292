/*
 * SAMR21 Xplained Pro + BNO055 Xplained Pro (I2C no SERCOM1)
 * Leitura de quaternion + Euler e envio via USB CDC
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>

/* BNO055 I2C Address (Xplained Pro) */
#define BNO055_ADDR 0x29  /* <--- ENDEREÇO CORRETO */

/* BNO055 Registers */
#define BNO055_CHIP_ID_REG       0x00
#define BNO055_OPR_MODE_REG      0x3D
#define BNO055_PWR_MODE_REG      0x3E
#define BNO055_EULER_H_LSB       0x1A
#define BNO055_QUAT_DATA_W_LSB   0x20

/* Operation Modes */
#define BNO055_MODE_CONFIG   0x00
#define BNO055_MODE_NDOF     0x0C

/* Usa o alias oficial da placa */
#define I2C_NODE DT_ALIAS(i2c_0)
static const struct device *i2c_dev;

/* Estrutura de dados */
struct imu_data {
    float qw, qx, qy, qz;
    float heading, roll, pitch;
};

/* Inicializa BNO055 */
int bno055_init(void)
{
    uint8_t chip_id;
    int ret;

    /* Lê Chip ID */
    ret = i2c_reg_read_byte(i2c_dev, BNO055_ADDR, BNO055_CHIP_ID_REG, &chip_id);
    if (ret < 0) {
        printk("Erro ao ler Chip ID (ret=%d)\n", ret);
        return ret;
    }

    if (chip_id != 0xA0) {
        printk("Chip ID invalido: 0x%02X (esperado 0xA0)\n", chip_id);
        return -1;
    }

    printk("BNO055 detectado! Chip ID: 0x%02X\n", chip_id);

    /* Modo config */
    i2c_reg_write_byte(i2c_dev, BNO055_ADDR, BNO055_OPR_MODE_REG, BNO055_MODE_CONFIG);
    k_msleep(25);

    /* Power mode normal */
    i2c_reg_write_byte(i2c_dev, BNO055_ADDR, BNO055_PWR_MODE_REG, 0x00);
    k_msleep(10);

    /* Modo NDOF */
    i2c_reg_write_byte(i2c_dev, BNO055_ADDR, BNO055_OPR_MODE_REG, BNO055_MODE_NDOF);
    k_msleep(20);

    printk("BNO055 inicializado no modo NDOF.\n");
    return 0;
}

/* Lê quaternion */
int bno055_read_quaternion(struct imu_data *d)
{
    uint8_t buf[8];
    int ret = i2c_burst_read(i2c_dev, BNO055_ADDR, BNO055_QUAT_DATA_W_LSB, buf, 8);
    if (ret < 0) return ret;

    int16_t qw = (buf[1] << 8) | buf[0];
    int16_t qx = (buf[3] << 8) | buf[2];
    int16_t qy = (buf[5] << 8) | buf[4];
    int16_t qz = (buf[7] << 8) | buf[6];

    d->qw = qw / 16384.0f;
    d->qx = qx / 16384.0f;
    d->qy = qy / 16384.0f;
    d->qz = qz / 16384.0f;

    return 0;
}

/* Lê Euler (Heading, Roll, Pitch) */
int bno055_read_euler(struct imu_data *d)
{
    uint8_t buf[6];
    int ret = i2c_burst_read(i2c_dev, BNO055_ADDR, BNO055_EULER_H_LSB, buf, 6);
    if (ret < 0) return ret;

    int16_t heading = (buf[1] << 8) | buf[0];
    int16_t roll    = (buf[3] << 8) | buf[2];
    int16_t pitch   = (buf[5] << 8) | buf[4];

    d->heading = heading / 16.0f;
    d->roll    = roll    / 16.0f;
    d->pitch   = pitch   / 16.0f;

    return 0;
}

/* Main */
int main(void)
{
    struct imu_data data;
    char json[128];

    printk("\n=== SAMR21 + BNO055 IMU Reader ===\n");

    /* Obtém o dispositivo I2C correto */
    i2c_dev = DEVICE_DT_GET(I2C_NODE);
    if (!device_is_ready(i2c_dev)) {
        printk("ERRO: I2C (sercom1 / i2c-0) nao esta pronto!\n");
        return -1;
    }

    printk("I2C inicializado (sercom1 / i2c-0)\n");

    /* Inicializa sensor */
    if (bno055_init() < 0) {
        printk("Falha ao inicializar o BNO055\n");
        return -1;
    }

    printk("Aguardando calibracao...\n");
    k_msleep(2000);

    printk("Iniciando leitura...\n");

    while (1) {

        if (bno055_read_quaternion(&data) < 0) {
            printk("Falha ao ler quaternion\n");
            k_msleep(100);
            continue;
        }

        if (bno055_read_euler(&data) < 0) {
            printk("Falha ao ler euler\n");
            k_msleep(100);
            continue;
        }

        snprintf(json, sizeof(json),
            "{\"qw\":%.4f,\"qx\":%.4f,\"qy\":%.4f,\"qz\":%.4f,"
            "\"h\":%.2f,\"r\":%.2f,\"p\":%.2f}\n",
            data.qw, data.qx, data.qy, data.qz,
            data.heading, data.roll, data.pitch
        );

        printk("%s", json);
        k_msleep(50);  // 20 Hz
    }

    return 0;
}
