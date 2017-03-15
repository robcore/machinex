#!/bin/bash

killer() {
find . -type f | parallel sed -i "s/ __cpuinit / /g" {} \;
find . -type f | parallel sed -i "s/ __cpuexit / /g" {} \;
find . -type f | parallel sed -i "s/ __cpuinitdata//g" {} \;
find . -type f | parallel sed -i "s/ __cpuinit$//g" {} \;
find . -type f | parallel sed -i "s/^__cpuinit //g" {} \;
find . -type f | parallel sed -i "s/ __cpuinit//g" {} \;
}
finder() {
find . -type f | parallel grep -rl '__devinit\|__devinitdata\|__devinitconst\|__devexit\|__devexitdata\|__devexitconst\|__devexit_p' {} \; | sort -d >> ~/machinex/devinit_remains.txt
}

cd ~/machinex
touch devinit_remains.txt

cd ~/machinex/arch
killer
finder
cd ~/machinex/block
finder
cd ~/machinex/crypto
finder
cd ~/machinex/Documentation
finder
cd ~/machinex/drivers
killer
finder
cd ~/machinex/firmware
finder
cd ~/machinex/fs
finder
cd ~/machinex/init
finder
cd ~/machinex/ipc
finder
cd ~/machinex/kernel
finder
cd ~/machinex/lib
finder
cd ~/machinex/mm
finder
cd ~/machinex/net
finder
cd ~/machinex/samples
finder
cd ~/machinex/scripts
finder
cd ~/machinex/security
finder
cd ~/machinex/sound
finder
cd ~/machinex/tools
finder
cd ~/machinex/usr
finder
cd ~/machinex/virt
finder
echo "complete"
