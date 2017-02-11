#!/bin/bash

for filename in pc/tests/*.h; do
	cp -v $filename bot-local-override.h
	make DEVICE=PC
	rc=$?
	rm bot-local-override.h
	if [[ $rc != 0 ]]; then 
		exit $rc; 
	fi
	make DEVICE=PC clean
done

for filename in mcu/tests/*.h; do
	cp -v $filename bot-local-override.h
	make DEVICE=MCU
	rc=$?
	rm bot-local-override.h
	if [[ $rc != 0 ]]; then 
		exit $rc; 
	fi
	make DEVICE=MCU clean
done

exit 0;
