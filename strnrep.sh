#!/bin/bash

cd arch/arm/mach-msm
grep -rl 'strnicmp' ./ | xargs sed -i 's/strnicmp/strncasecmp/g';
cd ~/machinex
cd drivers
grep -rl 'strnicmp' ./ | xargs sed -i 's/strnicmp/strncasecmp/g';
cd ~/machinex
cd fs
grep -rl 'strnicmp' ./ | xargs sed -i 's/strnicmp/strncasecmp/g';
cd ~/machinex
cd include/linux
grep -rl 'strnicmp' ./ | xargs sed -i 's/strnicmp/strncasecmp/g';
cd ~/machinex
cd kernel/debug/kdb/
grep -rl 'strnicmp' ./ | xargs sed -i 's/strnicmp/strncasecmp/g';
cd ~/machinex
cd net
grep -rl 'strnicmp' ./ | xargs sed -i 's/strnicmp/strncasecmp/g';
cd ~/machinex
cd sound
grep -rl 'strnicmp' ./ | xargs sed -i 's/strnicmp/strncasecmp/g';
echo "done"