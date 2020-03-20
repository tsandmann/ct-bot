#!/bin/bash

export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ "$TRAVIS" == "true" ]]; then

if [[ "`uname`" == "Darwin" ]]; then
	if [[ ! -e $MYDIR/avr8-gnu-toolchain-darwin_x86_64 ]]; then
		mkdir $MYDIR/avr8-gnu-toolchain-darwin_x86_64; cd $MYDIR/avr8-gnu-toolchain-darwin_x86_64; curl -L -o avr8-gnu-toolchain-osx.tar.gz 'https://dl.bintray.com/platformio/dl-packages/toolchain-atmelavr-darwin_x86_64-1.70300.191015.tar.gz'; tar xzf avr8-gnu-toolchain-osx.tar.gz
	fi

	export PATH=$MYDIR/avr8-gnu-toolchain-darwin_x86_64/bin:$PATH
fi

if [[ "`uname`" == "Linux" ]]; then
	if [[ ! -e $MYDIR/avr8-gnu-toolchain-linux_x86_64 ]]; then
		mkdir $MYDIR/avr8-gnu-toolchain-linux_x86_64; cd $MYDIR/avr8-gnu-toolchain-linux_x86_64; curl -L -o avr8-gnu-toolchain-linux.tar.gz 'https://dl.bintray.com/platformio/dl-packages/toolchain-atmelavr-linux_x86_64-1.70300.191015.tar.gz'; tar xzf avr8-gnu-toolchain-linux.tar.gz
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

		if [ "$#" -eq 1 ]; then
			case $1 in
			  /*) filename=$1 ;;
			  *) filename=$MYDIR/$1 ;;
			esac
			echo "just using file \"$filename\" as input."
		fi

		cp -v $filename $MYDIR/../bot-local-override.h
		if [ $? -ne 0 ]; then
			echo "file \"$filename\" not found, abort."
			echo ""; echo ""; echo "TEST $filename FOR PC $BUILD_TARGET FAILED."; echo ""; echo ""
			make DEVICE=PC clean >/dev/null
			exit 1
		fi

		cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
		echo "using $cores parallel jobs"

		make DEVICE=MCU MCU=$device WERROR=1 WCONVERSION=1 TESTRUN=1 -j$cores
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

		if [ "$#" -eq 1 ]; then
			echo "only file \"$filename\" was processed."
			break
		fi

	done
done

echo ""; echo ""; echo "ALL TESTS FOR MCU PASSED."; echo ""; echo ""

exit 0;
