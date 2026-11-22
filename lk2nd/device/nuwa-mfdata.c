// SPDX-License-Identifier: BSD-3-Clause

#include <arch/defines.h>
#include <ctype.h>
#include <debug.h>
#include <libfdt.h>
#include <mmc_wrapper.h>
#include <partition_parser.h>
#include <stdlib.h>
#include <string.h>

#include <lk2nd/mac.h>

#include "device.h"

/*
 * Nuwa's factory record: "DfM_", then 84-byte entries of a 16-byte name and its
 * value. It also holds the serial number and a device key, so only the Wi-Fi
 * MAC is taken and the buffer is wiped.
 */
#define MFDATA_SIZE		4096
#define MFDATA_FIRST_ENTRY	12
#define MFDATA_ENTRY_SIZE	84
#define MFDATA_NAME_SIZE	16

static bool parse_mac(const char *hex, uint8_t mac[MAC_ADDR_SIZE])
{
	char *end;
	int i;

	for (i = 0; i < 2 * MAC_ADDR_SIZE; i++)
		if (!isxdigit((unsigned char)hex[i]))
			return false;

	for (i = 0; i < MAC_ADDR_SIZE; i++) {
		char byte[3] = { hex[2 * i], hex[2 * i + 1], 0 };

		mac[i] = strtoul(byte, &end, 16);
	}

	/* A multicast or all-zero entry is not an address to use */
	return !(mac[0] & 1) &&
	       (mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]);
}

static int lk2nd_nuwa_mfdata_init(const void *dtb, int node)
{
	const char *name = fdt_getprop(dtb, node, "partition", NULL);
	struct partition_info part;
	uint8_t mac[MAC_ADDR_SIZE];
	char *buf;
	int off;

	if (!name)
		return 0;

	part = partition_get_info(name);
	if (!part.offset || part.size < MFDATA_SIZE)
		return 0;

	buf = memalign(CACHE_LINE, MFDATA_SIZE);
	if (!buf)
		return 0;

	if (!mmc_read(part.offset, (uint32_t *)buf, MFDATA_SIZE) &&
	    !memcmp(buf, "DfM_", 4)) {
		for (off = MFDATA_FIRST_ENTRY;
		     off + MFDATA_ENTRY_SIZE <= MFDATA_SIZE;
		     off += MFDATA_ENTRY_SIZE) {
			if (strncmp(buf + off, "wifi_mac", MFDATA_NAME_SIZE))
				continue;
			if (parse_mac(buf + off + MFDATA_NAME_SIZE, mac))
				lk2nd_mac_set_wifi(mac);
			break;
		}
	}

	memset(buf, 0, MFDATA_SIZE);
	free(buf);
	return 0;
}

LK2ND_DEVICE_INIT("nuwa,mfdata", lk2nd_nuwa_mfdata_init);
