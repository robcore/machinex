#!/bin/bash

t=0

while [ $t -lt 10 ]; do
	adb shell cat /sys/devices/system/cpu/cpu0/rq-stats/cpu_normalized_load
	sleep 1
	t=$(( t + 1 ))
done