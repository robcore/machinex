#!/bin/bash
#!/bin/bash
echo 'beginning arch conversions'
cd arch
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/early_suspend/power_suspend/' {} \;
echo 'one finished'
sleep 1
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/earlysuspend/powersuspend/' {} \;
echo 'two finished'
sleep 1
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/late_resume/power_resume/' {} \;
echo 'three finished'
sleep 1
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/CONFIG_HAS_EARLYSUSPEND/CONFIG_POWERSUSPEND/' {} \;
sleep 1
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/CONFIG_EARLYSUSPEND/CONFIG_POWERSUSPEND/' {} \;
sleep 1
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/CONFIG_HAS_POWERSUSPEND/CONFIG_POWERSUSPEND/' {} \;
echo 'finished arch'
