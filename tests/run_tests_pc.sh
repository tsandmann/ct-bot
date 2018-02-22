#!/bin/bash

export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

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
		echo ""; echo ""; echo "TEST $filename FOR PC FAILED."; echo ""; echo ""
		make DEVICE=PC clean >/dev/null
		exit $rc;
	fi
	make DEVICE=PC clean >/dev/null

	echo ""
done

echo ""; echo ""; echo "ALL TESTS FOR PC PASSED."; echo ""; echo ""

exit 0;
