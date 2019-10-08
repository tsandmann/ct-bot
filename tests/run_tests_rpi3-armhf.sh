#!/bin/bash

export BUILD_TARGET=armv8l-linux-gnueabihf
export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ "`uname`" == "Darwin" ]]; then
	if [[ ! -e $MYDIR/armv8l-unknown-linux-gnueabihf ]]; then
		command -v armv8l-linux-gnueabihf-g++ >/dev/null 2>&1 || { cd $MYDIR/; curl -L -o armv8l-toolchain-mac.tbz2 'https://www.dropbox.com/s/lx4pk4tlqprkat5/armv8l-toolchain-mac_8.2.tbz2'; tar xjf armv8l-toolchain-mac.tbz2; }
	fi

	export PATH=$MYDIR/armv8l-unknown-linux-gnueabihf/bin:$PATH
fi

if [[ "`uname`" == "Linux" ]]; then
	if [[ ! -e $MYDIR/armv8l-unknown-linux-gnueabihf ]]; then
		command -v armv8l-linux-gnueabihf-g++ >/dev/null 2>&1 || { cd $MYDIR/; curl -L -o armv8l-toolchain-linux.tbz2 'https://www.dropbox.com/s/t90rpqltfqhbite/armv8l-toolchain-linux_8.2_trusty.tbz2'; tar xjf armv8l-toolchain-linux.tbz2; }
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

	make DEVICE=PC WERROR=1 TESTRUN=1 -j$cores
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

	if [ "$#" -eq 1 ]; then
		echo "only file \"$filename\" was processed."
		break
	fi

done

echo ""; echo ""; echo "ALL TESTS FOR RPI3-arm PASSED."; echo ""; echo ""

exit 0;
