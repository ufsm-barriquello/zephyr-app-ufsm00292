#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>
#include <string.h>

/* --------- Sensores IO1 ---------- */

/* Temp AT30TSE75x (I2C) na IO1 */
#define TEMP_SENSOR_ADDR 0x4F
#define TEMP_REG         0x00

/* Luz TEMT6000 no ADC AIN6 / PA06 */
#define ADC_CH           6
#define ADC_RES          12

static const struct adc_channel_cfg adc_cfg = {
    .gain             = ADC_GAIN_1,
    .reference        = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id       = ADC_CH,
    .input_positive   = ADC_CH,
};

static float    g_temperature = 0.0f;
static float    g_light_pct   = 0.0f;
static uint16_t g_light_raw   = 0;

/* Protótipo */
static int read_temperature(const struct device *i2c, float *t);

/* --------- MAIN ---------- */

void main(void)
{
    const struct device *i2c_dev;
    const struct device *adc_dev;
    int16_t adc_buf;

    struct adc_sequence seq = {
        .channels    = BIT(ADC_CH),
        .buffer      = &adc_buf,
        .buffer_size = sizeof(adc_buf),
        .resolution  = ADC_RES,
    };

    printk("\n=============================================\n");
    printk(" SAMR21 + IO1 - IEEE 802.15.4 + UDP\n");
    printk(" Sensores (temp + luz) enviando JSON + MQTT\n");
    printk("=============================================\n\n");

    /* --- Inicializa I2C --- */
    i2c_dev = DEVICE_DT_GET(DT_ALIAS(i2c_0));
    if (!device_is_ready(i2c_dev)) {
        printk("[ERRO] I2C nao pronto\n");
    } else {
        printk("[OK] I2C pronto\n");
    }

    /* --- Inicializa ADC --- */
    adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
    if (!device_is_ready(adc_dev)) {
        printk("[ERRO] ADC nao pronto\n");
    } else if (adc_channel_setup(adc_dev, &adc_cfg) == 0) {
        printk("[OK] ADC configurado (CANAL %d - PA06)\n", ADC_CH);
    } else {
        printk("[ERRO] adc_channel_setup\n");
    }

    /* --- Configura rede IPv6 sobre 802.15.4 --- */
    struct net_if *iface = net_if_get_default();
    if (iface) {
        /* endereço link-local manual: fe80::1234 */
        struct in6_addr my_addr = {
            .s6_addr = { 0xfe, 0x80, 0,0,0,0,0,0,
                         0,0,0,0,0,0x12,0x34 }
        };

        if (net_if_ipv6_addr_add(iface, &my_addr,
                                 NET_ADDR_MANUAL, 0)) {
            printk("[OK] IPv6 local: fe80::1234\n");
        } else {
            printk("[ERRO] falha ao adicionar IPv6 local\n");
        }

        net_if_up(iface);
        printk("[OK] Interface de rede UP\n");
    }

    /* --- Cria socket UDP (802.15.4) --- */
    int sock = -1;
    struct sockaddr_in6 dest;

    sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock >= 0) {
        memset(&dest, 0, sizeof(dest));
        dest.sin6_family = AF_INET6;
        dest.sin6_port   = htons(4242);

        /* Destino: fe80::1 (::1 no final) */
        dest.sin6_addr.s6_addr[0]  = 0xfe;
        dest.sin6_addr.s6_addr[1]  = 0x80;
        dest.sin6_addr.s6_addr[15] = 0x01;

        printk("[OK] Socket UDP criado, destino fe80::1:4242\n");
    } else {
        printk("[ERRO] zsock_socket UDP\n");
    }

    int  count = 0;
    char json_udp[128];
    char json_serial[80];

    while (1) {
        printk("\n[%d] -------------------------\n", count);

        /* ---- Temperatura ---- */
        if (read_temperature(i2c_dev, &g_temperature) == 0) {
            printk("  Temp: %.2f C\n", (double)g_temperature);
        } else {
            printk("  Temp: ERRO\n");
        }

        /* ---- Luz ---- */
        if (adc_read(adc_dev, &seq) == 0) {
            g_light_raw = adc_buf;
            g_light_pct = (g_light_raw * 100.0f) / 4095.0f;
            printk("  Luz: %u/4095 (%.1f%%)\n",
                   g_light_raw, (double)g_light_pct);
        } else {
            printk("  Luz: ERRO ADC\n");
        }

        /* ---- Monta JSON para UDP ---- */
        snprintk(json_udp, sizeof(json_udp),
                 "{\"temp\":%.2f,\"light_pct\":%.1f,\"light_raw\":%u}",
                 (double)g_temperature,
                 (double)g_light_pct,
                 g_light_raw);

        printk("  Enviando UDP: %s\n", json_udp);

        if (sock >= 0) {
            int ret = zsock_sendto(sock, json_udp, strlen(json_udp), 0,
                                   (struct sockaddr *)&dest,
                                   sizeof(dest));
            if (ret < 0) {
                printk("  [ERRO] sendto = %d\n", ret);
            } else {
                printk("  [OK] %d bytes enviados\n", ret);
            }
        }

        /* ---- Monta JSON para SERIAL (MQTT) ---- */
        snprintk(json_serial, sizeof(json_serial),
                 "{\"temp\":%.2f,\"light_pct\":%.1f}\n",
                 (double)g_temperature,
                 (double)g_light_pct);

        /* Linha pura em JSON para o script Python */
        printk("%s", json_serial);

        count++;
        k_sleep(K_SECONDS(2));
    }
}

/* --------- Temperatura I2C ---------- */

static int read_temperature(const struct device *i2c, float *temp)
{
    uint8_t reg = TEMP_REG;
    uint8_t buf[2];

    if (!device_is_ready(i2c)) {
        return -1;
    }

    if (i2c_write_read(i2c, TEMP_SENSOR_ADDR, &reg, 1, buf, 2) != 0) {
        return -1;
    }

    int16_t raw = ((int16_t)buf[0] << 8) | buf[1];
    *temp = raw / 256.0f;  /* 1/256 °C por LSB */

    return 0;
}
