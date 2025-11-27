/*
 * SAMR21 Thread End Device - Zephyr 3.7 LTS
 * Transmite dados simulados via Thread/CoAP
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/openthread.h>
#include <zephyr/usb/usb_device.h>
#include <openthread/thread.h>
#include <openthread/ip6.h>
#include <string.h>

LOG_MODULE_REGISTER(samr21_sensor, LOG_LEVEL_INF);

#define COAP_PORT 5683
#define SENSOR_READ_INTERVAL_MS 100  // 10Hz

/* Estrutura para dados do sensor */
struct sensor_data {
    double quat_w;
    double quat_x;
    double quat_y;
    double quat_z;
    double euler_h;
    double euler_r;
    double euler_p;
    double accel_x;
    double accel_y;
    double accel_z;
    uint64_t timestamp;
};

static struct sensor_data current_data;
static K_MUTEX_DEFINE(data_mutex);

/* Simulador de dados do sensor */
static void simulate_sensor_data(struct sensor_data *data)
{
    static float angle = 0.0f;
    angle += 0.05f;
    if (angle > 6.28f) angle = 0.0f;
    
    /* Simula quaternion rotacionando em Z - aproximações simples */
    data->quat_w = 1.0 - (angle * angle / 8.0);  // cos aproximado
    data->quat_x = 0.0;
    data->quat_y = 0.0;
    data->quat_z = angle / 2.0;  // sin aproximado para ângulos pequenos
    
    /* Simula ângulos de Euler */
    data->euler_h = (double)(angle * 57.2958f);
    data->euler_r = (double)(angle * 10.0f);
    data->euler_p = (double)((6.28f - angle) * 10.0f);
    
    /* Simula aceleração */
    data->accel_x = (double)(angle * 2.0f);
    data->accel_y = (double)((6.28f - angle) * 2.0f);
    data->accel_z = 9.81;
    
    data->timestamp = k_uptime_get();
}

/* Thread de leitura do sensor */
static void sensor_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    LOG_INF("Sensor thread started (simulating data)");
    
    while (1) {
        struct sensor_data data;
        simulate_sensor_data(&data);
        
        k_mutex_lock(&data_mutex, K_FOREVER);
        memcpy(&current_data, &data, sizeof(struct sensor_data));
        k_mutex_unlock(&data_mutex);
        
        if ((k_uptime_get() / 1000) % 2 == 0) {
            LOG_INF("Q: w=%.3f x=%.3f y=%.3f z=%.3f | E: h=%.1f r=%.1f p=%.1f",
                    data.quat_w, data.quat_x, data.quat_y, data.quat_z,
                    data.euler_h, data.euler_r, data.euler_p);
        }
        
        k_msleep(SENSOR_READ_INTERVAL_MS);
    }
}

/* CoAP handler */
static int sensor_coap_handler(struct coap_resource *resource,
                               struct coap_packet *request,
                               struct sockaddr *addr, socklen_t addr_len)
{
    struct sensor_data data;
    uint8_t response_buf[256];
    struct coap_packet response;
    char payload[200];
    int ret;
    uint8_t token[COAP_TOKEN_MAX_LEN];
    uint8_t token_len;
    uint16_t msg_id;
    
    k_mutex_lock(&data_mutex, K_FOREVER);
    memcpy(&data, &current_data, sizeof(struct sensor_data));
    k_mutex_unlock(&data_mutex);
    
    ret = snprintf(payload, sizeof(payload),
             "{\"quat\":{\"w\":%.3f,\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},"
             "\"euler\":{\"h\":%.1f,\"r\":%.1f,\"p\":%.1f},"
             "\"accel\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},"
             "\"ts\":%llu}",
             data.quat_w, data.quat_x, data.quat_y, data.quat_z,
             data.euler_h, data.euler_r, data.euler_p,
             data.accel_x, data.accel_y, data.accel_z,
             data.timestamp);
    
    if (ret < 0 || ret >= sizeof(payload)) {
        return -EINVAL;
    }
    
    token_len = coap_header_get_token(request, token);
    msg_id = coap_header_get_id(request);
    
    ret = coap_packet_init(&response, response_buf, sizeof(response_buf),
                          COAP_VERSION_1, COAP_TYPE_ACK,
                          token_len, token,
                          COAP_RESPONSE_CODE_CONTENT,
                          msg_id);
    
    if (ret < 0) {
        return ret;
    }
    
    ret = coap_packet_append_payload_marker(&response);
    if (ret < 0) {
        return ret;
    }
    
