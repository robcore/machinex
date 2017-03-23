/*
 * drivers/uio/uio.c
 *
 * Copyright(C) 2005, Benedikt Spranger <b.spranger@linutronix.de>
 * Copyright(C) 2005, Thomas Gleixner <tglx@linutronix.de>
 * Copyright(C) 2006, Hans J. Koch <hjk@hansjkoch.de>
 * Copyright(C) 2006, Greg Kroah-Hartman <greg@kroah.com>
 *
 * Userspace IO
 *
 * Base Functions
 *
 * Licensed under the GPLv2 only.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/cdev.h>
#include <linux/uio_driver.h>

#define UIO_MAX_DEVICES		(1U << MINORBITS)

struct uio_device {
	struct module		*owner;
	struct device		*dev;
	int			minor;
	atomic_t		event;
	struct fasync_struct	*async_queue;
	wait_queue_head_t	wait;
	struct uio_info		*info;
	struct kobject		*map_dir;
	struct kobject		*portio_dir;
};

static int uio_major;
static struct cdev *uio_cdev;
static DEFINE_IDR(uio_idr);
static const struct file_operations uio_fops;

/* Protect idr accesses */
static DEFINE_MUTEX(minor_lock);

/*
 * attributes
 */

struct uio_map {
	struct kobject kobj;
	struct uio_mem *mem;
};
#define to_map(map) container_of(map, struct uio_map, kobj)

static ssize_t map_name_show(struct uio_mem *mem, char *buf)
{
	if (unlikely(!mem->name))
		mem->name = "";

	return sprintf(buf, "%s\n", mem->name);
}

static ssize_t map_addr_show(struct uio_mem *mem, char *buf)
{
	return sprintf(buf, "0x%llx\n", (unsigned long long)mem->addr);
}

static ssize_t map_size_show(struct uio_mem *mem, char *buf)
{
	return sprintf(buf, "0x%lx\n", mem->size);
}

static ssize_t map_offset_show(struct uio_mem *mem, char *buf)
{
	return sprintf(buf, "0x%llx\n", (unsigned long long)mem->addr & ~PAGE_MASK);
}

struct map_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(struct uio_mem *, char *);
	ssize_t (*store)(struct uio_mem *, const char *, size_t);
};

static struct map_sysfs_entry name_attribute =
	__ATTR(name, S_IRUGO, map_name_show, NULL);
static struct map_sysfs_entry addr_attribute =
	__ATTR(addr, S_IRUGO, map_addr_show, NULL);
static struct map_sysfs_entry size_attribute =
	__ATTR(size, S_IRUGO, map_size_show, NULL);
static struct map_sysfs_entry offset_attribute =
	__ATTR(offset, S_IRUGO, map_offset_show, NULL);

static struct attribute *attrs[] = {
	&name_attribute.attr,
	&addr_attribute.attr,
	&size_attribute.attr,
	&offset_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static void map_release(struct kobject *kobj)
{
	struct uio_map *map = to_map(kobj);
	kfree(map);
}

static ssize_t map_type_show(struct kobject *kobj, struct attribute *attr,
			     char *buf)
{
	struct uio_map *map = to_map(kobj);
	struct uio_mem *mem = map->mem;
	struct map_sysfs_entry *entry;

	entry = container_of(attr, struct map_sysfs_entry, attr);

	if (!entry->show)
		return -EIO;

	return entry->show(mem, buf);
}

static const struct sysfs_ops map_sysfs_ops = {
	.show = map_type_show,
};

static struct kobj_type map_attr_type = {
	.release	= map_release,
	.sysfs_ops	= &map_sysfs_ops,
	.default_attrs	= attrs,
};

struct uio_portio {
	struct kobject kobj;
	struct uio_port *port;
};
#define to_portio(portio) container_of(portio, struct uio_portio, kobj)

static ssize_t portio_name_show(struct uio_port *port, char *buf)
{
	if (unlikely(!port->name))
		port->name = "";

	return sprintf(buf, "%s\n", port->name);
}

static ssize_t portio_start_show(struct uio_port *port, char *buf)
{
	return sprintf(buf, "0x%lx\n", port->start);
}

static ssize_t portio_size_show(struct uio_port *port, char *buf)
{
	return sprintf(buf, "0x%lx\n", port->size);
}

static ssize_t portio_porttype_show(struct uio_port *port, char *buf)
{
	const char *porttypes[] = {"none", "x86", "gpio", "other"};

	if ((port->porttype < 0) || (port->porttype > UIO_PORT_OTHER))
		return -EINVAL;

	return sprintf(buf, "port_%s\n", porttypes[port->porttype]);
}

struct portio_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(struct uio_port *, char *);
	ssize_t (*store)(struct uio_port *, const char *, size_t);
};

