#!/bin/bash

export BUILD_TARGET=armv8l-linux-gnueabihf
export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ "`uname`" == "Darwin" ]]; then
	if [[ ! -e $MYDIR/armv8l-unknown-linux-gnueabihf ]]; then
		command -v armv8l-linux-gnueabihf-g++ >/dev/null 2>&1 || { cd $MYDIR/; curl -L -o armv8l-toolchain-mac.tbz2 'https://www.dropbox.com/s/7375ckr4wgiqp86/armv8l-toolchain-mac.tbz2'; tar xjf armv8l-toolchain-mac.tbz2; }
	fi

	export PATH=$MYDIR/armv8l-unknown-linux-gnueabihf/bin:$PATH
fi

if [[ "`uname`" == "Linux" ]]; then
	if [[ ! -e $MYDIR/armv8l-unknown-linux-gnueabihf ]]; then
		command -v armv8l-linux-gnueabihf-g++ >/dev/null 2>&1 || { cd $MYDIR/; curl -L -o armv8l-toolchain-linux.tbz2 'https://www.dropbox.com/s/y6429vlagozve0r/armv8l-toolchain-linux.tbz2'; tar xjf armv8l-toolchain-linux.tbz2; }
	fi
	export LD_LIBRARY_PATH=$MYDIR/armv8l-unknown-linux-gnueabihf/bin
	export PATH=$MYDIR/armv8l-unknown-linux-gnueabihf/bin:$PATH
fi

cd $MYDIR/../

for filename in $MYDIR/pc/*.h; do
	if [ -e "$MYDIR/../bot-local-override.h" ]
	then
		echo "$MYDIR/../bot-local-override.h exists."
		mv -v $MYDIR/../bot-local-override.h $MYDIR/../bot-local-override.h.saved
	fi
	cp -v $filename $MYDIR/../bot-local-override.h

	cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
	echo "using $cores parallel jobs"

	make DEVICE=PC WERROR=1 -j$cores
	rc=$?

	rm $MYDIR/../bot-local-override.h
	if [ -e "$MYDIR/../bot-local-override.h.saved" ]
	then
		mv -v $MYDIR/../bot-local-override.h.saved $MYDIR/../bot-local-override.h
		echo "$MYDIR/../bot-local-override.h restored."
	fi

	if [[ $rc != 0 ]]; then
		echo ""; echo ""; echo "TEST $filename FOR RPI3-arm FAILED."; echo ""; echo ""
		make DEVICE=PC clean >/dev/null
		exit $rc;
	fi
	make DEVICE=PC clean >/dev/null
	
	echo ""
done

echo ""; echo ""; echo "ALL TESTS FOR RPI3-arm PASSED."; echo ""; echo ""

exit 0;
