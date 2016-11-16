#!/bin/bash
#!/bin/bash
echo 'beginning driver conversions'
cd drivers
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
echo 'finished drivers'
sleep 1
echo 'beginning include conversions'
cd ../include
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
cd ../arch
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
echo 'include finished'
