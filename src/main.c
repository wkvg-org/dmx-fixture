/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * dmx-fixture — DMX512 relay actuator for ESP32-S3 8DI 8RO.
 *
 * Receives DMX512 slots from the RS485 bus and controls 8 relays
 * via a PCA9538 GPIO expander.  The DMX start address is configured
 * through CONFIG_DMX_FIXTURE_START_ADDR (Kconfig / prj.conf).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <dmx_z.h>

#include "modules/relay/relay_fixture.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define USB_SERIAL_NODE DT_ALIAS(usb_serial)

int main(void)
{
	const struct device *serial = DEVICE_DT_GET(USB_SERIAL_NODE);

	if (!device_is_ready(serial)) {
		LOG_ERR("USB serial not ready");
		return 0;
	}
	LOG_INF("USB serial ready");

	/*
	 * Initialise the relay fixture first: it drives the PCA9538 over
	 * I²C.  The DMX RX polling thread is started afterwards so that its
	 * UART register access cannot interfere with the I²C transactions
	 * performed during init.
	 */
	int ret = relay_fixture_init(CONFIG_DMX_FIXTURE_START_ADDR);
	if (ret != 0) {
		LOG_ERR("relay_fixture_init failed: %d", ret);
		return 0;
	}

	ret = dmx_rx_init();
	if (ret != 0) {
		LOG_ERR("dmx_rx_init failed: %d", ret);
		return 0;
	}

	LOG_INF("dmx-fixture running");
	return 0;
}
