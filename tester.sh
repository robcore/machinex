#!/bin/bash

for i in /root/machinex/arch/arm/mach-msm/alucard_hotplug.c \
/root/machinex/arch/arm/mach-msm/blu_plug.c \
/root/machinex/arch/arm/mach-msm/bricked_hotplug.c \
/root/machinex/arch/arm/mach-msm/dyn_hotplug.c \
/root/machinex/arch/arm/mach-msm/fast_hotplug.c \
/root/machinex/arch/arm/mach-msm/hotplug.c \
/root/machinex/arch/arm/mach-msm/intelli_hotplug.c \
/root/machinex/arch/arm/mach-msm/ix_hotplug.c \
/root/machinex/arch/arm/mach-msm/mako_hotplug.c \
/root/machinex/arch/arm/mach-msm/msm_hotplug.c \
/root/machinex/arch/arm/mach-msm/thunderplug.c \;
do medit $i;
done;