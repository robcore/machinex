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
#define DRIVER_VERSION "2.0"

/*Based on Sweep2Sleep by flar2 (Aaron Segaert) */
static struct input_dev *virtkeydev;
static struct work_struct virtkey_input_work;
struct wake_lock vwklock;

static void press_key(void)
{
	input_report_key(virtkeydev, KEY_WAKEUP, 1);
	input_sync(virtkeydev);
	input_report_key(virtkeydev, KEY_WAKEUP, 0);
	input_sync(virtkeydev);
}

/* PowerKey trigger */
void virt_wakeup_key_trig(void) {
	wake_lock(&vwklock);
	press_key();
	wake_unlock(&vwklock);
}
EXPORT_SYMBOL(virt_wakeup_key_trig);

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
	input_unregister_device(virtkeydev);
	input_free_device(virtkeydev);
	return;
}

late_initcall(virtual_wakeup_key_init);
module_exit(virtual_wakeup_key_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPLv2");