static struct portio_sysfs_entry portio_name_attribute =
	__ATTR(name, S_IRUGO, portio_name_show, NULL);
static struct portio_sysfs_entry portio_start_attribute =
	__ATTR(start, S_IRUGO, portio_start_show, NULL);
static struct portio_sysfs_entry portio_size_attribute =
	__ATTR(size, S_IRUGO, portio_size_show, NULL);
static struct portio_sysfs_entry portio_porttype_attribute =
	__ATTR(porttype, S_IRUGO, portio_porttype_show, NULL);

static struct attribute *portio_attrs[] = {
	&portio_name_attribute.attr,
	&portio_start_attribute.attr,
	&portio_size_attribute.attr,
	&portio_porttype_attribute.attr,
	NULL,
};

static void portio_release(struct kobject *kobj)
{
	struct uio_portio *portio = to_portio(kobj);
	kfree(portio);
}

static ssize_t portio_type_show(struct kobject *kobj, struct attribute *attr,
			     char *buf)
{
	struct uio_portio *portio = to_portio(kobj);
	struct uio_port *port = portio->port;
	struct portio_sysfs_entry *entry;

	entry = container_of(attr, struct portio_sysfs_entry, attr);

	if (!entry->show)
		return -EIO;

	return entry->show(port, buf);
}

static const struct sysfs_ops portio_sysfs_ops = {
	.show = portio_type_show,
};

static struct kobj_type portio_attr_type = {
	.release	= portio_release,
	.sysfs_ops	= &portio_sysfs_ops,
	.default_attrs	= portio_attrs,
};

static ssize_t show_name(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct uio_device *idev = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", idev->info->name);
}

static ssize_t show_version(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct uio_device *idev = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", idev->info->version);
}

static ssize_t show_event(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct uio_device *idev = dev_get_drvdata(dev);
	return sprintf(buf, "%u\n", (unsigned int)atomic_read(&idev->event));
}

static struct device_attribute uio_class_attributes[] = {
	__ATTR(name, S_IRUGO, show_name, NULL),
	__ATTR(version, S_IRUGO, show_version, NULL),
	__ATTR(event, S_IRUGO, show_event, NULL),
	{}
};

/* UIO class infrastructure */
static struct class uio_class = {
	.name = "uio",
	.dev_attrs = uio_class_attributes,
};

/*
 * device functions
 */
