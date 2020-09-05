#!/bin/bash

export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for environment in "test_1284p" "test_644p"; do
	echo "building for $environment"

	cd $MYDIR/../

	for filename in $MYDIR/mcu/*.h; do
		rm -rf .pio

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
			rm -rf .pio
			exit 1
		fi

		cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
		echo "using $cores parallel jobs"
		pio run -e $environment
		rc=$?

		rm $MYDIR/../bot-local-override.h
		if [ -e "$MYDIR/../bot-local-override.h.saved" ]
		then
			mv -v $MYDIR/../bot-local-override.h.saved $MYDIR/../bot-local-override.h
			echo "$MYDIR/../bot-local-override.h restored."
		fi

		rm -rf .pio
		if [[ $rc != 0 ]]; then
			echo ""; echo ""; echo "TEST $filename FOR MCU FAILED."; echo ""; echo ""
			exit $rc;
		fi

		echo ""

		if [ "$#" -eq 1 ]; then
			echo "only file \"$filename\" was processed."
			break
		fi

	done
done

echo ""; echo ""; echo "ALL TESTS FOR MCU PASSED."; echo ""; echo ""

exit 0;
