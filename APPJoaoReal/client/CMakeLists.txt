cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(client)

target_sources(app PRIVATE src/echo-client.c)
target_sources_ifdef(CONFIG_NET_UDP app PRIVATE src/udp.c src/meu_sensor.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
