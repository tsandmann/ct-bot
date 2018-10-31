#!/bin/bash

export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ "$TRAVIS" == "true" ]]; then
	
if [[ "`uname`" == "Darwin" ]]; then
	if [[ ! -e $MYDIR/avr8-gnu-toolchain-darwin_x86_64 ]]; then
		cd $MYDIR/; curl -L -o avr8-gnu-toolchain-osx.tar.gz 'http://ww1.microchip.com/downloads/en/DeviceDoc/avr8-gnu-toolchain-osx-3.6.2.503-darwin.any.x86_64.tar.gz'; tar xzf avr8-gnu-toolchain-osx.tar.gz
	fi

	export PATH=$MYDIR/avr8-gnu-toolchain-darwin_x86_64/bin:$PATH
fi

if [[ "`uname`" == "Linux" ]]; then
	if [[ ! -e $MYDIR/avr8-gnu-toolchain-linux_x86_64 ]]; then
		cd $MYDIR/; curl -L -o avr8-gnu-toolchain-linux.tar.gz 'http://ww1.microchip.com/downloads/en/DeviceDoc/avr8-gnu-toolchain-3.6.2.1759-linux.any.x86_64.tar.gz'; tar xzf avr8-gnu-toolchain-linux.tar.gz
	fi
	
	export PATH=$MYDIR/avr8-gnu-toolchain-linux_x86_64/bin:$PATH
fi

fi

for device in "atmega1284p" "atmega644p"; do
	echo "building for $device"

	cd $MYDIR/../

	for filename in $MYDIR/mcu/*.h; do
		if [ -e "$MYDIR/../bot-local-override.h" ]
		then
			echo "$MYDIR/../bot-local-override.h exists."
			mv -v $MYDIR/../bot-local-override.h $MYDIR/../bot-local-override.h.saved
		fi
		cp -v $filename $MYDIR/../bot-local-override.h

		cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
		echo "using $cores parallel jobs"

		make DEVICE=MCU MCU=$device WERROR=1 WCONVERSION=1 -j$cores
		rc=$?

		rm $MYDIR/../bot-local-override.h
		if [ -e "$MYDIR/../bot-local-override.h.saved" ]
		then
			mv -v $MYDIR/../bot-local-override.h.saved $MYDIR/../bot-local-override.h
			echo "$MYDIR/../bot-local-override.h restored."
		fi
		
		if [[ $rc != 0 ]]; then
			echo ""; echo ""; echo "TEST $filename FOR MCU FAILED."; echo ""; echo ""
			make DEVICE=MCU MCU=$device clean >/dev/null
			exit $rc;
		fi
		make DEVICE=MCU MCU=$device clean >/dev/null

		echo ""
	done
done

echo ""; echo ""; echo "ALL TESTS FOR MCU PASSED."; echo ""; echo ""

exit 0;
