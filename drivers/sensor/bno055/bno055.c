#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

// Registradores do BNO055 (iguais aos do bare-metal)
#define BNO055_REG_CHIP_ID      (0x00)
#define BNO055_REG_OPR_MODE     (0x3D)
#define BNO055_REG_EUL_HEADING_LSB (0x1A)
#define BNO055_MODE_NDOF        (0x0C)

/*
 * Estrutura de dados "privada" do driver.
 * Armazena os dados lidos e o estado.
 */
struct bno055_data {
    struct i2c_dt_spec i2c; // O Zephyr preenche isso com os dados do Devicetree!
    
    // Armazena os dados de orientação
    int16_t heading_raw;
    int16_t roll_raw;
    int16_t pitch_raw;
};

/*
 * FUNÇÃO DE LEITURA (Fetch):
 * Esta é a função que o Zephyr chama quando a APP pede dados.
 */
static int bno055_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    struct bno055_data *data = dev->data;
    uint8_t euler_data[6];

    // USA A API I2C DO ZEPHYR (em vez de mexer nos registradores do SERCOM)
    if (i2c_burst_read_dt(&data->i2c, BNO055_REG_EUL_HEADING_LSB, euler_data, 6) < 0) {
        return -EIO; // Erro de I/O
    }

    // Processa os dados e salva na struct 'data'
    data->heading_raw = ((int16_t)euler_data[1] << 8) | euler_data[0];
    data->roll_raw    = ((int16_t)euler_data[3] << 8) | euler_data[2];
    data->pitch_raw   = ((int16_t)euler_data[5] << 8) | euler_data[4];

    return 0;
}

/*
 * FUNÇÃO DE LEITURA (Get):
 * Esta é a função que a APP usa para pegar os dados já lidos.
 */
static int bno055_channel_get(const struct device *dev,
                              enum sensor_channel chan,
                              struct sensor_value *val)
{
    // Esta função converte os dados brutos (raw) para a 
    // estrutura 'sensor_value' que a APP espera.
    // ... (lógica de conversão de int16 para float/double) ...
    return 0;
}


/*
 * FUNÇÃO DE INICIALIZAÇÃO:
 * O Zephyr executa isso automaticamente na inicialização do sistema.
 */
static int bno055_init(const struct device *dev)
{
    struct bno055_data *data = dev->data;
    uint8_t chip_id = 0;

    // 1. Pega a configuração do I2C do Devicetree
    data->i2c = I2C_DT_SPEC_GET(DT_INST(0, bosch_bno055));

    // 2. Verifica se o dispositivo está pronto
    if (!device_is_ready(data->i2c.bus)) {
        return -ENODEV;
    }

    // 3. Verifica o Chip ID (Usando a API I2C do Zephyr)
    if (i2c_reg_read_byte_dt(&data->i2c, BNO055_REG_CHIP_ID, &chip_id) < 0) {
        return -EIO;
    }

    if (chip_id != 0xA0) {
        return -ENODEV;
    }

    // 4. Configura o modo de operação NDOF
    if (i2c_reg_write_byte_dt(&data->i2c, BNO055_REG_OPR_MODE, BNO055_MODE_NDOF) < 0) {
        return -EIO;
    }

    return 0;
}

/*
 * ESTRUTURA DA API:
 * Mapeia nossas funções para as chamadas genéricas do Zephyr.
 */
static const struct sensor_driver_api bno055_api = {
    .sample_fetch = bno055_sample_fetch,
    .channel_get = bno055_channel_get,
};

/*
 * "MÁGICA" DO ZEPHYR:
 * Isso registra seu driver no sistema.
 * Ele conecta:
 * - O "compatible" do Devicetree ("bosch,bno055")
 * - A função de inicialização (bno055_init)
 * - As estruturas de dados e a API
 */
DEVICE_DT_INST_DEFINE(0,
                      bno055_init,
                      NULL,
                      &bno055_data_instance, // Sua struct de dados
                      NULL,
                      POST_KERNEL,
                      CONFIG_SENSOR_INIT_PRIORITY,
                      &bno055_api); // Sua struct de API
