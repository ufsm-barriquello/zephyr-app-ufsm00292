#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>
#include "common.h"
#include "meu_sensor.h"

LOG_MODULE_DECLARE(net_echo_client_sample, LOG_LEVEL_DBG);

static K_THREAD_STACK_DEFINE(udp_tx_thread_stack, UDP_STACK_SIZE);
static struct k_thread udp_tx_thread;

static struct udp_control udp6_ctrl;
static struct k_poll_signal udp_kill;

static int send_udp_data(struct sample_data *data);
static void wait_transmit(struct k_timer *timer);

static void process_udp_tx(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &udp_kill),

#if defined(CONFIG_NET_IPV6)
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &udp6_ctrl.tx_signal),
#endif
	};

	while (true) {
		k_poll(events, ARRAY_SIZE(events), K_FOREVER);

		for (int i = 0; i < ARRAY_SIZE(events); i++) {
			unsigned int signaled;
			int result;

			k_poll_signal_check(events[i].signal, &signaled, &result);
			if (signaled == 0) {
				continue;
			}

			k_poll_signal_reset(events[i].signal);
			events[i].state = K_POLL_STATE_NOT_READY;

			if (events[i].signal == &udp_kill) {
				return;
			} else if (events[i].signal == &udp6_ctrl.tx_signal) {
				send_udp_data(&conf.ipv6);
			}
		}
	}
}

static void udp_control_init(struct udp_control *ctrl)
{
	k_timer_init(&ctrl->tx_timer, wait_transmit, NULL);
	k_poll_signal_init(&ctrl->tx_signal);
}

void init_udp(void)

{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		udp_control_init(&udp6_ctrl);
		conf.ipv6.udp.ctrl = &udp6_ctrl;
	}

	k_poll_signal_init(&udp_kill);
}

static int send_udp_data(struct sample_data *data)
{
    int ret;

    struct sensor_data_packet dados;

    dados.temperatura = get_sensor_temp(); 

    data->udp.expecting = sizeof(dados);

    ret = send(data->udp.sock, &dados, sizeof(dados), 0);

    LOG_INF("Enviando: Temp %.1f C (%d bytes)",
            (double)dados.temperatura, sizeof(dados));

    return ret < 0 ? -EIO : 0;
}


static void wait_transmit(struct k_timer *timer)
{
    struct udp_control *ctrl = CONTAINER_OF(timer, struct udp_control, tx_timer);

    k_poll_signal_raise(&ctrl->tx_signal, 0);

    k_timer_start(&ctrl->tx_timer, K_SECONDS(5), K_NO_WAIT);
}

static int start_udp_proto(struct sample_data *data, sa_family_t family,
			   struct sockaddr *addr, socklen_t addrlen)
{
	int optval;
	int ret;

	data->udp.sock = socket(family, SOCK_DGRAM, IPPROTO_UDP);

	if (data->udp.sock < 0) {
		LOG_ERR("Failed to create UDP socket (%s): %d", data->proto,
			errno);
		return -errno;
	}

	if (family == AF_INET6) {
		optval = IPV6_PREFER_SRC_TMP;
		(void)setsockopt(data->udp.sock, IPPROTO_IPV6,
				 IPV6_ADDR_PREFERENCES,
				 &optval, sizeof(optval));
	}

	ret = connect(data->udp.sock, addr, addrlen);
	if (ret < 0) {
		LOG_ERR("Cannot connect to UDP remote (%s): %d", data->proto,
			errno);
		ret = -errno;
	}

	return ret;
}

int start_udp(void)
{
	int ret = 0;
	struct sockaddr_in6 addr6;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(PEER_PORT);
		inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
			  &addr6.sin6_addr);

		ret = start_udp_proto(&conf.ipv6, AF_INET6,
				      (struct sockaddr *)&addr6,
				      sizeof(addr6));
		if (ret < 0) {
			return ret;
		}
	}

	k_thread_create(&udp_tx_thread, udp_tx_thread_stack,
			K_THREAD_STACK_SIZEOF(udp_tx_thread_stack),
			process_udp_tx,
			NULL, NULL, NULL, THREAD_PRIORITY,
			0, K_NO_WAIT);

	k_thread_name_set(&udp_tx_thread, "udp_tx");


	if (IS_ENABLED(CONFIG_NET_IPV6)) {
    k_timer_start(&conf.ipv6.udp.ctrl->tx_timer, K_SECONDS(5), K_NO_WAIT);
	}

	return ret;
}

void stop_udp(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		k_timer_stop(&udp6_ctrl.tx_timer);

		if (conf.ipv6.udp.sock >= 0) {
			(void)close(conf.ipv6.udp.sock);
		}
	}

	k_poll_signal_raise(&udp_kill, 0);
}
