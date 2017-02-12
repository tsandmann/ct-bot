#!/bin/bash

for filename in tests/pc/*.h; do
	cp -v $filename bot-local-override.h
	cores=$(grep -c "^processor" /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
	echo "using $cores parallel jobs"
	make DEVICE=PC WERROR=1 -j$cores
	rc=$?
	rm bot-local-override.h
	if [[ $rc != 0 ]]; then
		echo ""; echo ""; echo "TEST $filename FAILED."; echo ""; echo ""
		make DEVICE=PC clean >/dev/null
		exit $rc; 
	fi
	make DEVICE=PC clean >/dev/null
done

echo ""; echo ""; echo "ALL TESTS PASSED."; echo ""; echo ""

exit 0;
