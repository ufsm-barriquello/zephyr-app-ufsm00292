/*
 * SAMR21 Border Router
 * Faz ponte entre Thread Network e PC via USB/Serial
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/coap.h>
#include <openthread/thread.h>
#include <openthread/ip6.h>

LOG_MODULE_REGISTER(border_router, LOG_LEVEL_INF);

#define PROXY_PORT 5684  /* Porta para receber requisições do PC */
#define SENSOR_PORT 5683 /* Porta do sensor node */

/* Thread para proxy CoAP: recebe do PC e encaminha para sensor */
void coap_proxy_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    int sock_proxy, sock_client;
    struct sockaddr_in6 addr_proxy;
    uint8_t buffer[512];
    
    /* Aguarda Thread network estar pronta */
    k_sleep(K_SECONDS(10));
    
    LOG_INF("Starting CoAP Proxy on port %d", PROXY_PORT);
    
    /* Socket para receber do PC (via USB/rede local se houver) */
    sock_proxy = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_proxy < 0) {
        LOG_ERR("Failed to create proxy socket");
        return;
    }
    
    memset(&addr_proxy, 0, sizeof(addr_proxy));
    addr_proxy.sin6_family = AF_INET6;
    addr_proxy.sin6_port = htons(PROXY_PORT);
    addr_proxy.sin6_addr = in6addr_any;
    
    if (bind(sock_proxy, (struct sockaddr *)&addr_proxy, sizeof(addr_proxy)) < 0) {
        LOG_ERR("Failed to bind proxy socket");
        close(sock_proxy);
        return;
    }
    
    /* Socket para comunicar com sensor node */
    sock_client = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_client < 0) {
        LOG_ERR("Failed to create client socket");
        close(sock_proxy);
        return;
    }
    
    LOG_INF("CoAP Proxy ready - waiting for requests...");
    LOG_INF("Forward requests to sensor nodes automatically");
    
    while (1) {
        struct sockaddr_in6 client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int received, sent;
        
        /* Recebe requisição do PC */
        received = recvfrom(sock_proxy, buffer, sizeof(buffer), 0,
                           (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (received < 0) {
            continue;
        }
        
        LOG_INF("Received %d bytes from PC, forwarding to sensor...", received);
        
        /* 
         * Aqui você precisaria descobrir o endereço IPv6 do sensor node
         * Por enquanto, vamos usar multicast ou broadcast Thread
         * Em produção, use mDNS/service discovery
         */
        struct sockaddr_in6 sensor_addr;
        memset(&sensor_addr, 0, sizeof(sensor_addr));
        sensor_addr.sin6_family = AF_INET6;
        sensor_addr.sin6_port = htons(SENSOR_PORT);
        
        /* 
         * TODO: Descobrir endereço do sensor automaticamente
         * Por enquanto, use o endereço link-local ou mesh-local
         * Exemplo: ff03::1 (all nodes multicast)
         */
        inet_pton(AF_INET6, "ff03::1", &sensor_addr.sin6_addr);
        
        /* Encaminha para sensor node */
        sent = sendto(sock_client, buffer, received, 0,
                     (struct sockaddr *)&sensor_addr, sizeof(sensor_addr));
        
        if (sent < 0) {
            LOG_ERR("Failed to forward to sensor");
            continue;
        }
        
        /* Aguarda resposta do sensor */
        struct sockaddr_in6 sensor_response_addr;
        socklen_t sensor_response_len = sizeof(sensor_response_addr);
        
        received = recvfrom(sock_client, buffer, sizeof(buffer), 0,
                           (struct sockaddr *)&sensor_response_addr,
                           &sensor_response_len);
        
        if (received > 0) {
            LOG_INF("Got %d bytes from sensor, sending back to PC", received);
            
            /* Envia resposta de volta ao PC */
            sendto(sock_proxy, buffer, received, 0,
                  (struct sockaddr *)&client_addr, client_addr_len);
        }
    }
}

/* Thread para periodic discovery e logging */
void network_monitor_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    struct openthread_context *ot_context = openthread_get_default_context();
    
    while (1) {
        k_sleep(K_SECONDS(30));
        
        if (ot_context) {
            otInstance *instance = ot_context->instance;
            
            LOG_INF("=== Thread Network Status ===");
            LOG_INF("Role: %s", 
                    otThreadGetDeviceRole(instance) == OT_DEVICE_ROLE_LEADER ? "Leader" :
                    otThreadGetDeviceRole(instance) == OT_DEVICE_ROLE_ROUTER ? "Router" :
                    otThreadGetDeviceRole(instance) == OT_DEVICE_ROLE_CHILD ? "Child" : "Detached");
            
            /* List neighbor devices */
            otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
            otNeighborInfo neighborInfo;
            int count = 0;
            
            while (otThreadGetNextNeighborInfo(instance, &iterator, &neighborInfo) == OT_ERROR_NONE) {
                count++;
                LOG_INF("Neighbor %d: RLOC16=0x%04x, LinkQuality=%d",
                        count, neighborInfo.mRloc16, neighborInfo.mLinkQualityIn);
            }
            
            if (count == 0) {
                LOG_WRN("No neighbors found - sensor node may not be connected");
            }
        }
    }
}

int main(void)
{
    LOG_INF("SAMR21 Thread Border Router starting...");
    
    /* Initialize USB CDC */
    if (usb_enable(NULL) != 0) {
        LOG_ERR("Failed to enable USB");
    } else {
        LOG_INF("USB CDC enabled - connect to serial monitor");
    }
    
    /* Initialize OpenThread */
    struct openthread_context *ot_context = openthread_get_default_context();
    if (!ot_context) {
        LOG_ERR("Failed to get OpenThread context");
        return -1;
    }
    
    LOG_INF("OpenThread initialized as Border Router (FTD)");
    LOG_INF("Network: SAMR21-Network, Channel: 15, PAN ID: 0xABCD");
    LOG_INF("");
    LOG_INF("This device will:");
    LOG_INF("  1. Form/join Thread network");
    LOG_INF("  2. Act as proxy between PC and sensor nodes");
    LOG_INF("  3. Forward CoAP requests automatically");
    LOG_INF("");
    LOG_INF("Connect sensor node now...");
    
    /* Start proxy thread */
    k_thread_create(&proxy_thread_data, proxy_thread_stack,
                   K_THREAD_STACK_SIZEOF(proxy_thread_stack),
                   coap_proxy_thread, NULL, NULL, NULL,
                   7, 0, K_NO_WAIT);
    
    /* Start monitor thread */
    k_thread_create(&monitor_thread_data, monitor_thread_stack,
                   K_THREAD_STACK_SIZEOF(monitor_thread_stack),
                   network_monitor_thread, NULL, NULL, NULL,
                   8, 0, K_NO_WAIT);
    
    LOG_INF("Border Router operational");
    
    return 0;
}

/* Thread stacks */
K_THREAD_STACK_DEFINE(proxy_thread_stack, 2048);
K_THREAD_STACK_DEFINE(monitor_thread_stack, 1024);

static struct k_thread proxy_thread_data;
static struct k_thread monitor_thread_data;