static int uio_dev_add_attributes(struct uio_device *idev)
{
	int ret;
	int mi, pi;
	int map_found = 0;
	int portio_found = 0;
	struct uio_mem *mem;
	struct uio_map *map;
	struct uio_port *port;
	struct uio_portio *portio;

	for (mi = 0; mi < MAX_UIO_MAPS; mi++) {
		mem = &idev->info->mem[mi];
		if (mem->size == 0)
			break;
		if (!map_found) {
			map_found = 1;
			idev->map_dir = kobject_create_and_add("maps",
							&idev->dev->kobj);
			if (!idev->map_dir)
				goto err_map;
		}
		map = kzalloc(sizeof(*map), GFP_KERNEL);
		if (!map)
			goto err_map;
		kobject_init(&map->kobj, &map_attr_type);
		map->mem = mem;
		mem->map = map;
		ret = kobject_add(&map->kobj, idev->map_dir, "map%d", mi);
		if (ret)
			goto err_map;
		ret = kobject_uevent(&map->kobj, KOBJ_ADD);
		if (ret)
			goto err_map;
	}

	for (pi = 0; pi < MAX_UIO_PORT_REGIONS; pi++) {
		port = &idev->info->port[pi];
		if (port->size == 0)
			break;
		if (!portio_found) {
			portio_found = 1;
			idev->portio_dir = kobject_create_and_add("portio",
							&idev->dev->kobj);
			if (!idev->portio_dir)
				goto err_portio;
		}
		portio = kzalloc(sizeof(*portio), GFP_KERNEL);
		if (!portio)
			goto err_portio;
		kobject_init(&portio->kobj, &portio_attr_type);
		portio->port = port;
		port->portio = portio;
		ret = kobject_add(&portio->kobj, idev->portio_dir,
							"port%d", pi);
		if (ret)
			goto err_portio;
		ret = kobject_uevent(&portio->kobj, KOBJ_ADD);
		if (ret)
			goto err_portio;
	}

	return 0;

err_portio:
	for (pi--; pi >= 0; pi--) {
		port = &idev->info->port[pi];
		portio = port->portio;
		kobject_put(&portio->kobj);
	}
	kobject_put(idev->portio_dir);
err_map:
	for (mi--; mi>=0; mi--) {
		mem = &idev->info->mem[mi];
		map = mem->map;
		kobject_put(&map->kobj);
	}
	kobject_put(idev->map_dir);
	dev_err(idev->dev, "error creating sysfs files (%d)\n", ret);
	return ret;
}

static void uio_dev_del_attributes(struct uio_device *idev)
{
	int i;
	struct uio_mem *mem;
	struct uio_port *port;

	for (i = 0; i < MAX_UIO_MAPS; i++) {
		mem = &idev->info->mem[i];
		if (mem->size == 0)
			break;
		kobject_put(&mem->map->kobj);
	}
	kobject_put(idev->map_dir);

	for (i = 0; i < MAX_UIO_PORT_REGIONS; i++) {
		port = &idev->info->port[i];
		if (port->size == 0)
			break;
		kobject_put(&port->portio->kobj);
	}
	kobject_put(idev->portio_dir);
}

static int uio_get_minor(struct uio_device *idev)
{
	int retval = -ENOMEM;
	int id;

	mutex_lock(&minor_lock);
	if (idr_pre_get(&uio_idr, GFP_KERNEL) == 0)
		goto exit;

	retval = idr_get_new(&uio_idr, idev, &id);
	if (retval < 0) {
		if (retval == -EAGAIN)
			retval = -ENOMEM;
		goto exit;
	}
	if (id < UIO_MAX_DEVICES) {
		idev->minor = id;
	} else {
		dev_err(idev->dev, "too many uio devices\n");
		retval = -EINVAL;
		idr_remove(&uio_idr, id);
	}
exit:
	mutex_unlock(&minor_lock);
	return retval;
}

static void uio_free_minor(struct uio_device *idev)
{
	mutex_lock(&minor_lock);
	idr_remove(&uio_idr, idev->minor);
	mutex_unlock(&minor_lock);
}

/**
 * uio_event_notify - trigger an interrupt event
 * @info: UIO device capabilities
 */
void uio_event_notify(struct uio_info *info)
{
	struct uio_device *idev = info->uio_dev;

	atomic_inc(&idev->event);
	wake_up_interruptible(&idev->wait);
	kill_fasync(&idev->async_queue, SIGIO, POLL_IN);
}
EXPORT_SYMBOL_GPL(uio_event_notify);

/**
 * uio_interrupt - hardware interrupt handler
 * @irq: IRQ number, can be UIO_IRQ_CYCLIC for cyclic timer
 * @dev_id: Pointer to the devices uio_device structure
 */
static irqreturn_t uio_interrupt(int irq, void *dev_id)
{
	struct uio_device *idev = (struct uio_device *)dev_id;
	irqreturn_t ret = idev->info->handler(irq, idev->info);

	if (ret == IRQ_HANDLED)
		uio_event_notify(idev->info);

	return ret;
}

