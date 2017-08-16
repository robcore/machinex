#include <linux/module.h>
#include <linux/kernel.h>    
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/timed_output.h>

#define DRIVER_AUTHOR "robcore"
#define DRIVER_DESCRIPTION "virtualpowerkey(wakeup) driver"
#define DRIVER_VERSION "1.0"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPLv2");

/*Based on Sweep2Sleep by flar2 (Aaron Segaert) */

#define WTIMEOUT 5

static struct input_dev *virtkeydev;
static DEFINE_MUTEX(virtkeyworklock);
static struct workqueue_struct *virtkey_input_wq;
static struct delayed_work wakeup_key_release_work;
static struct work_struct wakeup_key_press_work;
static void wakeup_key_release(struct work_struct *work);
static void wakeup_key_press(struct work_struct *work);
static struct work_struct virtkey_input_work;

/* WakeKeyReleased work func */
static void wakeup_key_release(struct work_struct *work)
{
	input_report_key(virtkeydev, KEY_WAKEUP, 0);
	input_sync(virtkeydev);
	mutex_unlock(&virtkeyworklock);
}

/* WakeKeyPressed work func */
static void wakeup_key_press(struct work_struct *work)
{
	if (!mutex_trylock(&virtkeyworklock))
                return;
	input_report_key(virtkeydev, KEY_WAKEUP, 1);
	input_sync(virtkeydev);
	schedule_delayed_work(&wakeup_key_release_work, msecs_to_jiffies(WTIMEOUT));
}

/* PowerKey trigger */
void virt_wakeup_key_trig(void) {
	schedule_work(&wakeup_key_press_work);
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
	INIT_WORK(&wakeup_key_press_work, wakeup_key_press);
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
