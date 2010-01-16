/* $(CROSS_COMPILE)gcc -Wall -O3 -g -lusb-1.0 -o cleware cleware.c */
/**
 * cleware.c - Cleware USB-Controlled Power Switch
 *
 * Copyright (C) 2010 Felipe Balbi <felipe.balbi@nokia.com>
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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <libusb-1.0/libusb.h>

#define CLEWARE_VENDOR_ID	0x0d50
#define CLEWARE_USB_SWITCH	0x0008

enum cleware_leds {
	LED0 = 0x00,
	LED1 = 0x01,
	LED2 = 0x02,
	LED3 = 0x03,
};

enum cleware_switches {
	SWITCH0 = 0x10,
	SWITCH1 = 0x11,
	SWITCH2 = 0x12,
	SWITCH3 = 0x13,
	SWITCH4 = 0x14,
	SWITCH5 = 0x15,
	SWITCH6 = 0x16,
	SWITCH7 = 0x17,
	SWITCH8 = 0x18,
	SWITCH9 = 0x19,
	SWITCH10 = 0x1a,
	SWITCH11 = 0x1b,
	SWITCH12 = 0x1c,
	SWITCH13 = 0x1d,
	SWITCH14 = 0x1e,
	SWITCH15 = 0x1f
};

struct usb_device_id {
	unsigned		idVendor;
	unsigned		idProduct;
};

#define USB_DEVICE(v, p)		\
{					\
	.idVendor		= v,	\
	.idProduct		= p,	\
}

static struct usb_device_id cleware_id[] = {
	USB_DEVICE(CLEWARE_VENDOR_ID, CLEWARE_USB_SWITCH),
};

static int debug;

#define DBG(fmt, args...)			\
	if (debug)				\
		fprintf(stdout, fmt, ## args)

#define TIMEOUT			1000	/* ms */
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

static int match_device_id(libusb_device *udev)
{
	struct libusb_device_descriptor	desc;
	int				match = 0;
	int				ret;
	int				i;

	ret = libusb_get_device_descriptor(udev, &desc);
	if (ret < 0) {
		DBG("%s: failed to get device descriptor\n", __func__);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(cleware_id); i++) {
		if ((desc.idVendor == cleware_id[i].idVendor) &&
				(desc.idProduct == cleware_id[i].idProduct)) {
			DBG("%s: matched device %04x:%04x\n",__func__,
					desc.idVendor, desc.idProduct);
			match = 1;
		}
	}

	return match;
}

static int match_device_serial_number(libusb_device *udev, unsigned iSerial)
{
	struct libusb_device_descriptor	desc;

	libusb_device_handle		*tmp;

	unsigned char			serial;
	int				ret;

	ret = libusb_get_device_descriptor(udev, &desc);
	if (ret < 0) {
		DBG("%s: failed to get device descriptor\n", __func__);
		goto out0;
	}

	ret = libusb_open(udev, &tmp);
	if (ret < 0 || !tmp) {
		DBG("%s: couldn't open device\n", __func__);
		goto out0;
	}

	if (libusb_kernel_driver_active(tmp, 0)) {
		ret = libusb_detach_kernel_driver(tmp, 0);
		if (ret < 0) {
			DBG("%s: couldn't detach kernel driver\n", __func__);
			goto out1;
		}
	}

	ret = libusb_get_string_descriptor(tmp, desc.iSerialNumber,
			0x0409, &serial, sizeof(serial));

	if (ret < 0) {
		DBG("%s: failed to get string descriptor\n", __func__);
		goto out1;
	}

	if (serial != iSerial) {
		DBG("%s: not the serial number we want\n", __func__);
		goto out2;
	}

out2:
	libusb_attach_kernel_driver(tmp, 0);

out1:
	libusb_close(tmp);

out0:
	return ret;
}

