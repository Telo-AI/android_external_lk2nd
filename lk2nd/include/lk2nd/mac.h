/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LK2ND_MAC_H
#define LK2ND_MAC_H

#include <stdint.h>

#define MAC_ADDR_SIZE	6

void lk2nd_mac_set_wifi(const uint8_t mac[MAC_ADDR_SIZE]);

#endif /* LK2ND_MAC_H */
