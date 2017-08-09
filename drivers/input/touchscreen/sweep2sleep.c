#include <linux/module.h>
#include <linux/kernel.h>    
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/timed_output.h>

#define DRIVER_AUTHOR "flar2 (asegaert at gmail.com)"
#define DRIVER_DESCRIPTION "sweep2sleep driver"
#define DRIVER_VERSION "4.2"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

//sweep2sleep
#define S2S_PWRKEY_DUR          10
#define S2S_Y_MAX             	1919
#define S2S_Y_LIMIT             S2S_Y_MAX-180
#define SWEEP_RIGHT		0x01
#define SWEEP_LEFT		0x02

// 1=sweep right, 2=sweep left, 3=both
static unsigned int s2s_switch = 0;
static unsigned int vibration_timeout = 125;

static int touch_x = 0, touch_y = 0, firstx = 0;
static bool touch_x_called = false, touch_y_called = false;
static bool scr_on_touch = false, barrier[2] = {false, false};
static bool exec_count = true;
static struct input_dev * sweep2sleep_pwrdev;
static DEFINE_MUTEX(pwrkeyworklock);
static struct workqueue_struct *s2s_input_wq;
static struct delayed_work sweep2sleep_releasepwr_work;
static struct work_struct sweep2sleep_presspwr_work;
static void sweep2sleep_releasepwr(struct work_struct *work);
static void sweep2sleep_presspwr(struct work_struct *work);
static struct work_struct s2s_input_work;

/* PowerKeyReleased work func */
static void sweep2sleep_releasepwr(struct work_struct *work)
{
	input_report_key(sweep2sleep_pwrdev, KEY_SLEEP, 0);
	input_sync(sweep2sleep_pwrdev);
	mutex_unlock(&pwrkeyworklock);
}

/* PowerKeyPressed work func */
static void sweep2sleep_presspwr(struct work_struct *work)
{
	if (!mutex_trylock(&pwrkeyworklock))
                return;
	input_report_key(sweep2sleep_pwrdev, KEY_SLEEP, 1);
	input_sync(sweep2sleep_pwrdev);
	schedule_delayed_work(&sweep2sleep_releasepwr_work, msecs_to_jiffies(S2S_PWRKEY_DUR));
}

/* PowerKey trigger */
static void sweep2sleep_pwrtrigger(void) {
	if (vibration_timeout)
		machinex_vibrator(vibration_timeout);
	schedule_work(&sweep2sleep_presspwr_work);
	return;
}

/* reset on finger release */
static void sweep2sleep_reset(void) {
	exec_count = true;
	barrier[0] = false;
	barrier[1] = false;
	firstx = 0;
	scr_on_touch = false;
}

/* Sweep2sleep main function */
static void detect_sweep2sleep(int x, int y, bool st)
{
        int prevx = 0, nextx = 0;
        bool single_touch = st;

	if (firstx == 0)
		firstx = x;

	if (s2s_switch == 0)
		return;

	//left->right
	if (single_touch && firstx < 810 && (s2s_switch & SWEEP_RIGHT)) {
		scr_on_touch=true;
		prevx = firstx;
		nextx = prevx + 180;
		if ((barrier[0] == true) ||
		   ((x > prevx) &&
		    (x < nextx) &&
		    (y > S2S_Y_LIMIT))) {
			prevx = nextx;
			nextx += 200;
			barrier[0] = true;
			if ((barrier[1] == true) ||
			   ((x > prevx) &&
			    (x < nextx) &&
			    (y > S2S_Y_LIMIT))) {
				prevx = nextx;
				barrier[1] = true;
				if ((x > prevx) &&
				    (y > S2S_Y_LIMIT)) {
					if (x > (nextx + 180)) {
						if (exec_count) {
							sweep2sleep_pwrtrigger();
							exec_count = false;
						}
					}
				}
			}
		}
	//right->left
	} else if (firstx >= 180 && (s2s_switch & SWEEP_LEFT)) {
		scr_on_touch=true;
		prevx = firstx;
		nextx = prevx - 180;
		if ((barrier[0] == true) ||
		   ((x < prevx) &&
		    (x > nextx) &&
		    (y > S2S_Y_LIMIT))) {
			prevx = nextx;
			nextx -= 200;
			barrier[0] = true;
			if ((barrier[1] == true) ||
			   ((x < prevx) &&
			    (x > nextx) &&
			    (y > S2S_Y_LIMIT))) {
				prevx = nextx;
				barrier[1] = true;
				if ((x < prevx) &&
				    (y > S2S_Y_LIMIT)) {
					if (x < (nextx - 180)) {
						if (exec_count) {
							sweep2sleep_pwrtrigger();
							exec_count = false;
						}
					}
				}
			}
		}
	}
}


static void s2s_input_callback(struct work_struct *unused) {

	detect_sweep2sleep(touch_x, touch_y, true);

	return;
}

static void s2s_input_event(struct input_handle *handle, unsigned int type,
				unsigned int code, int value) {

	if (code == ABS_MT_SLOT) {
		sweep2sleep_reset();
		return;
	}

	if (code == ABS_MT_TRACKING_ID && value == -1) {
		sweep2sleep_reset();
		return;
	}

	if (code == ABS_MT_POSITION_X) {
		touch_x = value;
		touch_x_called = true;
	}

	if (code == ABS_MT_POSITION_Y) {
		touch_y = value;
		touch_y_called = true;
	}

	if (touch_x_called && touch_y_called) {
		touch_x_called = false;
		touch_y_called = false;
		queue_work(s2s_input_wq, &s2s_input_work);
	}
}

