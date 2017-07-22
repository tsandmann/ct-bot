#!/bin/bash

export ARM_TARGET=armv8l-linux-gnueabihf
export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ "`uname`" == "Darwin" ]]; then
	if [[ ! -e $MYDIR/armv8l-unknown-linux-gnueabihf ]]; then
		command -v armv8l-linux-gnueabihf-g++ >/dev/null 2>&1 || { git clone --depth=1 --branch=master https://github.com/tsandmann/armv8l-toolchain-mac.git $MYDIR/armv8l-unknown-linux-gnueabihf; }
	fi

	export PATH=$MYDIR/armv8l-unknown-linux-gnueabihf/bin:$PATH
fi

if [[ "`uname`" == "Linux" ]]; then
	if [[ ! -e $MYDIR/armv8l-unknown-linux-gnueabihf ]]; then
		command -v armv8l-linux-gnueabihf-g++ >/dev/null 2>&1 || { git clone --depth=1 --branch=master https://github.com/tsandmann/armv8l-toolchain-linux.git $MYDIR/armv8l-unknown-linux-gnueabihf; }
	fi

	export PATH=$MYDIR/armv8l-unknown-linux-gnueabihf/bin:$PATH
fi

cd $MYDIR/../
for filename in $MYDIR/pc/*.h; do
	cp -v $filename $MYDIR/../bot-local-override.h
	cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
	echo "using $cores parallel jobs"
	make DEVICE=PC WERROR=1 -j$cores
	rc=$?
	rm $MYDIR/../bot-local-override.h
	if [[ $rc != 0 ]]; then
		echo ""; echo ""; echo "TEST $filename FOR RPI3 FAILED."; echo ""; echo ""
		make DEVICE=PC clean >/dev/null
		exit $rc;
	fi
	make DEVICE=PC clean >/dev/null
done

echo ""; echo ""; echo "ALL TESTS FOR RPI3 PASSED."; echo ""; echo ""

exit 0;
