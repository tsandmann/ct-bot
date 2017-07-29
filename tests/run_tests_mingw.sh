#!/bin/bash

export BUILD_TARGET=x86_64-w64-mingw32
export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $MYDIR/../
for filename in $MYDIR/pc/*.h; do
	cp -v $filename $MYDIR/../bot-local-override.h
	cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
	echo "using $cores parallel jobs"
	make DEVICE=PC WERROR=1 -j$cores
	rc=$?
	rm $MYDIR/../bot-local-override.h
	if [[ $rc != 0 ]]; then
		echo ""; echo ""; echo "TEST $filename FOR MINGW FAILED."; echo ""; echo ""
		make DEVICE=PC clean >/dev/null
		exit $rc;
	fi
	make DEVICE=PC clean >/dev/null
done

echo ""; echo ""; echo "ALL TESTS FOR MINGW PASSED."; echo ""; echo ""

exit 0;