static int input_dev_filter(struct input_dev *dev) {
	if (strstr(dev->name, "synaptics_rmi4_i2c") ||
		strstr(dev->name, "sec_touchscreen")) {
		return 0;
	} else {
		return 1;
	}
}

static int s2s_input_connect(struct input_handler *handler,
				struct input_dev *dev, const struct input_device_id *id) {
	struct input_handle *handle;
	int error;

	if (input_dev_filter(dev))
		return -ENODEV;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "s2s";

	error = input_register_handle(handle);

	error = input_open_device(handle);

	return 0;

}

static void s2s_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id s2s_ids[] = {
	{ .driver_info = 1 },
	{ },
};

static struct input_handler s2s_input_handler = {
	.event		= s2s_input_event,
	.connect	= s2s_input_connect,
	.disconnect	= s2s_input_disconnect,
	.name		= "s2s_inputreq",
	.id_table	= s2s_ids,
};

#define MX_ATTR_RW(_name) \
static struct kobj_attribute _name##_attr = \
	__ATTR(_name, 0644, show_##_name, store_##_name)

static ssize_t show_sweep2sleep(struct kobject *kobj,
			   struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", s2s_switch);
}

static ssize_t store_sweep2sleep(struct kobject *kobj, 
			   struct kobj_attribute *attr, 
			   const char *buf, size_t count)
{
	int input;

	sscanf(buf, "%d\n", &input);

	if (input <= 0)
		input = 0;				
	if (input >= 3)
		input = 3;

	s2s_switch = input;			
	
	return count;
}
MX_ATTR_RW(sweep2sleep);


static ssize_t show_vibration_timeout(struct kobject *kobj,
			   struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", vibration_timeout);
}

static ssize_t store_vibration_timeout(struct kobject *kobj, 
			   struct kobj_attribute *attr, 
			   const char *buf, size_t count)
{
	int input;

	sscanf(buf, "%d\n", &input);

	if (input <= 0)
		input = 0;				
	if (input >= 10000)
		input = 10000;

	vibration_timeout = input;			
	
	return count;
}
MX_ATTR_RW(vibration_timeout);

static struct attribute *sweep2sleep_attrs[] = {
	&sweep2sleep_attr.attr,
	&vibration_timeout_attr.attr,
	NULL,
};

static struct attribute_group sweep2sleep_attr_group = {
	.attrs = sweep2sleep_attrs,
	.name = "sweep2sleep",
};

static int __init sweep2sleep_init(void)
{
	int rc = 0;

	sweep2sleep_pwrdev = input_allocate_device();
	if (!sweep2sleep_pwrdev) {
		pr_err("Failed to allocate sweep2sleep_pwrdev\n");
		rc = -ENOMEM;
		goto err_alloc_dev;
	}

	input_set_capability(sweep2sleep_pwrdev, EV_KEY, KEY_SLEEP);

	sweep2sleep_pwrdev->name = "s2s_pwrkey";
	sweep2sleep_pwrdev->phys = "s2s_pwrkey/input0";

	rc = input_register_device(sweep2sleep_pwrdev);
	if (rc) {
		pr_err("%s: input_register_device err=%d\n", __func__, rc);
		goto err_input_dev;
	}

	s2s_input_wq = create_workqueue("s2s_iwq");
	if (!s2s_input_wq) {
		pr_err("%s: Failed to create workqueue\n", __func__);
		rc = -EFAULT;
		goto err_unregister;
	}
	INIT_WORK(&s2s_input_work, s2s_input_callback);
	INIT_WORK(&sweep2sleep_presspwr_work, sweep2sleep_presspwr);
	INIT_DELAYED_WORK(&sweep2sleep_releasepwr_work, sweep2sleep_releasepwr);

	rc = input_register_handler(&s2s_input_handler);
	if (rc) {
		pr_err("%s: Failed to register s2s_input_handler\n", __func__);
		goto err_wq;
	}
	rc = sysfs_create_group(mx_kobj, &sweep2sleep_attr_group);
	if (rc) {
		pr_err("%s: sweep2sleep group failed\n", __func__);
		goto err_group;
	}

	return 0;

err_group:
	input_unregister_handler(&s2s_input_handler);
err_wq:
	destroy_workqueue(s2s_input_wq);
err_unregister:
	input_unregister_device(sweep2sleep_pwrdev);
err_input_dev:
	input_free_device(sweep2sleep_pwrdev);

err_alloc_dev:
	pr_info("%s Failed\n", __func__);

	return rc;
}

static void __exit sweep2sleep_exit(void)
{
	sysfs_remove_group(mx_kobj, &sweep2sleep_attr_group);
	input_unregister_handler(&s2s_input_handler);
	destroy_workqueue(s2s_input_wq);
	input_unregister_device(sweep2sleep_pwrdev);
	input_free_device(sweep2sleep_pwrdev);

	return;
}

module_init(sweep2sleep_init);
module_exit(sweep2sleep_exit);