    ret = coap_packet_append_payload(&response, (uint8_t *)payload, strlen(payload));
    if (ret < 0) {
        return ret;
    }
    
    ret = sendto(*(int *)resource->user_data, response.data, response.offset, 0,
                addr, addr_len);
    
    if (ret > 0) {
        LOG_INF("CoAP response sent (%d bytes)", ret);
    }
    
    return ret;
}

static struct coap_resource resources[] = {
    {
        .path = (const char *[]){ "sensor", "data", NULL },
        .get = sensor_coap_handler,
    },
};

/* Thread CoAP server */
static void coap_server_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    int sock;
    struct sockaddr_in6 addr;
    uint8_t request_buf[256];
    
    LOG_INF("Waiting for Thread network...");
    k_sleep(K_SECONDS(10));
    
    LOG_INF("Starting CoAP server on port %d", COAP_PORT);
    
    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        LOG_ERR("Failed to create socket: %d", errno);
        return;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(COAP_PORT);
    addr.sin6_addr = in6addr_any;
    
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERR("Failed to bind socket: %d", errno);
        zsock_close(sock);
        return;
    }
    
    resources[0].user_data = &sock;
    
    LOG_INF("CoAP server listening on [::]:%d", COAP_PORT);
    LOG_INF("Path: /sensor/data");
    
    while (1) {
        struct sockaddr_in6 client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        struct coap_packet request;
        int received;
        
        received = recvfrom(sock, request_buf, sizeof(request_buf), 0,
                           (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (received < 0) {
            k_msleep(100);
            continue;
        }
        
        LOG_INF("Received %d bytes from CoAP client", received);
        
        if (coap_packet_parse(&request, request_buf, received, NULL, 0) < 0) {
            LOG_ERR("Failed to parse CoAP packet");
            continue;
        }
        
        for (size_t i = 0; i < ARRAY_SIZE(resources); i++) {
            if (coap_packet_is_request(&request)) {
                sensor_coap_handler(&resources[i], &request,
                                  (struct sockaddr *)&client_addr,
                                  client_addr_len);
                break;
            }
        }
    }
}

K_THREAD_STACK_DEFINE(sensor_thread_stack, 1024);
K_THREAD_STACK_DEFINE(coap_thread_stack, 2048);

static struct k_thread sensor_thread_data;
static struct k_thread coap_thread_data;

int main(void)
{
    int ret;
    
    LOG_INF("===========================================");
    LOG_INF("  SAMR21 Thread Sensor Node (Zephyr 3.7)");
    LOG_INF("===========================================");
    
    /* Initialize USB */
    ret = usb_enable(NULL);
    if (ret != 0) {
        LOG_WRN("Failed to enable USB: %d (continuing...)", ret);
    } else {
        LOG_INF("USB CDC initialized");
        k_sleep(K_SECONDS(1));
    }
    
    /* Get OpenThread context */
    struct openthread_context *ot_context = openthread_get_default_context();
    if (!ot_context) {
        LOG_ERR("Failed to get OpenThread context");
        return -1;
    }
    
    otInstance *instance = ot_context->instance;
    
    LOG_INF("OpenThread initialized");
    LOG_INF("Device role: End Device (MTD)");
    LOG_INF("Network: %s", CONFIG_OPENTHREAD_NETWORK_NAME);
    LOG_INF("Channel: %d", CONFIG_OPENTHREAD_CHANNEL);
    LOG_INF("PAN ID: %d", CONFIG_OPENTHREAD_PANID);
    
    k_sleep(K_SECONDS(3));
    
    const otNetifAddress *addr_ptr = otIp6GetUnicastAddresses(instance);
    LOG_INF("IPv6 Addresses:");
    while (addr_ptr) {
        char addr_str[40];
        otIp6AddressToString(&addr_ptr->mAddress, addr_str, sizeof(addr_str));
        LOG_INF("  %s", addr_str);
        addr_ptr = addr_ptr->mNext;
    }
    
    LOG_INF("Starting sensor simulation...");
    
    k_thread_create(&sensor_thread_data, sensor_thread_stack,
                   K_THREAD_STACK_SIZEOF(sensor_thread_stack),
                   sensor_thread, NULL, NULL, NULL,
                   7, 0, K_NO_WAIT);
    
    k_thread_create(&coap_thread_data, coap_thread_stack,
                   K_THREAD_STACK_SIZEOF(coap_thread_stack),
                   coap_server_thread, NULL, NULL, NULL,
                   7, 0, K_NO_WAIT);
    
    LOG_INF("System ready!");
    
    return 0;
}