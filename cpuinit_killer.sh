#!/bin/bash

killer() {
find -type f | xargs sed -i "s/ __cpuinit / /g"
find -type f | xargs sed -i "s/ __cpuexit / /g"
find -type f | xargs sed -i "s/ __cpuinitdata//g"
find -type f | xargs sed -i "s/ __cpuinit$//g"
find -type f | xargs sed -i "s/^__cpuinit //g"
}

cd ~/machinex/arch
killer
cd ~/machinex/block
killer
cd ~/machinex/crypto
killer
cd ~/machinex/Documentation
killer
cd ~/machinex/drivers
killer
cd ~/machinex/firmware
killer
cd ~/machinex/fs
killer
cd ~/machinex/init
killer
cd ~/machinex/ipc
killer
cd ~/machinex/kernel
killer
cd ~/machinex/lib
killer
cd ~/machinex/mm
killer
cd ~/machinex/net
killer
cd ~/machinex/samples
killer
cd ~/machinex/scripts
killer
cd ~/machinex/security
killer
cd ~/machinex/sound
killer
cd ~/machinex/tools
killer
cd ~/machinex/usr
killer
cd ~/machinex/virt
killer
echo "complete"
