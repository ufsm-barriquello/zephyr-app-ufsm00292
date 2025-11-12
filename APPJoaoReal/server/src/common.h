#include "net_sample_common.h"

#define MY_PORT 4242
#define STACK_SIZE 1024

#define THREAD_PRIORITY K_PRIO_PREEMPT(8)

#define RECV_BUFFER_SIZE 1280

#define APP_BMEM
#define APP_DMEM


struct sensor_data_packet {
    float temperatura;
} __packed;

struct data {
	const char *proto;

	struct {
		int sock;
		char recv_buffer[RECV_BUFFER_SIZE];
	} udp;
};

struct configs {
	struct data ipv6;
};

extern struct configs conf;

void start_udp(void);
void stop_udp(void);

void quit(void);

