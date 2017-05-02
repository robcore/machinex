#!/bin/bash
cd ~/machinex/drivers
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/CONFIG_PM_RUNTIME/CONFIG_PM/' {} \;
echo 'drivers complete'
cd ~/machinex/include/linux/
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/CONFIG_PM_RUNTIME/CONFIG_PM/' {} \;
echo 'include complete'
cd ~/machinex/kernel
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/CONFIG_PM_RUNTIME/CONFIG_PM/' {} \;
echo 'kernel complete'
cd ~/machinex/sound/
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/CONFIG_PM_RUNTIME/CONFIG_PM/' {} \;
echo 'finito'