struct uio_listener {
	struct uio_device *dev;
	s32 event_count;
};

static int uio_open(struct inode *inode, struct file *filep)
{
	struct uio_device *idev;
	struct uio_listener *listener;
	int ret = 0;

	mutex_lock(&minor_lock);
	idev = idr_find(&uio_idr, iminor(inode));
	mutex_unlock(&minor_lock);
	if (!idev) {
		ret = -ENODEV;
		goto out;
	}

	if (!try_module_get(idev->owner)) {
		ret = -ENODEV;
		goto out;
	}

	listener = kmalloc(sizeof(*listener), GFP_KERNEL);
	if (!listener) {
		ret = -ENOMEM;
		goto err_alloc_listener;
	}

	listener->dev = idev;
	listener->event_count = atomic_read(&idev->event);
	filep->private_data = listener;

	if (idev->info->open) {
		ret = idev->info->open(idev->info, inode);
		if (ret)
			goto err_infoopen;
	}
	return 0;

err_infoopen:
	kfree(listener);

err_alloc_listener:
	module_put(idev->owner);

out:
	return ret;
}

static int uio_fasync(int fd, struct file *filep, int on)
{
	struct uio_listener *listener = filep->private_data;
	struct uio_device *idev = listener->dev;

	return fasync_helper(fd, filep, on, &idev->async_queue);
}

static int uio_release(struct inode *inode, struct file *filep)
{
	int ret = 0;
	struct uio_listener *listener = filep->private_data;
	struct uio_device *idev = listener->dev;

	if (idev->info->release)
		ret = idev->info->release(idev->info, inode);

	module_put(idev->owner);
	kfree(listener);
	return ret;
}

static unsigned int uio_poll(struct file *filep, poll_table *wait)
{
	struct uio_listener *listener = filep->private_data;
	struct uio_device *idev = listener->dev;

	if (!idev->info->irq)
		return -EIO;

	poll_wait(filep, &idev->wait, wait);
	if (listener->event_count != atomic_read(&idev->event))
		return POLLIN | POLLRDNORM;
	return 0;
}

static ssize_t uio_read(struct file *filep, char __user *buf,
			size_t count, loff_t *ppos)
{
	struct uio_listener *listener = filep->private_data;
	struct uio_device *idev = listener->dev;
	DECLARE_WAITQUEUE(wait, current);
	ssize_t retval;
	s32 event_count;

	if (!idev->info->irq)
		return -EIO;

	if (count != sizeof(s32))
		return -EINVAL;

	add_wait_queue(&idev->wait, &wait);

	do {
		set_current_state(TASK_INTERRUPTIBLE);

		event_count = atomic_read(&idev->event);
		if (event_count != listener->event_count) {
			if (copy_to_user(buf, &event_count, count))
				retval = -EFAULT;
			else {
				listener->event_count = event_count;
				retval = count;
			}
			break;
		}

		if (filep->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			break;
		}
		schedule();
	} while (1);

	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&idev->wait, &wait);

	return retval;
}

static ssize_t uio_write(struct file *filep, const char __user *buf,
			size_t count, loff_t *ppos)
{
	struct uio_listener *listener = filep->private_data;
	struct uio_device *idev = listener->dev;
	ssize_t retval;
	s32 irq_on;

	if (!idev->info->irq)
		return -EIO;

	if (count != sizeof(s32))
		return -EINVAL;

	if (!idev->info->irqcontrol)
		return -ENOSYS;

	if (copy_from_user(&irq_on, buf, count))
		return -EFAULT;

	retval = idev->info->irqcontrol(idev->info, irq_on);

	return retval ? retval : sizeof(s32);
}

static int uio_find_mem_index(struct vm_area_struct *vma)
{
	struct uio_device *idev = vma->vm_private_data;

	if (vma->vm_pgoff < MAX_UIO_MAPS) {
		if (idev->info->mem[vma->vm_pgoff].size == 0)
			return -1;
		return (int)vma->vm_pgoff;
	}
	return -1;
}

