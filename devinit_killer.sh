#!/bin/bash

killer() {
find -type f | xargs sed -i "s/ __devinit / /g"
find -type f | xargs sed -i "s/ __devexit_p / /g"
find -type f | xargs sed -i "s/ __devexit / /g"
find -type f | xargs sed -i "s/ __devexitdata//g"
find -type f | xargs sed -i "s/ __devinit$//g"
find -type f | xargs sed -i "s/^__devinit //g"
find -type f | xargs sed -i "s/ __devinitdata//g"
find -type f | xargs sed -i "s/ __devexitconst//g"
find -type f | xargs sed -i "s/ __devinitconst//g"
}

#define __devinit
#define __devinitdata
#define __devinitconst
#define __devexit
#define __devexitdata
#define __devexitconst
#define __devexit_p(x) x
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
cd ~/machinex/include
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
