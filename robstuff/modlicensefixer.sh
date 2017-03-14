#!/bin/bash
#!/bin/bash
echo 'beginning arch conversions'
cd /root/machinex/arch/
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/GPLv2/GPL v2/' {} \;
echo 'arch finished'
cd /root/machinex/crypto/
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/GPLv2/GPL v2/' {} \;
echo 'crypto finished'
cd /root/machinex/drivers
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/GPLv2/GPL v2/' {} \;
echo 'drivers finished'
cd /root/machinex/sound
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/GPLv2/GPL v2/' {} \;
echo 'sound finished'
echo 'we are all done here'
