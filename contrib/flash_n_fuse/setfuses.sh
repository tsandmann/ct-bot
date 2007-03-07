#!/bin/sh
# echo program C't-Bot with avrdude and mySmartUSB
#
# echo Usage: setfuses.sh <filename>
#
if [ $# != 1 ]; then
	echo "Usage: setfuses.sh <filename>"
	exit 1;
fi

avrdude -p m32 -c avr910 -P /dev/ttyUSB0 -e -u -U lfuse:w:lfuse.hex:i -U hfuse:w:hfuse.hex:i -U lock:w:lock.hex:i