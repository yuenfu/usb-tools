/* $(CROSS_COMPILE)gcc -Wall -O2 -g -lusb-1.0 -o control control.c */
/**
 * control.c - Build specified control messages and send to remote device
 *
 * Copyright (C) 2013 Felipe Balbi <balbi@ti.com>
 *
 * This file is part of the USB Verification Tools Project
 *
 * USB Tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public Liicense as published by
 * the Free Software Foundation, either version 3 of the license, or
 * (at your option) any later version.
 *
 * USB Tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with USB Tools. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <libusb.h>

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#define DEFAULT_TIMEOUT	2000	/* ms */

#define OPTION(n, h, v)			\
{					\
	.name		= n,		\
	.has_arg	= h,		\
	.val		= v,		\
}

static struct option control_opts[] = {
	OPTION("bmRequestType",	1, 't'),
	OPTION("bRequest",	1, 'r'),
	OPTION("wValue",	1, 'v'),
	OPTION("wIndex",	1, 'i'),
	OPTION("wLength",	1, 'l'),
	OPTION("count",		1, 'c'),
	OPTION("help",		0, 'h'),
	OPTION("device",	1, 'D'),
	{  }	/* Terminating entry */
};

static void hexdump(char *buf, unsigned size)
{
	int			i;

	if (!size)
		return;

	printf("dumping %d byte%c\n", size, size > 1 ? 's' : ' ');

	for (i = 0; i < size; i++) {
		if (i && ((i % 16) == 0))
			printf("\n");
		printf("%02x ", buf[i]);
	}
	printf("\n");
}

static int send_control_message(libusb_device_handle *udevh, uint8_t bmRequestType,
		uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength)

{
	unsigned char *buf = NULL;
	int ret;

	if (wLength) {
		buf = malloc(wLength);
		if (!buf) {
			fprintf(stderr, "Couldn't allocate buffer\n");
			return -1;
		}
	}

	ret = libusb_control_transfer(udevh, bmRequestType, bRequest, wValue,
			wIndex, buf, wLength, DEFAULT_TIMEOUT);
	if (ret < 0)
		fprintf(stderr, "control message failed --> %d\n", ret);
	else
		hexdump((char *) buf, ret);

	free(buf);

	return ret;
}

static void usage(char *cmd)
{
	fprintf(stdout, "%s -D VID:PID [options]\n\
		-t, --bmRequestType	request type field for setup packet\n\
		-r, --bRequest		request field for setup packet\n\
		-v, --wValue		value field for setup packet\n\
		-i, --wIndex		index field for setup packet\n\
		-l, --wLength		length field for setup packet\n\
		-c, --count		number of times to issue same request\n",
		cmd);
}

int main(int argc, char *argv[])
{
	libusb_context		*context;
	libusb_device_handle	*udevh;

	unsigned		count = 1;

	uint16_t		wLength = 0;
	uint16_t		wValue = 0;
	uint16_t		wIndex = 0;
	uint16_t		vid = 0;
	uint16_t		pid = 0;

	uint8_t			bmRequestType = 0;
	uint8_t			bRequest = 0;

	int			ret = 0;
	int			i;

	while (1) {
		int		optidx;
		int		opt;

		char		*token;

		opt = getopt_long(argc, argv, "D:c:t:r:v:i:l:h", control_opts, &optidx);
		if (opt == -1)
			break;

		switch (opt) {
		case 'D':
			token = strtok(optarg, ":");
			vid = strtoul(token, NULL, 16);
			token = strtok(NULL, ":");
			pid = strtoul(token, NULL, 16);
			break;
		case 'c':
			count = strtoul(optarg, NULL, 10);
			break;
		case 't':
			bmRequestType = strtoul(optarg, NULL, 16);
			break;
		case 'r':
			bRequest = strtoul(optarg, NULL, 16);
			break;
		case 'v':
			wValue = strtoul(optarg, NULL, 16);
			break;
		case 'i':
			wIndex = strtoul(optarg, NULL, 16);
			break;
		case 'l':
			wLength = strtoul(optarg, NULL, 10);
			break;
		case 'h': /* FALLTHROUGH */
		default:
			usage(argv[0]);
			exit(-1);
		}
	}

	libusb_init(&context);

	udevh = libusb_open_device_with_vid_pid(context, vid, pid);
	if (!udevh) {
		perror("couldn't open device");
		ret = -ENODEV;
		goto out;
	}

	libusb_set_auto_detach_kernel_driver(udevh, 1);

	if (bmRequestType == 0x01 && bRequest == 0x0b)
		libusb_claim_interface(udevh, wIndex);

	for (i = count; i; --i) {
		ret = send_control_message(udevh, bmRequestType, bRequest, wValue,
				wIndex, wLength);
		if (ret < 0)
			break;
	}

	libusb_close(udevh);

out:
	libusb_exit(context);

	return ret;
}
