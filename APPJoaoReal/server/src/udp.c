#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>
#include <zephyr/net/socket.h>
#include "common.h"

LOG_MODULE_DECLARE(net_echo_server_sample, LOG_LEVEL_DBG);

static void process_udp6(void);

K_THREAD_DEFINE(udp6_thread_id, STACK_SIZE,
		process_udp6, NULL, NULL, NULL,
		THREAD_PRIORITY,
		0, -1);

static int start_udp_proto(struct data *data, struct sockaddr *bind_addr,
			   socklen_t bind_addrlen)
{
	int optval;
	int ret;

	data->udp.sock = socket(bind_addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);

	if (data->udp.sock < 0) {
		NET_ERR("Failed to create UDP socket (%s): %d", data->proto,
			errno);
		return -errno;
	}

	if (bind_addr->sa_family == AF_INET6) {
		optval = IPV6_PREFER_SRC_PUBLIC;
		(void)setsockopt(data->udp.sock, IPPROTO_IPV6,
				 IPV6_ADDR_PREFERENCES,
				 &optval, sizeof(optval));
		optval = 1;
		(void)setsockopt(data->udp.sock, IPPROTO_IPV6, IPV6_V6ONLY,
				 &optval, sizeof(optval));
	}

	ret = bind(data->udp.sock, bind_addr, bind_addrlen);
	if (ret < 0) {
		NET_ERR("Failed to bind UDP socket (%s): %d", data->proto,
			errno);
		ret = -errno;
	}

	return ret;
}


static void process_udp6(void)
{
	int ret;
	struct sockaddr_in6 addr6;

	(void)memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(MY_PORT);

	ret = start_udp_proto(&conf.ipv6, (struct sockaddr *)&addr6,
			      sizeof(addr6));
	if (ret < 0) {
		quit();
		return;
	}

	int received;
	struct sockaddr client_addr;
	socklen_t client_addr_len;

	NET_INFO("Waiting for UDP packets on port %d (%s)...",
		 MY_PORT, conf.ipv6.proto);

	while (true) {
    	client_addr_len = sizeof(client_addr);
    	received = recvfrom(conf.ipv6.udp.sock, conf.ipv6.udp.recv_buffer,
                sizeof(conf.ipv6.udp.recv_buffer), 0,
                &client_addr, &client_addr_len);

    	if (received < 0) {
        	NET_ERR("UDP (%s): Connection error %d", conf.ipv6.proto,
            errno);
        	quit();
			break;
		}
    
    	struct sensor_data_packet *dados = (struct sensor_data_packet *)conf.ipv6.udp.recv_buffer;

    	LOG_INF("Recebido: Temp %.1f C", (double)dados->temperatura);
	}
}

void start_udp(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {

		k_thread_name_set(udp6_thread_id, "udp6");
		k_thread_start(udp6_thread_id);
	}

}

void stop_udp(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		k_thread_abort(udp6_thread_id);
		if (conf.ipv6.udp.sock >= 0) {
			(void)close(conf.ipv6.udp.sock);
		}
	}
}
