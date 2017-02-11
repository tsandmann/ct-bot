#!/bin/bash

for filename in tests/pc/*.h; do
	cp -v $filename bot-local-override.h
	make DEVICE=PC
	rc=$?
	rm bot-local-override.h
	if [[ $rc != 0 ]]; then 
		exit $rc; 
	fi
	make DEVICE=PC clean
done

exit 0;
