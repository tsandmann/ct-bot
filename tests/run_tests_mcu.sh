#!/bin/bash

for filename in tests/mcu/*.h; do
	cp -v $filename bot-local-override.h
	cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
	echo "using $cores parallel jobs"
	make DEVICE=MCU WERROR=1 WCONVERSION=0 -j$cores
	rc=$?
	rm bot-local-override.h
	if [[ $rc != 0 ]]; then
		echo ""; echo ""; echo "TEST $filename FAILED."; echo ""; echo ""
		make DEVICE=MCU clean >/dev/null
		exit $rc; 
	fi
	make DEVICE=MCU clean >/dev/null
done

echo ""; echo ""; echo "ALL TESTS PASSED."; echo ""; echo ""

exit 0;
