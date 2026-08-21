/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Relay fixture — 8-channel DMX-controlled relay actuator.
 *
 * DMX mapping (relative to configured start address):
 *   addr + 0  →  relay 1
 *   addr + 1  →  relay 2
 *   …
 *   addr + 7  →  relay 8
 *
 * Threshold:  value ≥ 128 → ON, value < 128 → OFF.
 */

#include "relay_fixture.h"

#include <dmx_z.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(relay_fix, LOG_LEVEL_INF);

#define RELAY_COUNT   8
#define RELAY_ON_VAL  128

#define TCA_NODE DT_ALIAS(tca)

static const struct device *tca_dev;
static uint16_t g_dmx_start_addr = CONFIG_DMX_FIXTURE_START_ADDR;
static uint8_t  relay_state;

static void on_dmx_channel(uint8_t channel, uint8_t value)
{
	if (channel >= RELAY_COUNT) {
		return;
	}

	uint8_t bit = BIT(channel);
	uint8_t prev = relay_state;
	bool on = (value >= RELAY_ON_VAL);

	if (on) {
		relay_state |= bit;
	} else {
		relay_state &= ~bit;
	}

	if (relay_state != prev) {
		printk("[DMX] relay %u -> %s (ch %u = %u)\n",
		       channel + 1, on ? "ON" : "OFF",
		       g_dmx_start_addr + channel, value);
		for (int i = 0; i < RELAY_COUNT; i++) {
			int ret = gpio_pin_set_raw(tca_dev, i,
						   (relay_state >> i) & 1);
			if (ret) {
				LOG_WRN("relay %d set failed: %d", i, ret);
			}
		}
	}
}

static void relay_thread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);

	while (true) {
		if (dmx_rx_frame_ready()) {
			const struct dmx_fixture_cfg cfg = {
				.start_addr   = g_dmx_start_addr,
				.num_channels = RELAY_COUNT,
				.on_channel   = on_dmx_channel,
			};
			dmx_fixture_process(&cfg);
		}
		k_msleep(1);
	}
}

#define RELAY_THREAD_STACK_SIZE 4096
#define RELAY_THREAD_PRIO       5

K_THREAD_STACK_DEFINE(relay_stack, RELAY_THREAD_STACK_SIZE);
static struct k_thread relay_thread;

int relay_fixture_init(uint16_t dmx_start_addr)
{
	if (dmx_start_addr >= 1 &&
	    dmx_start_addr + RELAY_COUNT - 1 <= DMX_UNIVERSE_SIZE) {
		g_dmx_start_addr = dmx_start_addr;
	}

	tca_dev = DEVICE_DT_GET(TCA_NODE);
	if (!device_is_ready(tca_dev)) {
		LOG_ERR("PCA9538 (tca) not ready");
		return -ENODEV;
	}

	for (int i = 0; i < RELAY_COUNT; i++) {
		int ret = gpio_pin_configure(tca_dev, i,
					     GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("relay %d config failed: %d", i, ret);
			return ret;
		}
	}
	relay_state = 0;

	k_thread_create(&relay_thread, relay_stack,
			K_THREAD_STACK_SIZEOF(relay_stack),
			relay_thread_fn, NULL, NULL, NULL,
			RELAY_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&relay_thread, "relay");

	LOG_INF("relay fixture: %u channels at DMX addr %u",
		RELAY_COUNT, g_dmx_start_addr);
	return 0;
}
