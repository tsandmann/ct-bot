#!/bin/bash

export BUILD_TARGET=arm-linux-gnueabihf
export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ "`uname`" == "Darwin" ]]; then
	if [[ ! -e $MYDIR/arm-unknown-linux-gnueabihf ]]; then
		command -v arm-linux-gnueabihf-g++ >/dev/null 2>&1 || { git clone --depth=1 --branch=master https://github.com/tsandmann/arm-toolchain-mac.git $MYDIR/arm-unknown-linux-gnueabihf; }
	fi

	export PATH=$MYDIR/arm-unknown-linux-gnueabihf/bin:$PATH
fi

if [[ "`uname`" == "Linux" ]]; then
	if [[ ! -e $MYDIR/arm-unknown-linux-gnueabihf ]]; then
		command -v arm-linux-gnueabihf-g++ >/dev/null 2>&1 || { git clone --depth=1 --branch=gcc-5.4 https://github.com/tsandmann/arm-toolchain-linux.git $MYDIR/arm-unknown-linux-gnueabihf; }
	fi

	export PATH=$MYDIR/arm-unknown-linux-gnueabihf/bin:$PATH
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
		echo ""; echo ""; echo "TEST $filename FOR RPI2 FAILED."; echo ""; echo ""
		make DEVICE=PC clean >/dev/null
		exit $rc;
	fi
	make DEVICE=PC clean >/dev/null
done

echo ""; echo ""; echo "ALL TESTS FOR RPI2 PASSED."; echo ""; echo ""

exit 0;