static void print_device_attributes(libusb_device *udev)
{
	struct libusb_device_descriptor	desc;

	libusb_device_handle		*tmp;

	int				ret;

	unsigned char			product[256] = { };

	ret = libusb_get_device_descriptor(udev, &desc);
	if (ret < 0) {
		DBG("%s: failed to get device descriptor\n", __func__);
		return;
	}

	ret = libusb_open(udev, &tmp);
	if (ret < 0 || !tmp) {
		DBG("%s: couldn't open device\n", __func__);
		return;
	}

	if (libusb_kernel_driver_active(tmp, 0)) {
		ret = libusb_detach_kernel_driver(tmp, 0);
		if (ret < 0) {
			DBG("%s: couldn't detach kernel driver\n", __func__);
			libusb_close(tmp);
			return;
		}
	}

	if (desc.iProduct) {
		ret = libusb_get_string_descriptor_ascii(tmp, desc.iProduct, product,
				sizeof(product));

		if (ret < 0) {
			DBG("%s: failed to get product name\n", __func__);
			libusb_close(tmp);
			return;
		}
	}

	fprintf(stdout, "%04x:%04x\t%s\n", desc.idVendor, desc.idProduct,
			product);
}

static void list_devices(libusb_device **list, ssize_t count)
{
	int				i;

	for (i = 0; i < count; i++) {
		libusb_device *udev = list[i];

		if (match_device_id(udev))
			print_device_attributes(udev);
	}
}

static libusb_device_handle *find_and_open_device(libusb_device **list,
		ssize_t count, unsigned iSerial)
{
	libusb_device_handle		*udevh = NULL;
	libusb_device			*found = NULL;
	int				ret;
	int				i;

	for (i = 0; i < count; i++) {
		libusb_device *udev = list[i];

		ret = match_device_id(udev);
		if (ret <= 0) {
			DBG("%s: couldn't match device id\n", __func__);
			goto out;
		}

		if (iSerial) {
			ret = match_device_serial_number(udev, iSerial);
			if (ret < 0) {
				DBG("%s: couldn't match serial number\n", __func__);
				found = NULL;
				break;
			}
		}

		found = udev;
		break;
	}

	ret = libusb_open(found, &udevh);
	if (ret < 0 || !found) {
		DBG("%s: couldn't open device\n", __func__);
		goto out;
	}

out:
	return udevh;
}

static int find_and_claim_interface(libusb_device_handle *udevh)
{
	int			ret;

	if (libusb_kernel_driver_active(udevh, 0)) {
		ret = libusb_detach_kernel_driver(udevh, 0);
		if (ret < 0) {
			DBG("%s: couldn't detach kernel driver\n", __func__);
			goto out0;
		}
	}

	ret = libusb_set_configuration(udevh, 1);
	if (ret < 0) {
		DBG("%s: couldn't set configuration 1\n", __func__);
		goto out0;
	}

	ret = libusb_claim_interface(udevh, 0);
	if (ret < 0) {
		DBG("%s: couldn't claim interface 0\n", __func__);
		goto out0;
	}

	ret = libusb_set_interface_alt_setting(udevh, 0, 0);
	if (ret < 0) {
		DBG("%s: couldn't set alternate setting 0\n", __func__);
		goto out1;
	}

	return 0;

out1:
	libusb_release_interface(udevh, 0);

out0:
	return ret;
}

static void release_interface(libusb_device_handle *udevh)
{
	libusb_release_interface(udevh, 0);
}

static int set_led(libusb_device_handle *udevh, unsigned led, unsigned on)
{
	unsigned char		data[3];

	data[0] = 0x00;
	data[1] = led;
	data[2] = on ? 0x00 : 0x0f;

	return libusb_control_transfer(udevh,
			0x21, 0x09, 0x200, 0x00, data, sizeof(data), TIMEOUT);
}

static int set_switch(libusb_device_handle *udevh, unsigned port, unsigned on)
{
	int			ret;
	unsigned char		data[3];

	/*
	 * the following sequence was sniffed from the example
	 * application provided by the manufacturer.
	 *
	 * it's known to work with the following device:
	 * http://www.cleware.de/produkte/p-usbswitch-E.html
	 */

	data[0] = 0x00;

	data[1] = port + 0x10;
	data[2] = on & 0x01;

	ret = libusb_control_transfer(udevh,
			0x21, 0x09, 0x200, 0x00, data, sizeof(data), TIMEOUT);
	if (ret < 0) {
		DBG("%s: couldn't turn %s device\n", __func__,
				on ? "on" : "off");
		goto out;
	}

out:
	return ret;
}

