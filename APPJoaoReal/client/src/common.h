#include <zephyr/kernel.h>
#include "net_sample_common.h"


#define MY_PORT 0

#define PEER_PORT 4242

#define PRINT_PROGRESS false

#define APP_BMEM
#define APP_DMEM


#if defined(CONFIG_NET_TC_THREAD_PREEMPTIVE)
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#else
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#endif

#define UDP_STACK_SIZE 1536

struct sensor_data_packet {
    float temperatura;
} __packed;

struct udp_control {
	struct k_poll_signal tx_signal;
	struct k_timer tx_timer;
};

struct sample_data {
	const char *proto;

	struct {
		int sock;
		uint32_t expecting;
		struct udp_control *ctrl;
	} udp;
};

struct configs {
	struct sample_data ipv6;
};

#if !defined(CONFIG_NET_CONFIG_PEER_IPV4_ADDR)
#define CONFIG_NET_CONFIG_PEER_IPV4_ADDR ""
#endif

#if !defined(CONFIG_NET_CONFIG_PEER_IPV6_ADDR)
#define CONFIG_NET_CONFIG_PEER_IPV6_ADDR ""
#endif

extern struct configs conf;

void init_udp(void);
int start_udp(void);
void stop_udp(void);

