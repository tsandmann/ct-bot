#!/bin/sh
# echo program C't-Bot with avrdude and mySmartUSB
#
# echo Usage: flash.sh <filename>
#
if [ $# != 1 ]; then
	echo "Usage: flash.sh <filename>"
	exit 1;
fi

avrdude -p m32 -c avr910 -P /dev/ttyUSB0 -e -U flash:w:$1:i
