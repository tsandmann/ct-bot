#!/bin/sh
# create ttyUSB0 device and load module for cp2101

if [ ! -c /dev/ttyUSB0 ]; then
	mknod -m 666 /dev/ttyUSB0 c 188 0
fi

if [ -z $(lsmod |cut -d" " -f1 |grep "cp2101") ]; then
	modprobe cp2101 >/dev/null 2>&1
	if [ $? != "0" ]; then
		exit 1;
	fi
fi

