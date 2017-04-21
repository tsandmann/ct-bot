#!/bin/bash

export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ "$TRAVIS" == "true" ]]; then
	source $MYDIR/avr_toolchain_install.sh
fi

for device in "atmega1284p" "atmega644p"; do
	echo "building for $device"

	cd $MYDIR/../
	for filename in $MYDIR/mcu/*.h; do
		cp -v $filename $MYDIR/../bot-local-override.h
		cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
		echo "using $cores parallel jobs"
		make DEVICE=MCU MCU=$device WERROR=1 WCONVERSION=0 -j$cores
		rc=$?
		rm $MYDIR/../bot-local-override.h
		if [[ $rc != 0 ]]; then
			echo ""; echo ""; echo "TEST $filename FOR MCU FAILED."; echo ""; echo ""
			make DEVICE=MCU MCU=$device clean >/dev/null
			exit $rc;
		fi
		make DEVICE=MCU MCU=$device clean >/dev/null
	done
done

echo ""; echo ""; echo "ALL TESTS FOR MCU PASSED."; echo ""; echo ""

exit 0;
