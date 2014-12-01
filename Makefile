##
# Makefile - make support for usb-tools
#
# Copyright (C) 2010 Felipe Balbi <felipe.balbi@nokia.com>
#
# This file is part of the USB Verification Tools Project
#
# USB Tools is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public Liicense as published by
# the Free Software Foundation, either version 3 of the license, or
# (at your option) any later version.
#
# USB Tools is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with USB Tools. If not, see <http://www.gnu.org/licenses/>.
##

#CROSS_COMPILE = arm-linux-
CC = gcc
GENERIC_CFLAGS = -Wall -O3 -g -finline-functions -fno-strict-aliasing \
		 -D_GNU_SOURCE

LIBUSB_CFLAGS = $(shell pkg-config --cflags libusb-1.0)
LIBUSB_LIBS = $(shell pkg-config --libs libusb-1.0)

LIBPTHREAD_LIBS = -lpthread
LIBRT_LIBS = -lrt

CFLAGS = $(GENERIC_CFLAGS) $(LIBUSB_CFLAGS)

#
# Pretty print
#
V		= @
Q		= $(V:1=)
QUIET_CC	= $(Q:@=@echo    '     CC       '$@;)
QUIET_CLEAN	= $(Q:@=@echo    '     CLEAN    '$@;)
QUIET_AR	= $(Q:@=@echo    '     AR       '$@;)
QUIET_GEN	= $(Q:@=@echo    '     GEN      '$@;)
QUIET_LINK	= $(Q:@=@echo    '     LINK     '$@;)
QUIET_TAGS	= $(Q:@=@echo    '     TAGS     '$@;)

all: usb rt pthread generic cross

usb: companion-desc testmode cleware control device-reset

rt: msc

pthread: testusb

generic: serialc acmc switchbox

cross: seriald acmd

# Tools which need libusb-1.0 go here

companion-desc: companion-desc.o
	$(QUIET_LINK) $(CC) $< -o $@ $(LIBUSB_LIBS)

companion-desc.o: companion-desc.c
	$(QUIET_CC) $(CC) $< -o $@ -c $(CFLAGS)

testmode: testmode.o
	$(QUIET_LINK) $(CC) $< -o $@ $(LIBUSB_LIBS)

testmode.o: testmode.c
	$(QUIET_CC) $(CC) $< -o $@ -c $(CFLAGS)

device-reset: device-reset.o
	$(QUIET_LINK) $(CC) $< -o $@ $(LIBUSB_LIBS)

device-reset.o: device-reset.c
	$(QUIET_CC) $(CC) $< -o $@ -c $(CFLAGS)

cleware: cleware.o
	$(QUIET_LINK) $(CC) $< -o $@ $(LIBUSB_LIBS)

cleware.o: cleware.c
	$(QUIET_CC) $(CC) $< -o $@ -c $(CFLAGS)

control: control.o
	$(QUIET_LINK) $(CC) $< -o $@ $(LIBUSB_LIBS)

control.o: control.c
	$(QUIET_CC) $(CC) $< -o $@ -c $(CFLAGS)

serialc: serialc.o
	$(QUIET_LINK) $(CC) $< -o $@ $(LIBUSB_LIBS)

serialc.o: serialc.c
	$(QUIET_CC) $(CC) $< -o $@ -c $(CFLAGS)



# Tools which need cross-compilation go here

seriald: seriald.o
	$(QUIET_LINK) $(CROSS_COMPILE)$(CC) $< -o $@ $(GENERIC_CFLAGS)

seriald.o: seriald.c
	$(QUIET_CC) $(CROSS_COMPILE)$(CC) $< -o $@ -c $(GENERIC_CFLAGS)

acmd: acmd.o
	$(QUIET_LINK) $(CROSS_COMPILE)$(CC) $< -o $@ $(GENERIC_CFLAGS)

acmd.o: acmd.c
	$(QUIET_CC) $(CROSS_COMPILE)$(CC) $< -o $@ -c $(GENERIC_CFLAGS)



# Tools which need libpthread go here

testusb: testusb.o
	$(QUIET_LINK) $(CC) $< -o $@ $(CFLAGS) $(LIBPTHREAD_LIBS)

testusb.o: testusb.c
	$(QUIET_CC) $(CC) $< -o $@ -c $(CFLAGS)



# Tools which need librt go here

msc: msc.o
	$(QUIET_LINK) $(CC) $< -o $@ $(CFLAGS) $(LIBRT_LIBS)

msc.o: msc.c
	$(QUIET_CC) $(CC) $< -o $@ -c $(CFLAGS)



# Tools which don't have special requirements go here

acmc: acmc.o
	$(QUIET_LINK) $(CC) $< -o $@ $(CFLAGS)

acmc.o: acmc.c
	$(QUIET_CC) $(CC) $< -o $@ -c $(CFLAGS)

switchbox: switchbox.o
	$(QUIET_LINK) $(CC) $< -o $@ $(CFLAGS)

switchbox.o: switchbox.c
	$(QUIET_CC) $(CC) $< -o $@ -c $(CFLAGS)

# cleaning

clean:
	$(QUIET_CLEAN) rm -f	\
		*.o		\
		acmc		\
		acmd		\
		cleware		\
		companion-desc	\
		control		\
		device-reset	\
		msc		\
		serialc		\
		seriald		\
		switchbox	\
		testmode	\
		testusb

