#include <linux/module.h>
#include <linux/kernel.h>    
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/timed_output.h>

#define DRIVER_AUTHOR "robcore"
#define DRIVER_DESCRIPTION "virtualpowerkey(wakeup) driver"
#define DRIVER_VERSION "1.3"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPLv2");

/*Based on Sweep2Sleep by flar2 (Aaron Segaert) */

#define WPTIMEOUT 100
#define WRTIMEOUT 10

static struct input_dev *virtkeydev;
static struct workqueue_struct *virtkey_input_wq;
static struct delayed_work wakeup_key_release_work;
static struct delayed_work wakeup_key_press_work;
static void wakeup_key_release(struct work_struct *work);
static void wakeup_key_press(struct work_struct *work);
static struct work_struct virtkey_input_work;
static unsigned int key_is_pressed = 0;

static void press_key(unsigned int pressed)
{
	if (pressed != 0 && pressed != 1)
		return;

	input_report_key(virtkeydev, KEY_WAKEUP, pressed);
	input_sync(virtkeydev);
	key_is_pressed = pressed;
}

/* WakeKeyReleased work func */
static void wakeup_key_release(struct work_struct *work)
{
	press_key(0);
}

/* WakeKeyPressed work func */
static void wakeup_key_press(struct work_struct *work)
{
	press_key(1);
	schedule_delayed_work(&wakeup_key_release_work, msecs_to_jiffies(WRTIMEOUT));
}

/* PowerKey trigger */
void virt_wakeup_key_trig(void) {
	schedule_delayed_work_on(0, &wakeup_key_press_work, msecs_to_jiffies(WPTIMEOUT));
}
EXPORT_SYMBOL(virt_wakeup_key_trig);

static int virtkey_input_connect(struct input_handler *handler,
				struct input_dev *dev, const struct input_device_id *id) {
	struct input_handle *handle;
	int error;


	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "virt_wakeup";

	error = input_register_handle(handle);

	error = input_open_device(handle);

	return 0;

}

static void virtkey_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id virtkey_ids[] = {
	{ .driver_info = 1 },
	{ },
};

static struct input_handler virtkey_input_handler = {
	.connect	= virtkey_input_connect,
	.disconnect	= virtkey_input_disconnect,
	.name		= "virtual_power_key",
	.id_table	= virtkey_ids,
};
#if 0
static ssize_t press_wakeup_key_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", key_is_pressed);
}
#endif
/*
static int bootup_force_screen_on(void)
{
	while (!late_init_complete) {
		press_key(1);
		msleep(10);
		press_key(0);
		msleep(10);
		if (late_init_complete)
			break;
	}

	if (key_is_pressed)
		press_key(0);

	return key_is_pressed;
}

static int __init virtual_wakeup_key_late_init(void)
{
	int ret;

	ret = bootup_force_screen_on();
	if (ret)
		pr_err("VPK System Late Init ERROR!!\n");
	else
		pr_info("VPK System Late Init Complete:%u\n", late_init_complete);

	return ret;
}

late_initcall(virtual_wakeup_key_late_init);
*/
static int __init virtual_wakeup_key_init(void)
{
	int rc = 0;

	virtkeydev = input_allocate_device();
	if (!virtkeydev) {
		pr_err("Failed to allocate virtkeydev\n");
		rc = -ENOMEM;
		goto err_alloc_dev;
	}

	input_set_capability(virtkeydev, EV_KEY, KEY_WAKEUP);

	virtkeydev->name = "virtual_wakeup_key";
	virtkeydev->phys = "virtual_wake_key/input0";

	rc = input_register_device(virtkeydev);
	if (rc) {
		pr_err("%s: input_register_device err=%d\n", __func__, rc);
		goto err_input_dev;
	}

	virtkey_input_wq = create_workqueue("virtkeyq");
	if (!virtkey_input_wq) {
		pr_err("%s: Failed to create workqueue\n", __func__);
		rc = -EFAULT;
		goto err_unregister;
	}
	INIT_DELAYED_WORK(&wakeup_key_press_work, wakeup_key_press);
	INIT_DELAYED_WORK(&wakeup_key_release_work, wakeup_key_release);

	rc = input_register_handler(&virtkey_input_handler);
	if (rc) {
		pr_err("%s: Failed to register virtkey_input_handler\n", __func__);
		goto err_wq;
	}

	return 0;

err_wq:
	destroy_workqueue(virtkey_input_wq);
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
	input_unregister_handler(&virtkey_input_handler);
	destroy_workqueue(virtkey_input_wq);
	input_unregister_device(virtkeydev);
	input_free_device(virtkeydev);
	return;
}

module_init(virtual_wakeup_key_init);
module_exit(virtual_wakeup_key_exit);
