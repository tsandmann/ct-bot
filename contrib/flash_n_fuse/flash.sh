#!/bin/sh
# echo program C't-Bot with avrdude and mySmartUSB
#
# echo Usage: flash.sh <filename>
#
if [ $# != 1 ]; then
	echo "Usage: flash.sh <filename>"
	exit 1;
fi
# now check if the fuses are set correctly
# use temporary file names in /tmp for it
lowtemp="$(mktemp /tmp/lfuse.XXXXX)"
hightemp="$(mktemp /tmp/hfuse.XXXXX)"
locktemp="$(mktemp /tmp/lock.XXXXX)"

# read fuses
avrdude -p m32 -c avr910 -P /dev/ttyUSB0 -U lfuse:r:$lowtemp:i \
	-U hfuse:r:$hightemp:i -U lock:r:$locktemp:i >/dev/null 2>&1
if [ $? != "0" ]; then
	echo "Error while reading fuses!"
	exit 1
fi

fuses_ok=1
# now compare the values with the correspondent .hex file
cmp -n 13 lfuse.hex $lowtemp >/dev/null 2>&1
if [ $? != "0" ]; then 
	echo "low fuse doesn't fit!"
	fuses_ok=0;
fi
cmp -n 13 hfuse.hex $hightemp >/dev/null 2>&1
if [ $? != "0" ]; then
        echo "high fuse doesn't fit!"
        fuses_ok=0;
fi
cmp -n 13 lock.hex $locktemp >/dev/null 2>&1
if [ $? != "0" ]; then
        echo "lock byte doesn't fit!"
        fuses_ok=0;
fi


# clean up temp files
rm $lowtemp
rm $hightemp
rm $locktemp

# if some or all fuses were wrong, give hint and bail out
if [ $fuses_ok != "1" ]; then
	echo "Please correct your fuses with setfuses.sh, else you'll probably get locked out from your MCU!"
	exit 2;
fi

# all fuses ok, now program flash
avrdude -p m32 -c avr910 -P /dev/ttyUSB0 -e -U flash:w:$1:i
