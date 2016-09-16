/*
 * Author: Jean-Pierre Rasquin <yank555.lu@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * CPU freq hard limit - SysFS interface :
 * ---------------------------------------
 *
 * /sys/kernel/cpufreq/hardlimit (rw)
 *
 *   set or show the real hard CPU frequency limit
 *
 * /sys/kernel/cpufreq/version (ro)
 *
 *   display CPU freq hard limit version information
 *
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/cpufreq_hardlimit.h>
#include <linux/cpufreq.h>

unsigned int hardlimit = CPUFREQ_HARDLIMIT_STOCK;

/* Externally reachable function call */

unsigned int check_cpufreq_hardlimit(unsigned int freq)
{
	return min(hardlimit, freq); /* Enforce hard limit */
}

/* sysfs interface for "hardlimit" */
static ssize_t hardlimit_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hardlimit);
}

static ssize_t hardlimit_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{

	unsigned int new_hardlimit, i;

	struct cpufreq_frequency_table *table;

	if (!sscanf(buf, "%du", &new_hardlimit))
		return -EINVAL;

	table = cpufreq_frequency_get_table(0); /* Get frequency table */

	for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++)
		if (table[i].frequency == new_hardlimit) {
		    hardlimit = new_hardlimit;
		    return count;
		}

	return -EINVAL;

}

static struct kobj_attribute hardlimit_attribute =
__ATTR(hardlimit, 0666, hardlimit_show, hardlimit_store);

/* sysfs interface for "version" */
static ssize_t version_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CPUFREQ_HARDLIMIT_VERSION);
}

static struct kobj_attribute version_attribute =
__ATTR(version, 0444, version_show, NULL);

/* Initialize sysfs folder */
static struct kobject *hardlimit_kobj;

static struct attribute *hardlimit_attrs[] = {
	&hardlimit_attribute.attr,
	&version_attribute.attr,
	NULL,
};

static struct attribute_group hardlimit_attr_group = {
.attrs = hardlimit_attrs,
};

int hardlimit_init(void)
{
	int hardlimit_retval;

        hardlimit_kobj = kobject_create_and_add("cpufreq", kernel_kobj);
        if (!hardlimit_kobj) {
                return -ENOMEM;
        }
        hardlimit_retval = sysfs_create_group(hardlimit_kobj, &hardlimit_attr_group);
        if (hardlimit_retval)
                kobject_put(hardlimit_kobj);
        return (hardlimit_retval);
}
/* end sysfs interface */

void hardlimit_exit(void)
{
	kobject_put(hardlimit_kobj);
}

module_init(hardlimit_init);
module_exit(hardlimit_exit);


