#include <linux/fault-inject.h>
#include <linux/slab.h>

static struct {
	struct fault_attr attr;
	u32 ignore_gfp_wait;
	int cache_filter;
	u32 size;
} failslab = {
	.attr = FAULT_ATTR_INITIALIZER,
	.ignore_gfp_wait = 1,
	.cache_filter = 0,
	.size = 0,
};

static void fail_dump(struct fault_attr *attr)
{
	if (attr->verbose > 0)
		printk(KERN_NOTICE "FAULT_INJECTION: forcing a failure\n");
	if (attr->verbose > 1)
		dump_stack();
}

bool should_failslab(size_t size, gfp_t gfpflags, unsigned long cache_flags)
{
	if (failslab.size && size > failslab.size) {
		fail_dump(&failslab.attr);
		return true;
	}

	if (gfpflags & __GFP_NOFAIL)
		return false;

        if (failslab.ignore_gfp_wait && (gfpflags & __GFP_WAIT))
		return false;

	if (failslab.cache_filter && !(cache_flags & SLAB_FAILSLAB))
		return false;

	return should_fail(&failslab.attr, size);
}

static int __init setup_failslab(char *str)
{
	return setup_fault_attr(&failslab.attr, str);
}
__setup("failslab=", setup_failslab);

#ifdef CONFIG_FAULT_INJECTION_DEBUG_FS
static int __init failslab_debugfs_init(void)
{
	struct dentry *dir;
	umode_t mode = S_IFREG | S_IRUSR | S_IWUSR;

	dir = fault_create_debugfs_attr("failslab", NULL, &failslab.attr);
	if (IS_ERR(dir))
		return PTR_ERR(dir);

	if (!debugfs_create_bool("ignore-gfp-wait", mode, dir,
				&failslab.ignore_gfp_wait))
		goto fail;
	if (!debugfs_create_bool("cache-filter", mode, dir,
				&failslab.cache_filter))
		goto fail;

	if (!debugfs_create_u32("size", mode, dir,
				&failslab.size))
		goto fail;

	return 0;
fail:
	debugfs_remove_recursive(dir);

	return -ENOMEM;
}

late_initcall(failslab_debugfs_init);

#endif /* CONFIG_FAULT_INJECTION_DEBUG_FS */
