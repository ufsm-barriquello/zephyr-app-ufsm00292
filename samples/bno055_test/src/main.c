/* app/src/main.c */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

int main(void)
{
    printf("Iniciando aplicacao de teste do BNO055...\n");

    /* * 1. OBTER O DISPOSITIVO
     * O Zephyr busca a struct device associada ao nó do Devicetree.
     * 'DT_NODELABEL(bno055)' refere-se ao rótulo que criamos no app.overlay
     */
    const struct device *dev = DEVICE_DT_GET(DT_ANY_INST(bosch_bno055));

    /* * 2. VERIFICAR SE O DRIVER ESTÁ PRONTO
     * Isso verifica se a função de inicialização (bno055_init) rodou sem erros.
     */
    if (!device_is_ready(dev)) {
        printf("ERRO: Dispositivo BNO055 nao esta pronto ou nao foi encontrado.\n");
        return 0;
    }

    printf("Dispositivo BNO055 encontrado! Lendo dados...\n");

    while (1) {
        struct sensor_value val_x, val_y, val_z;

        /* * 3. BUSCAR DADOS (FETCH)
         * Isso chama a função 'sample_fetch' que você implementou no driver.
         * Ela vai lá no I2C, lê os registradores e guarda na memória interna do driver.
         */
        if (sensor_sample_fetch(dev) < 0) {
            printf("Falha na leitura (sample_fetch)\n");
            k_msleep(1000);
            continue;
        }

        /* * 4. OBTER CANAIS (GET)
         * Isso chama a função 'channel_get' do driver para converter os dados
         * brutos para o formato padrão do Zephyr (sensor_value).
         * Estamos pedindo Aceleração ou Orientação (depende do que você implementou).
         * Vamos assumir que você implementou SENSOR_CHAN_ROTATION ou ACCEL.
         */
        
        // Exemplo: Lendo Orientação (Euler Angles) se seu driver suportar SENSOR_CHAN_ROTATION
        // Caso contrário, tente SENSOR_CHAN_ACCEL_XYZ
        sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val_x); // Roll (ou X)
        
        /*
         * O Zephyr usa uma struct com parte inteira e decimal para não depender de float.
         * sensor_value_to_double converte isso para facilitar o print.
         */
        printf("Leitura: %f\n", sensor_value_to_double(&val_x));

        k_msleep(500); // Aguarda 500ms
    }
    return 0;
}
