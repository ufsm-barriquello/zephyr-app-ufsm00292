#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>
#include <zephyr/posix/sys/eventfd.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include "common.h"

LOG_MODULE_REGISTER(net_echo_client_sample, LOG_LEVEL_DBG);

#define APP_BANNER "Run Client"

#define INVALID_SOCK (-1)

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | \
		    NET_EVENT_L4_DISCONNECTED)
#define IPV6_EVENT_MASK (NET_EVENT_IPV6_ADDR_ADD | \
			 NET_EVENT_IPV6_ADDR_DEPRECATED)


APP_DMEM struct configs conf = {
	.ipv6 = {
		.proto = "IPv6",
		.udp.sock = INVALID_SOCK,
	},
};

static APP_BMEM struct pollfd fds[2];
static APP_BMEM int nfds;

static APP_BMEM bool connected;
static APP_BMEM bool need_restart;

K_SEM_DEFINE(run_app, 0, 1);

static struct net_mgmt_event_callback mgmt_cb;
static struct net_mgmt_event_callback ipv6_mgmt_cb;

static void prepare_fds(void)
{
	nfds = 0;

	fds[nfds].fd = eventfd(0, 0);
	fds[nfds].events = POLLIN;
	nfds++;

	if (conf.ipv6.udp.sock >= 0) {
		fds[nfds].fd = conf.ipv6.udp.sock;
		fds[nfds].events = POLLIN;
		nfds++;
	}
}

static void wait(void)
{
	int ret;

	ret = poll(fds, nfds, -1);
	if (ret < 0) {
		static bool once;

		if (!once) {
			once = true;
			LOG_ERR("Error in poll:%d", errno);
		}

		return;
	}

	if (ret > 0 && fds[0].revents) {
		eventfd_t value;

		eventfd_read(fds[0].fd, &value);
		LOG_DBG("Received restart event.");
		return;
	}
}

static int start_udp_and_tcp(void)
{
	int ret;

	LOG_INF("Starting...");

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		ret = start_udp();
		if (ret < 0) {
			return ret;
		}
	}

	prepare_fds();

	return 0;
}

static int run_udp_and_tcp(void)
{
	wait();

	return 0;
}

static void stop_udp_and_tcp(void)
{
	LOG_INF("Stopping...");

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		stop_udp();
	}
}

static int check_our_ipv6_sockets(int sock,
				  struct in6_addr *deprecated_addr)
{
	struct sockaddr_in6 addr = { 0 };
	socklen_t addrlen = sizeof(addr);
	int ret;

	if (sock < 0) {
		return -EINVAL;
	}

	ret = getsockname(sock, (struct sockaddr *)&addr, &addrlen);
	if (ret != 0) {
		return -errno;
	}

	if (!net_ipv6_addr_cmp(deprecated_addr, &addr.sin6_addr)) {
		return -ENOENT;
	}

	need_restart = true;

	return 0;
}

static void ipv6_event_handler(struct net_mgmt_event_callback *cb,
			       uint64_t mgmt_event, struct net_if *iface)
{
	static char addr_str[INET6_ADDRSTRLEN];

	if (!IS_ENABLED(CONFIG_NET_IPV6_PE)) {
		return;
	}

	if ((mgmt_event & IPV6_EVENT_MASK) != mgmt_event) {
		return;
	}

	if (cb->info == NULL ||
	    cb->info_length != sizeof(struct in6_addr)) {
		return;
	}

	if (mgmt_event == NET_EVENT_IPV6_ADDR_ADD) {
		struct net_if_addr *ifaddr;
		struct in6_addr added_addr;

		memcpy(&added_addr, cb->info, sizeof(struct in6_addr));

		ifaddr = net_if_ipv6_addr_lookup(&added_addr, &iface);
		if (ifaddr == NULL) {
			return;
		}

		if (ifaddr->is_temporary) {
			static bool once;

			LOG_INF("Temporary IPv6 address %s added",
				inet_ntop(AF_INET6, &added_addr, addr_str,
					  sizeof(addr_str) - 1));

			if (!once) {
				k_sem_give(&run_app);
				once = true;
			}
		}
	}

	if (mgmt_event == NET_EVENT_IPV6_ADDR_DEPRECATED) {
		struct in6_addr deprecated_addr;

		memcpy(&deprecated_addr, cb->info, sizeof(struct in6_addr));

		LOG_INF("IPv6 address %s deprecated",
			inet_ntop(AF_INET6, &deprecated_addr, addr_str,
				  sizeof(addr_str) - 1));

		(void)check_our_ipv6_sockets(conf.ipv6.udp.sock,
					     &deprecated_addr);

		if (need_restart) {
			eventfd_write(fds[0].fd, 1);
		}

		return;
	}
}

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint64_t mgmt_event, struct net_if *iface)
{
	if ((mgmt_event & EVENT_MASK) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");

		connected = true;

		if (!IS_ENABLED(CONFIG_NET_IPV6_PE)) {
			k_sem_give(&run_app);
		}

		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("Network disconnected");

		connected = false;
		k_sem_reset(&run_app);

		return;
	}
}

static void init_app(void)
{
	LOG_INF(APP_BANNER);

	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		net_mgmt_init_event_callback(&mgmt_cb,
					     event_handler, EVENT_MASK);
		net_mgmt_add_event_callback(&mgmt_cb);

		conn_mgr_mon_resend_status();
	}

	net_mgmt_init_event_callback(&ipv6_mgmt_cb,
				     ipv6_event_handler, IPV6_EVENT_MASK);
	net_mgmt_add_event_callback(&ipv6_mgmt_cb);

	init_udp();
}

static void start_client(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int ret;

	while (true) {
		k_sem_take(&run_app, K_FOREVER);

		if (IS_ENABLED(CONFIG_NET_IPV6_PE)) {
			k_sleep(K_SECONDS(1));
		}

		do {
			if (need_restart) {
				stop_udp_and_tcp();
				need_restart = false;
			}

			ret = start_udp_and_tcp();

			while (connected && (ret == 0)) {
				ret = run_udp_and_tcp();

				if (need_restart) {
					break;
				}
			}
		} while (need_restart);

		stop_udp_and_tcp();
	}
}

int main(void)
{
	init_app();

	if (!IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		connected = true;
		k_sem_give(&run_app);
	}

	k_thread_priority_set(k_current_get(), THREAD_PRIORITY);

	start_client(NULL, NULL, NULL);

	return 0;
}
