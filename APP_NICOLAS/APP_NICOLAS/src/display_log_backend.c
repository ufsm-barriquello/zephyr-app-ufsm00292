#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

LOG_MODULE_REGISTER(display_logger, LOG_LEVEL_INF);

// buffer temporário para o texto
static uint8_t buf[256];

// instancia o output
static void write_to_display(uint8_t *data, size_t len)
{
    const struct device *display_dev = DEVICE_DT_GET_ANY(zephyr_display);

    if (!device_is_ready(display_dev)) {
        return;
    }

    /* --- EXEMPLO DE IMPRESSÃO NO DISPLAY ---
       Aqui você vai substituir pela sua API !!!
    */

    /* Exemplo genérico (não funciona sem sua API):
    display_write_string(display_dev, data, len);
    */

    /* só pra visualizar no console, debug */
    printk("DISPLAY: %s\n", data);
}

LOG_OUTPUT_DEFINE(display_output, write_to_display, buf, sizeof(buf));

static void display_log_process(const struct log_backend *const backend,
                                union log_msg_generic *msg)
{
    uint32_t flags = LOG_OUTPUT_FLAG_FORMAT_SYST |
                     LOG_OUTPUT_FLAG_LEVEL |
                     LOG_OUTPUT_FLAG_TIMESTAMP;

    log_output_msg_process(&display_output, &msg->log, flags);
}

static void display_log_put(const struct log_backend *const backend,
                            struct log_msg *msg)
{
    log_msg_get(msg);
    display_log_process(backend, (union log_msg_generic *)msg);
    log_msg_put(msg);
}

static void display_log_panic(const struct log_backend *const backend)
{
    log_output_flush(&display_output);
}

const struct log_backend_api display_log_backend_api = {
    .put = display_log_put,
    .panic = display_log_panic,
};

LOG_BACKEND_DEFINE(display_backend, display_log_backend_api, true);
