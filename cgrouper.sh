#!/bin/bash
cd ~/machinex/block
find . -type f | parallel sed -i 's/cgroup_subsys_state/cgroup_css/' {} \;
find . -type f | parallel sed -i 's/task_subsys_state/task_css/' {} \;
echo 'block complete'

cd ~/machinex/drivers
find . -type f | parallel sed -i 's/cgroup_subsys_state/cgroup_css/' {} \;
find . -type f | parallel sed -i 's/task_subsys_state/task_css/' {} \;
echo 'drivers complete'

cd ~/machinex/Documentation
find . -type f | parallel sed -i 's/cgroup_subsys_state/cgroup_css/' {} \;
find . -type f | parallel sed -i 's/task_subsys_state/task_css/' {} \;
echo 'Documentation complete'

cd ~/machinex/include
find . -type f | parallel sed -i 's/cgroup_subsys_state/cgroup_css/' {} \;
find . -type f | parallel sed -i 's/task_subsys_state/task_css/' {} \;
echo 'include complete'

cd ~/machinex/kernel
find . -type f | parallel sed -i 's/cgroup_subsys_state/cgroup_css/' {} \;
find . -type f | parallel sed -i 's/task_subsys_state/task_css/' {} \;
echo 'kernel complete'

cd ~/machinex/mm
find . -type f | parallel sed -i 's/cgroup_subsys_state/cgroup_css/' {} \;
find . -type f | parallel sed -i 's/task_subsys_state/task_css/' {} \;
echo 'mm complete'

cd ~/machinex/net
find . -type f | parallel sed -i 's/cgroup_subsys_state/cgroup_css/' {} \;
find . -type f | parallel sed -i 's/task_subsys_state/task_css/' {} \;
echo 'net complete'

cd ~/machinex/security
find . -type f | parallel sed -i 's/cgroup_subsys_state/cgroup_css/' {} \;
find . -type f | parallel sed -i 's/task_subsys_state/task_css/' {} \;
echo 'security complete'

echo "all done"
