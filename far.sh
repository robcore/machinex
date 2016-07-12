#!/bin/bash
echo 'beginning'
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/DEFERRED_WORK_INITIALIZER/DEFERRABLE_WORK_INITIALIZER/' {} \;
echo 'one finished'
sleep 3
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/DECLARE_DEFERRED_WORK/DECLARE_DEFERRABLE_WORK/' {} \;
echo 'two finished'
sleep 3
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/INIT_DELAYED_WORK_DEFERRABLE/INIT_DEFERRABLE_WORK/' {} \;
echo 'finished'
