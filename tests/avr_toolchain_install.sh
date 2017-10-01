#!/bin/bash

export MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ "`uname`" == "Darwin" ]]; then
	if [[ ! -e $MYDIR/avr8-gnu-toolchain-darwin_x86_64 ]]; then
		cd $MYDIR/
		curl -L -o avr8-gnu-toolchain-osx.tar.gz 'http://distribute.atmel.no/tools/opensource/Atmel-AVR-GNU-Toolchain/3.6.0/avr8-gnu-toolchain-osx-3.6.0.487-darwin.any.x86_64.tar.gz'
		tar xzf avr8-gnu-toolchain-osx.tar.gz
	fi

	export PATH=$MYDIR/avr8-gnu-toolchain-darwin_x86_64/bin:$PATH
fi

if [[ "`uname`" == "Linux" ]]; then
	if [[ ! -e $MYDIR/avr8-gnu-toolchain-linux_x86_64 ]]; then
		cd $MYDIR/
		curl -L -o avr8-gnu-toolchain-linux.tar.gz 'http://www.atmel.com/images/avr8-gnu-toolchain-3.5.4.1709-linux.any.x86_64.tar.gz'
		tar xzf avr8-gnu-toolchain-linux.tar.gz
	fi

	export PATH=$MYDIR/avr8-gnu-toolchain-linux_x86_64/bin:$PATH
fi
