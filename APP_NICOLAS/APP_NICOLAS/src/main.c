#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/services/nus.h>

#include <zephyr/drivers/gpio.h>
#include <string.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* Advertising: flags + nome do device */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* LED genérico usando alias "led0" da placa */
static const struct gpio_dt_spec led0 =
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

static struct bt_conn *current_conn;

/* Envia string pelo NUS (se estiver conectado) */
static void ble_send_str(const char *s)
{
    int err;

    if (!current_conn) {
        return;
    }

    err = bt_nus_send(current_conn, (const uint8_t *)s, strlen(s));
    if (err) {
        LOG_WRN("bt_nus_send falhou (%d)", err);
    }
}

/* Callback: dados recebidos do celular via NUS */
static void nus_receive_cb(struct bt_conn *conn,
                           const uint8_t *const data,
                           uint16_t len)
{
    char buf[32];

    uint16_t copy_len = len;
    if (copy_len >= sizeof(buf)) {
        copy_len = sizeof(buf) - 1;
    }

    memcpy(buf, data, copy_len);
    buf[copy_len] = '\0';

    LOG_INF("CMD BLE: %s", log_strdup(buf));

    if (strcmp(buf, "ping") == 0) {
        ble_send_str("pong\n");

    } else if (strcmp(buf, "led on") == 0) {
        if (led0.port) {
            gpio_pin_set_dt(&led0, 1);
        }
        ble_send_str("LED ON\n");

    } else if (strcmp(buf, "led off") == 0) {
        if (led0.port) {
            gpio_pin_set_dt(&led0, 0);
        }
        ble_send_str("LED OFF\n");

    } else if (strcmp(buf, "help") == 0) {
        ble_send_str("cmds: ping, led on, led off, help\n");

    } else {
        ble_send_str("comando desconhecido\n");
    }
}

static struct bt_nus_cb nus_cb = {
    .received = nus_receive_cb,
};

/* Callbacks de conexão BLE */
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Falha ao conectar (%u)", err);
        return;
    }

    current_conn = bt_conn_ref(conn);
    LOG_INF("Central conectou");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Central desconectou (reason %u)", reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

int main(void)
{
    int err;

    LOG_INF("APP_NICOLAS BLE + NUS iniciando");

    /* Configura LED, se existir alias led0 */
    if (led0.port) {
        if (!device_is_ready(led0.port)) {
            LOG_WRN("LED0 não está pronto");
        } else {
            gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
        }
    }

    /* Inicializa Bluetooth (host) */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("bt_enable falhou (%d)", err);
        return 0;
    }
    LOG_INF("Bluetooth inicializado");

    /* Inicializa NUS (UART BLE) */
    err = bt_nus_init(&nus_cb);
    if (err) {
        LOG_ERR("bt_nus_init falhou (%d)", err);
        return 0;
    }

    /* Começa advertising */
    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Falha ao iniciar advertising (%d)", err);
        return 0;
    }

    LOG_INF("Anunciando como \"%s\"", DEVICE_NAME);
    LOG_INF("Conecte com app BLE (NUS) e mande \"help\"");

    while (1) {
        LOG_INF("Loop...");
        if (current_conn) {
            ble_send_str("log: loop...\n");
        }
        k_sleep(K_SECONDS(5));
    }

    return 0;
}


