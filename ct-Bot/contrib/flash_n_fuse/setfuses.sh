#!/bin/sh
# echo program C't-Bot with avrdude and mySmartUSB

avrdude -p m32 -c avr910 -P /dev/ttyUSB0 -e -u -U lfuse:w:lfuse.hex:i -U hfuse:w:hfuse.hex:i -U lock:w:lock.hex:i