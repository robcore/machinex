#!/bin/bash

ONLINE=`adb get-state 2> /dev/null`
if [[ $ONLINE == device ]]; then
	echo "connected"
else
	echo "disconnected, retrying"
fi