/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Relay fixture — 8-channel DMX-controlled relay actuator via PCA9538.
 *
 * Each DMX slot (at start_addr + N) maps to relay N:
 *   value ≥ 128  →  relay ON
 *   value <  128  →  relay OFF
 */

#ifndef RELAY_FIXTURE_H_
#define RELAY_FIXTURE_H_

#include <stdint.h>

int relay_fixture_init(uint16_t dmx_start_addr);

#endif /* RELAY_FIXTURE_H_ */
