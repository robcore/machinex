#!/system/bin/sh

t=0

while [ $t -lt 10 ]; do
	cat /sys/devices/system/cpu/cpu0/mx_cpufreq/iactive_current_load
	sleep 0.1
	t=$(( t + 1 ))
done