static int set_power(libusb_device_handle *udevh, unsigned port, unsigned on)
{
	int			ret;

	ret = set_switch(udevh, port, on);
	if (ret < 0) {
		DBG("%s: failed to turn %s switch %d\n", __func__,
				on ? "on" : "off", port);
		goto out;
	}

	if (port == 0) {
		ret = set_led(udevh, LED0, on);
		if (ret < 0) {
			DBG("%s: couldn't turn %s device\n", __func__,
					on ? "on" : "off");
			goto out;
		}

		ret = set_led(udevh, LED1, !on);
		if (ret < 0) {
			DBG("%s: couldn't turn %s device\n", __func__,
					on ? "on" : "off");
			goto out;
		}
	}

	return 0;

out:
	return ret;
}

static void usage(char *name)
{
	fprintf(stdout, "usage: %s [-0 | -1] [-s SerialNumber] [-d] [-h]\n\
			-0, --on		Switch it on\n\
			-1, --off		Switch it off\n\
			-s, --serial-number	Device's serial number\n\
			-p, --port		0-based port number\n\
			-d, --debug		Enable debugging\n\
			-h, --help		Print this\n", name);
}

#define OPTION(n, h, v)		\
{				\
	.name		= #n,	\
	.has_arg	= h,	\
	.val		= v,	\
}

static struct option cleware_opts[] = {
	OPTION("on",		0,	'0'),
	OPTION("off",		0,	'1'),
	OPTION("serial-number",	1,	's'),
	OPTION("port",		1,	'p'),
	OPTION("list",		0,	'l'),
	OPTION("debug",		0,	'd'),
	OPTION("help",		0,	'h'),
	{  }	/* Terminating entry */
};

int main(int argc, char *argv[])
{
	libusb_context		*context;
	libusb_device_handle	*udevh;
	libusb_device		**list;

	unsigned		port = 0;
	unsigned		iSerial = 0;
	unsigned		list_devs = 0;

	ssize_t			count;

	int			ret = 0;
	int			on = 0;

	char			*serial = NULL;

	while (ARRAY_SIZE(cleware_opts)) {
		int		optidx = 0;
		int		opt;

		opt = getopt_long(argc, argv, "l01p:s:dh", cleware_opts, &optidx);
		if (opt < 0)
			break;

		switch (opt) {
		case 'l':
			list_devs = 1;
			break;
		case '0':
			on = 0;
			break;
		case '1':
			on = 1;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 's':
			iSerial = strtoul(optarg, &serial, 16);
			break;
		case 'd':
			debug = 1;
			break;
		case 'h': /* FALLTHROUGH */
		default:
			usage(argv[0]);
			return 0;
		}
	}

	/* initialize libusb */
	libusb_init(&context);

	/* get rid of debug messages */
	libusb_set_debug(context, 0);

	count = libusb_get_device_list(context, &list);
	if (count < 0) {
		DBG("%s: couldn't get device list\n", __func__);
		ret = count;
		goto out1;
	}

	if (list_devs) {
		list_devices(list, count);
		goto out1;
	}

	udevh = find_and_open_device(list, count, iSerial);
	if (!udevh) {
		DBG("%s: couldn't find a suitable device\n", __func__);
		goto out2;
	}

	ret = find_and_claim_interface(udevh);
	if (ret < 0) {
		DBG("%s: couldn't claim interface\n", __func__);
		goto out2;
	}

	ret = set_power(udevh, port, on);
	if (ret < 0) {
		DBG("%s: couldn't switch power %s\n", __func__,
				on ? "on" : "off");
		goto out3;
	}

out3:
	release_interface(udevh);

out2:
	libusb_close(udevh);
	libusb_free_device_list(list, 1);

out1:
	libusb_exit(context);

	return ret;
}

