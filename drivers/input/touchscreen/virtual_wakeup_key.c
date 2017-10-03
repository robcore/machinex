#include <linux/module.h>
#include <linux/kernel.h>    
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/timed_output.h>
#include <linux/wakelock.h>
#include <linux/sysfs_helpers.h>

#define DRIVER_AUTHOR "robcore"
#define DRIVER_DESCRIPTION "virtual_wakeup_key driver"
#define DRIVER_VERSION "1.5"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPLv2");

/*Based on Sweep2Sleep by flar2 (Aaron Segaert) */

#define WPTIMEOUT 80
#define WRTIMEOUT 10

static struct input_dev *virtkeydev;
static struct delayed_work wakeup_key_press_work;
static void wakeup_key_press(struct work_struct *work);
static struct work_struct virtkey_input_work;
struct wake_lock vwklock;
unsigned int screen_wake_lock = 0;

static void press_key(unsigned int pressed)
{
	unsigned long flags;

	/* disable keyboard interrupts */
	local_irq_save(flags);
	/* don't change CPU */
	preempt_disable();

	switch (pressed) {
		case 1:
			wake_lock(&vwklock);
			input_report_key(virtkeydev, KEY_WAKEUP, 1);
			input_sync(virtkeydev);
			break;
		case 0:
			input_report_key(virtkeydev, KEY_WAKEUP, 0);
			input_sync(virtkeydev);
			wake_unlock(&vwklock);
			break;
		default:
			break;
	}
	/* reenable preemption */
	preempt_enable();
	/* reenable keyboard interrupts */
	local_irq_restore(flags);
}

/* WakeKeyPressed work func */
static void wakeup_key_press(struct work_struct *work)
{
	press_key(1);
	mdelay(100);
	press_key(0);
}

/* PowerKey trigger */
void virt_wakeup_key_trig(void) {
	schedule_delayed_work_on(0, &wakeup_key_press_work, 0);
}
EXPORT_SYMBOL(virt_wakeup_key_trig);

static void do_screen_wake(void)
{
checker:
	if (!screen_wake_lock)
			return;
	press_key(1);
	mdelay(100);
	press_key(0);
	goto checker;
}
static ssize_t screen_wake_lock_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", screen_wake_lock);
}

static ssize_t screen_wake_lock_store(struct kobject *kobj,
 struct kobj_attribute *attr,
 const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input == screen_wake_lock)
		return count;

	sanitize_min_max(input, 0, 1);

	screen_wake_lock = input;

	do_screen_wake();

	return count;
}

static struct kobj_attribute screen_wake_lock_attr =
					__ATTR(screen_wake_lock, 0644,
					screen_wake_lock_show,
					screen_wake_lock_store);

static struct attribute *virtual_wakeup_key_attrs[] = {
	&screen_wake_lock_attr.attr,
	NULL,
};

static struct attribute_group virtual_wakeup_key_attr_group = {
	.attrs = virtual_wakeup_key_attrs,
	.name = "virtual_wakeup_key",
};

static int __init virtual_wakeup_key_init(void)
{
	int rc = 0;

	virtkeydev = input_allocate_device();
	if (!virtkeydev) {
		pr_err("Failed to allocate virtkeydev\n");
		rc = -ENOMEM;
		goto err_alloc_dev;
	}


	virtkeydev->name = "virtual_wakeup_key";
	virtkeydev->id.bustype = BUS_VIRTUAL;
	virtkeydev->phys = "virtual_wake_key/virtual_wake_key";
	virtkeydev->dev.parent = NULL;

	__set_bit(EV_KEY, virtkeydev->evbit);
	__set_bit(KEY_WAKEUP, virtkeydev->keybit);

	rc = input_register_device(virtkeydev);
	if (rc) {
		pr_err("%s: input_register_device err=%d\n", __func__, rc);
		goto err_input_dev;
	}

	INIT_DELAYED_WORK(&wakeup_key_press_work, wakeup_key_press);

	rc = sysfs_create_group(kernel_kobj, &virtual_wakeup_key_attr_group);
	if (rc) {
		rc = -ENOMEM;
		goto err_unregister;
	}

	wake_lock_init(&vwklock, WAKE_LOCK_SUSPEND, "vwkey");

	return 0;

err_unregister:
	input_unregister_device(virtkeydev);
err_input_dev:
	input_free_device(virtkeydev);
err_alloc_dev:
	pr_info("%s Failed\n", __func__);

	return rc;
}

static void __exit virtual_wakeup_key_exit(void)
{
	wake_lock_destroy(&vwklock);
	sysfs_remove_group(kernel_kobj, &virtual_wakeup_key_attr_group);
	input_unregister_device(virtkeydev);
	input_free_device(virtkeydev);
	return;
}

module_init(virtual_wakeup_key_init);
module_exit(virtual_wakeup_key_exit);