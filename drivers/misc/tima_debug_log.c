#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/types.h>

#if defined(CONFIG_D2_TIMA_LOG) 
#define	DEBUG_LOG_START		(0x88700000)
#define	SECURE_LOG_START	(0x88880000)
#define	SEC_LOG_SIZE	(1<<19)
#elif defined(CONFIG_8930_TIMA_LOG)
#define	DEBUG_LOG_START		(0x88500000)
#define	SECURE_LOG_START	(0x88680000)
#define	SEC_LOG_SIZE	(1<<19)
#else
#define	DEBUG_LOG_START	(0x88600000)
#endif

#define	DEBUG_LOG_SIZE	(1<<20)
#define	DEBUG_LOG_MAGIC	(0xaabbccdd)
#define	DEBUG_LOG_ENTRY_SIZE	128
typedef struct debug_log_entry_s
{
    uint32_t	timestamp;          /* timestamp at which log entry was made*/
    uint32_t	logger_id;          /* id is 1 for tima, 2 for lkmauth app  */
#define	DEBUG_LOG_MSG_SIZE	(DEBUG_LOG_ENTRY_SIZE - sizeof(uint32_t) - sizeof(uint32_t))
    char	log_msg[DEBUG_LOG_MSG_SIZE];      /* buffer for the entry                 */
} __attribute__ ((packed)) debug_log_entry_t;

typedef struct debug_log_header_s
{
    uint32_t	magic;              /* magic number                         */
    uint32_t	log_start_addr;     /* address at which log starts          */
    uint32_t	log_write_addr;     /* address at which next entry is written*/
    uint32_t	num_log_entries;    /* number of log entries                */
    char	padding[DEBUG_LOG_ENTRY_SIZE - 4 * sizeof(uint32_t)];
} __attribute__ ((packed)) debug_log_header_t;

#define DRIVER_DESC   "A kernel module to read tima debug log"

unsigned long *tima_debug_log_addr = 0;

#if defined(CONFIG_D2_TIMA_LOG) || defined(CONFIG_8930_TIMA_LOG)
unsigned long *tima_secure_log_addr = 0;

ssize_t tima_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	if (!strcmp(file->f_path.dentry->d_iname, "tima_secure_log")){
		if (copy_to_user(buf, tima_secure_log_addr, size))
		        return -EFAULT;
	}
	else if (copy_to_user(buf,  tima_debug_log_addr, size))
		return -EFAULT;
	return size;
}

#else
ssize_t	tima_read(struct file *filep, char __user *buf, size_t size, loff_t *offset)
{
	/* First check is to get rid of integer overflow exploits */
	if (size > DEBUG_LOG_SIZE || (*offset) + size > DEBUG_LOG_SIZE) {
		printk(KERN_ERR"Extra read\n");
		return -EINVAL;
	}

	if (copy_to_user(buf, (const char *)tima_debug_log_addr + (*offset), size)) {
		printk(KERN_ERR"Copy to user failed\n");
		return -1;
	} else {
		*offset += size;
		return size;
	}
}
#endif

static const struct file_operations tima_proc_fops = {
	.read		= tima_read,
};

/**
 *      tima_debug_log_read_init -  Initialization function for TIMA
 *
 *      It creates and initializes tima proc entry with initialized read handler 
 */
static int __init tima_debug_log_read_init(void)
{
        if (proc_create("tima_debug_log", 0644,NULL, &tima_proc_fops) == NULL) {
		printk(KERN_ERR"tima_debug_log_read_init: Error creating proc entry\n");
		goto error_return;
	}
#if defined(CONFIG_D2_TIMA_LOG) || defined(CONFIG_8930_TIMA_LOG)
        if (proc_create("tima_secure_log", 0644,NULL, &tima_proc_fops) == NULL) {
		printk(KERN_ERR"tima_secure_log_read_init: Error creating proc entry\n");
		goto error_return;
	}
#endif
        printk(KERN_INFO"tima_debug_log_read_init: Registering /proc/tima_debug_log Interface \n");

	tima_debug_log_addr = (unsigned long *)ioremap(DEBUG_LOG_START, DEBUG_LOG_SIZE);
	if (tima_debug_log_addr == NULL) {
		printk(KERN_ERR"tima_debug_log_read_init: ioremap Failed\n");
		goto ioremap_debug_failed;
	}
#if defined(CONFIG_D2_TIMA_LOG) || defined(CONFIG_8930_TIMA_LOG)
	tima_secure_log_addr = (unsigned long *)ioremap(SECURE_LOG_START, SEC_LOG_SIZE);
	if (tima_secure_log_addr == NULL) {
		printk(KERN_ERR"tima_debug_log_read_init: SECURE LOG ioremap Failed\n");
		goto ioremap_secure_failed;
	}
#endif
        return 0;

#if defined(CONFIG_D2_TIMA_LOG) || defined(CONFIG_8930_TIMA_LOG)
ioremap_secure_failed:
	remove_proc_entry("tima_secure_log", NULL);
#endif
ioremap_debug_failed:
	remove_proc_entry("tima_secure_log", NULL);

error_return:
	return -1;
}


/**
 *      tima_debug_log_read_exit -  Cleanup Code for TIMA
 *
 *      It removes /proc/tima proc entry and does the required cleanup operations 
 */
static void __exit tima_debug_log_read_exit(void)
{
        remove_proc_entry("tima_debug_log", NULL);
        printk(KERN_INFO"Deregistering /proc/tima_debug_log Interface\n");

	if(tima_debug_log_addr != NULL)
		iounmap(tima_debug_log_addr);
#if defined(CONFIG_D2_TIMA_LOG) || defined(CONFIG_8930_TIMA_LOG)
	if(tima_secure_log_addr != NULL)
		iounmap(tima_secure_log_addr);
#endif
}


module_init(tima_debug_log_read_init);
module_exit(tima_debug_log_read_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