static int uio_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct uio_device *idev = vma->vm_private_data;
	struct page *page;
	unsigned long offset;

	int mi = uio_find_mem_index(vma);
	if (mi < 0)
		return VM_FAULT_SIGBUS;

	/*
	 * We need to subtract mi because userspace uses offset = N*PAGE_SIZE
	 * to use mem[N].
	 */
	offset = (vmf->pgoff - mi) << PAGE_SHIFT;

	if (idev->info->mem[mi].memtype == UIO_MEM_LOGICAL)
		page = virt_to_page(idev->info->mem[mi].addr + offset);
	else
		page = vmalloc_to_page((void *)(unsigned long)idev->info->mem[mi].addr + offset);
	get_page(page);
	vmf->page = page;
	return 0;
}

static const struct vm_operations_struct uio_logical_vm_ops = {
	.fault = uio_vma_fault,
};

static int uio_mmap_logical(struct vm_area_struct *vma)
{
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_ops = &uio_logical_vm_ops;
	return 0;
}

static const struct vm_operations_struct uio_physical_vm_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys,
#endif
};

static int uio_mmap_physical(struct vm_area_struct *vma)
{
	struct uio_device *idev = vma->vm_private_data;
	int mi = uio_find_mem_index(vma);
	struct uio_mem *mem;
	if (mi < 0)
		return -EINVAL;
	mem = idev->info->mem + mi;

	if (vma->vm_end - vma->vm_start > mem->size)
		return -EINVAL;

	vma->vm_ops = &uio_physical_vm_ops;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/*
	 * We cannot use the vm_iomap_memory() helper here,
	 * because vma->vm_pgoff is the map index we looked
	 * up above in uio_find_mem_index(), rather than an
	 * actual page offset into the mmap.
	 *
	 * So we just do the physical mmap without a page
	 * offset.
	 */

	return remap_pfn_range(vma,
			       vma->vm_start,
			       mem->addr >> PAGE_SHIFT,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot);
}

static int uio_mmap(struct file *filep, struct vm_area_struct *vma)
{
	struct uio_listener *listener = filep->private_data;
	struct uio_device *idev = listener->dev;
	int mi;
	unsigned long requested_pages, actual_pages;
	int ret = 0;

	if (vma->vm_end < vma->vm_start)
		return -EINVAL;

	vma->vm_private_data = idev;

	mi = uio_find_mem_index(vma);
	if (mi < 0)
		return -EINVAL;

	requested_pages = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
	actual_pages = ((idev->info->mem[mi].addr & ~PAGE_MASK)
			+ idev->info->mem[mi].size + PAGE_SIZE -1) >> PAGE_SHIFT;
	if (requested_pages > actual_pages)
		return -EINVAL;

	if (idev->info->mmap) {
		ret = idev->info->mmap(idev->info, vma);
		return ret;
	}

	switch (idev->info->mem[mi].memtype) {
		case UIO_MEM_PHYS:
			return uio_mmap_physical(vma);
		case UIO_MEM_LOGICAL:
		case UIO_MEM_VIRTUAL:
			return uio_mmap_logical(vma);
		default:
			return -EINVAL;
	}
}

static const struct file_operations uio_fops = {
	.owner		= THIS_MODULE,
	.open		= uio_open,
	.release	= uio_release,
	.read		= uio_read,
	.write		= uio_write,
	.mmap		= uio_mmap,
	.poll		= uio_poll,
	.fasync		= uio_fasync,
	.llseek		= noop_llseek,
};

static int uio_major_init(void)
{
	static const char name[] = "uio";
	struct cdev *cdev = NULL;
	dev_t uio_dev = 0;
	int result;

	result = alloc_chrdev_region(&uio_dev, 0, UIO_MAX_DEVICES, name);
	if (result)
		goto out;

	result = -ENOMEM;
	cdev = cdev_alloc();
	if (!cdev)
		goto out_unregister;

	cdev->owner = THIS_MODULE;
	cdev->ops = &uio_fops;
	kobject_set_name(&cdev->kobj, "%s", name);

	result = cdev_add(cdev, uio_dev, UIO_MAX_DEVICES);
	if (result)
		goto out_put;

	uio_major = MAJOR(uio_dev);
	uio_cdev = cdev;
	return 0;
out_put:
	kobject_put(&cdev->kobj);
out_unregister:
	unregister_chrdev_region(uio_dev, UIO_MAX_DEVICES);
out:
	return result;
}

static void uio_major_cleanup(void)
{
	unregister_chrdev_region(MKDEV(uio_major, 0), UIO_MAX_DEVICES);
	cdev_del(uio_cdev);
}

static int init_uio_class(void)
{
	int ret;

	/* This is the first time in here, set everything up properly */
	ret = uio_major_init();
	if (ret)
		goto exit;

	ret = class_register(&uio_class);
	if (ret) {
		printk(KERN_ERR "class_register failed for uio\n");
		goto err_class_register;
	}
	return 0;

err_class_register:
	uio_major_cleanup();
exit:
	return ret;
}

static void release_uio_class(void)
{
	class_unregister(&uio_class);
	uio_major_cleanup();
}

/**
 * uio_register_device - register a new userspace IO device
 * @owner:	module that creates the new device
 * @parent:	parent device
 * @info:	UIO device capabilities
 *
 * returns zero on success or a negative error code.
 */
int __uio_register_device(struct module *owner,
			  struct device *parent,
			  struct uio_info *info)
{
	struct uio_device *idev;
	int ret = 0;

	if (!parent || !info || !info->name || !info->version)
		return -EINVAL;

	info->uio_dev = NULL;

	idev = kzalloc(sizeof(*idev), GFP_KERNEL);
	if (!idev) {
		ret = -ENOMEM;
		goto err_kzalloc;
	}

	idev->owner = owner;
	idev->info = info;
	init_waitqueue_head(&idev->wait);
	atomic_set(&idev->event, 0);

	ret = uio_get_minor(idev);
	if (ret)
		goto err_get_minor;

	idev->dev = device_create(&uio_class, parent,
				  MKDEV(uio_major, idev->minor), idev,
				  "uio%d", idev->minor);
	if (IS_ERR(idev->dev)) {
		printk(KERN_ERR "UIO: device register failed\n");
		ret = PTR_ERR(idev->dev);
		goto err_device_create;
	}

	ret = uio_dev_add_attributes(idev);
	if (ret)
		goto err_uio_dev_add_attributes;

	info->uio_dev = idev;

	if (info->irq && (info->irq != UIO_IRQ_CUSTOM)) {
		ret = request_irq(info->irq, uio_interrupt,
				  info->irq_flags, info->name, idev);
		if (ret)
			goto err_request_irq;
	}

	return 0;

err_request_irq:
	uio_dev_del_attributes(idev);
err_uio_dev_add_attributes:
	device_destroy(&uio_class, MKDEV(uio_major, idev->minor));
err_device_create:
	uio_free_minor(idev);
err_get_minor:
	kfree(idev);
err_kzalloc:
	return ret;
}
EXPORT_SYMBOL_GPL(__uio_register_device);

/**
 * uio_unregister_device - unregister a industrial IO device
 * @info:	UIO device capabilities
 *
 */
void uio_unregister_device(struct uio_info *info)
{
	struct uio_device *idev;

	if (!info || !info->uio_dev)
		return;

	idev = info->uio_dev;

	uio_free_minor(idev);

	if (info->irq && (info->irq != UIO_IRQ_CUSTOM))
		free_irq(info->irq, idev);

	uio_dev_del_attributes(idev);

	device_destroy(&uio_class, MKDEV(uio_major, idev->minor));
	kfree(idev);

	return;
}
EXPORT_SYMBOL_GPL(uio_unregister_device);

static int __init uio_init(void)
{
	return init_uio_class();
}

static void __exit uio_exit(void)
{
	release_uio_class();
}

module_init(uio_init)
module_exit(uio_exit)
MODULE_LICENSE("GPL v2");
