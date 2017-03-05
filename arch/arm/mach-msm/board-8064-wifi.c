#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/skbuff.h>
#include <linux/wlan_plat.h>
#ifdef CONFIG_PARTIALRESUME
#include <linux/partialresume.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#endif
#ifdef CONFIG_BROKEN_SDIO_HACK
#include <linux/mmc/host.h>
#include <mach/board.h>
#include <linux/irq.h>
#endif
#include <mach/gpio.h>
#include <mach/apq8064-gpio.h>
#include <linux/barcode_emul.h>		// yhcha-patch
#include "board-8064.h"

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM

#define WLAN_STATIC_SCAN_BUF0		5
#define WLAN_STATIC_SCAN_BUF1		6
#define WLAN_STATIC_DHD_INFO_BUF	7
#define WLAN_SCAN_BUF_SIZE		(64 * 1024)
#define WLAN_DHD_INFO_BUF_SIZE	(16 * 1024)
#define PREALLOC_WLAN_SEC_NUM		4
#define PREALLOC_WLAN_BUF_NUM		160
#define PREALLOC_WLAN_SECTION_HEADER	24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_BUF_NUM * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_BUF_NUM * 1024)

#define DHD_SKB_HDRSIZE			336
#define DHD_SKB_1PAGE_BUFSIZE	((PAGE_SIZE*1)-DHD_SKB_HDRSIZE)
#define DHD_SKB_2PAGE_BUFSIZE	((PAGE_SIZE*2)-DHD_SKB_HDRSIZE)
#define DHD_SKB_4PAGE_BUFSIZE	((PAGE_SIZE*4)-DHD_SKB_HDRSIZE)

#define WLAN_SKB_BUF_NUM	17

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

struct wlan_mem_prealloc {
	void *mem_ptr;
	unsigned long size;
};

static struct wlan_mem_prealloc wlan_mem_array[PREALLOC_WLAN_SEC_NUM] = {
	{NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER)}
};

void *wlan_static_scan_buf0;
void *wlan_static_scan_buf1;
void *wlan_static_dhd_info_buf;

#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE)
#define ENABLE_4335BT_WAR
#endif

#ifdef ENABLE_4335BT_WAR
static int bt_off = 0;
extern int bt_is_running;
#endif /* ENABLE_4335BT_WAR */

static void *brcm_wlan_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_SEC_NUM)
		return wlan_static_skb;
	if (section == WLAN_STATIC_SCAN_BUF0)
		return wlan_static_scan_buf0;
	if (section == WLAN_STATIC_SCAN_BUF1)
		return wlan_static_scan_buf1;

	if (section == WLAN_STATIC_DHD_INFO_BUF) {
		if (size > WLAN_DHD_INFO_BUF_SIZE) {
			pr_err("request DHD_INFO size(%lu) is bigger than static size(%d).\n", size, WLAN_DHD_INFO_BUF_SIZE);
			return NULL;
		}
		return wlan_static_dhd_info_buf;
	}

	if ((section < 0) || (section > PREALLOC_WLAN_SEC_NUM))
		return NULL;

	if (wlan_mem_array[section].size < size)
		return NULL;

	return wlan_mem_array[section].mem_ptr;
}

static int brcm_init_wlan_mem(void)
{
	int i;
	int j;

	for (i = 0; i < 8; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_1PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	for (; i < 16; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_2PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_4PAGE_BUFSIZE);
	if (!wlan_static_skb[i])
		goto err_skb_alloc;

	for (i = 0 ; i < PREALLOC_WLAN_SEC_NUM ; i++) {
		wlan_mem_array[i].mem_ptr =
				kmalloc(wlan_mem_array[i].size, GFP_KERNEL);

		if (!wlan_mem_array[i].mem_ptr)
			goto err_mem_alloc;
	}

	wlan_static_scan_buf0 = kmalloc(WLAN_SCAN_BUF_SIZE, GFP_KERNEL);
	if (!wlan_static_scan_buf0)
		goto err_mem_alloc;

	wlan_static_scan_buf1 = kmalloc(WLAN_SCAN_BUF_SIZE, GFP_KERNEL);
	if (!wlan_static_scan_buf1)
		goto err_mem_alloc;

	wlan_static_dhd_info_buf = kmalloc(WLAN_DHD_INFO_BUF_SIZE, GFP_KERNEL);
	if (!wlan_static_dhd_info_buf)
		goto err_mem_alloc;

	printk(KERN_INFO"%s: WIFI MEM Allocated\n", __func__);
	return 0;

 err_mem_alloc:
	if (wlan_static_scan_buf0)
		kfree(wlan_static_scan_buf0);
	if (wlan_static_scan_buf1)
		kfree(wlan_static_scan_buf1);
	if (wlan_static_dhd_info_buf)
		kfree(wlan_static_dhd_info_buf);

	pr_err("Failed to mem_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		kfree(wlan_mem_array[j].mem_ptr);

	i = WLAN_SKB_BUF_NUM;

 err_skb_alloc:
	pr_err("Failed to skb_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		dev_kfree_skb(wlan_static_skb[j]);

	return -ENOMEM;
}
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

#define GPIO_WL_HOST_WAKE 61
#define GPIO_WL_HOST_WAKE_REV08 65

#if 0
static unsigned config_gpio_wl_reg_on[] = {
	GPIO_CFG(GPIO_WL_REG_ON, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA) };
#endif
#ifdef CONFIG_BROKEN_SDIO_HACK
static void *wifi_mmc_host;
extern void sdio_ctrl_power(struct mmc_host *host, int onoff);
#endif

static unsigned get_gpio_wl_host_wake(void)
{
	unsigned gpio_wl_host_wake;
#if defined(CONFIG_MACH_JF_ATT) || defined(CONFIG_MACH_JF_TMO)
	if (system_rev < BOARD_REV08) {
		gpio_wl_host_wake = GPIO_WL_HOST_WAKE;
	} else {
		gpio_wl_host_wake = GPIO_WL_HOST_WAKE_REV08;
	}
#elif defined(CONFIG_MACH_JF_EUR)
	gpio_wl_host_wake = GPIO_WL_HOST_WAKE_REV08;
#else /*VZW/SPR/USCC */
	if (system_rev < BOARD_REV09) {
		gpio_wl_host_wake = GPIO_WL_HOST_WAKE;
	} else {
		gpio_wl_host_wake = GPIO_WL_HOST_WAKE_REV08;
	}
#endif

	return gpio_wl_host_wake;
}

int __init brcm_wifi_init_gpio(void)
{
#if CONFIG_PARTIALRESUME
/*do fuck all here*/
#endif

	unsigned gpio_cfg = GPIO_CFG(get_gpio_wl_host_wake(), 0, GPIO_CFG_INPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA);

	if (gpio_tlmm_config(gpio_cfg, GPIO_CFG_ENABLE))
		printk(KERN_ERR "%s: Failed to configure GPIO"
			" - WL_HOST_WAKE\n", __func__);
	if (gpio_request(get_gpio_wl_host_wake(), "WL_HOST_WAKE"))
		printk(KERN_ERR "Failed to request gpio for WL_HOST_WAKE\n");
	if (gpio_direction_input(get_gpio_wl_host_wake()))
		printk(KERN_ERR "%s: WL_HOST_WAKE  "
			"failed to pull down\n", __func__);

	return 0;
}

#ifdef ENABLE_4335BT_WAR
static int brcm_wlan_power(int onoff,bool b0rev)
#else
static int brcm_wlan_power(int onoff)
#endif
{
	int ret = 0;
	printk(KERN_INFO"------------------------------------------------");
	printk(KERN_INFO"------------------------------------------------\n");
	printk(KERN_INFO"%s Enter: power %s\n", __func__, onoff ? "on" : "off");

	if (onoff) {
#ifdef ENABLE_4335BT_WAR
		if(b0rev == true && ice_gpiox_get(FPGA_GPIO_BT_EN) == 0)
		{
			bt_off = 1;
			ice_gpiox_set(FPGA_GPIO_BT_EN, 1);
			printk("[brcm_wlan_power] Bluetooth Power On.\n");
			mdelay(50);
		}
		else {
			bt_off = 0;
		}
#endif /* ENABLE_4335BT_WAR */

		/*
		if (gpio_request(GPIO_WL_REG_ON, "WL_REG_ON"))
		{
			printk("Failed to request for WL_REG_ON\n");
		}*/
		/* if (gpio_direction_output(GPIO_WL_REG_ON, 1)) { */
		if (ice_gpiox_set(FPGA_GPIO_WLAN_EN, 1)) {		// yhcha-patch
			printk(KERN_ERR "%s: WL_REG_ON  failed to pull up\n",
				__func__);
			ret =  -EIO;
		}
#ifdef CONFIG_BROKEN_SDIO_HACK
	/* Power on/off SDIO host */
	sdio_ctrl_power((struct mmc_host *)wifi_mmc_host, onoff);
	} else {
	/* Power on/off SDIO host */
	sdio_ctrl_power((struct mmc_host *)wifi_mmc_host, onoff);
#else
	} else {
#endif
		/*
		if (gpio_request(GPIO_WL_REG_ON, "WL_REG_ON"))
		{
			printk("Failed to request for WL_REG_ON\n");
		}
		*/
		/* if (gpio_direction_output(GPIO_WL_REG_ON, 0)) { */
		if (ice_gpiox_set(FPGA_GPIO_WLAN_EN, 0)) {		// yhcha-patch
			printk(KERN_ERR "%s: WL_REG_ON  failed to pull down\n",
				__func__);
			ret = -EIO;
		}
	}
#ifdef ENABLE_4335BT_WAR
	if(onoff && (bt_off == 1) && (bt_is_running == 0)) {
		mdelay(100);
		ice_gpiox_set(FPGA_GPIO_BT_EN, 0);
		printk("[brcm_wlan_power] BT_REG_OFF.\n");
	}
#endif
	return ret;
}

static int brcm_wlan_reset(int onoff)
{
  /*
	gpio_set_value(GPIO_WLAN_ENABLE,
			onoff ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
  */
	ice_gpiox_set(FPGA_GPIO_WLAN_EN,
			onoff ? 1 : 0);

	return 0;
}


static int brcm_wifi_cd; /* WIFI virtual 'card detect' status */
static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;
#ifdef CONFIG_BROKEN_SDIO_HACK
int brcm_wifi_status_register(
	void (*callback)(int card_present, void *dev_id),
	void *dev_id, void *mmc_host)
#else
int brcm_wifi_status_register(
	void (*callback)(int card_present, void *dev_id),
	void *dev_id)
#endif
{
	if (wifi_status_cb)
		return -EAGAIN;
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
#ifdef CONFIG_BROKEN_SDIO_HACK
	wifi_mmc_host = mmc_host;
#endif
	printk(KERN_INFO "%s: callback is %p, devid is %p\n",
		__func__, wifi_status_cb, dev_id);
	return 0;
}

#if 1
unsigned int brcm_wifi_status(struct device *dev)
{
	printk("%s:%d status %d\n",__func__,__LINE__,brcm_wifi_cd);
	return brcm_wifi_cd;
}
#endif

static int brcm_wlan_set_carddetect(int val)
{
	pr_debug("%s: wifi_status_cb : %p, devid : %p, val : %d\n",
		__func__, wifi_status_cb, wifi_status_cb_devid, val);
	brcm_wifi_cd = val;
	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
		pr_warning("%s: Nobody to notify\n", __func__);

	/* msleep(200); wait for carddetect */

	return 0;
}

/* Customized Locale table : OPTIONAL feature */
#define WLC_CNTRY_BUF_SZ        4

struct cntry_locales_custom {
	char iso_abbrev[WLC_CNTRY_BUF_SZ];
	char custom_locale[WLC_CNTRY_BUF_SZ];
	int  custom_locale_rev;
};

static struct cntry_locales_custom brcm_wlan_translate_custom_table[] = {
	/* Table should be filled out based
	on custom platform regulatory requirement */
	{"",   "XZ", 11},  /* Universal if Country code is unknown or empty */
	{"AE", "AE", 1},
	{"AR", "AR", 1},
	{"AT", "AT", 1},
	{"AU", "AU", 2},
	{"BE", "BE", 1},
	{"BG", "BG", 1},
	{"BN", "BN", 1},
	{"CA", "CA", 2},
	{"CH", "CH", 1},
	{"CY", "CY", 1},
	{"CZ", "CZ", 1},
	{"DE", "DE", 3},
	{"DK", "DK", 1},
	{"EE", "EE", 1},
	{"ES", "ES", 1},
	{"FI", "FI", 1},
	{"FR", "FR", 1},
	{"GB", "GB", 1},
	{"GR", "GR", 1},
	{"HR", "HR", 1},
	{"HU", "HU", 1},
	{"IE", "IE", 1},
	{"IS", "IS", 1},
	{"IT", "IT", 1},
	{"JP", "JP", 5},
	{"KR", "KR", 24},
	{"KW", "KW", 1},
	{"LI", "LI", 1},
	{"LT", "LT", 1},
	{"LU", "LU", 1},
	{"LV", "LV", 1},
	{"MA", "MA", 1},
	{"MT", "MT", 1},
	{"MX", "MX", 1},
	{"NL", "NL", 1},
	{"NO", "NO", 1},
	{"PL", "PL", 1},
	{"PT", "PT", 1},
	{"PY", "PY", 1},
	{"RO", "RO", 1},
	{"RU", "RU", 5},
	{"SE", "SE", 1},
	{"SG", "SG", 4},
	{"SI", "SI", 1},
	{"SK", "SK", 1},
	{"TR", "TR", 7},
	{"TW", "TW", 2},
	{"US", "US", 46}
};


static void *brcm_wlan_get_country_code(char *ccode)
{
	int size = ARRAY_SIZE(brcm_wlan_translate_custom_table);
	int i;

	if (!ccode)
		return NULL;

	for (i = 0; i < size; i++)
		if (strcmp(ccode,
		brcm_wlan_translate_custom_table[i].iso_abbrev) == 0)
			return &brcm_wlan_translate_custom_table[i];
	return &brcm_wlan_translate_custom_table[0];
}

static struct resource brcm_wlan_resources[] = {
	[0] = {
		.name	= "bcmdhd_wlan_irq",
		.start	= MSM_GPIO_TO_INT(GPIO_WL_HOST_WAKE),
		.end	= MSM_GPIO_TO_INT(GPIO_WL_HOST_WAKE),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_SHAREABLE
			| IORESOURCE_IRQ_HIGHLEVEL,
	},
};

#if CONFIG_PARTIALRESUME
static bool smd_partial_resume(struct partial_resume *pr)
{
	return true;
}

#define PR_INIT_STATE		0
#define PR_IN_RESUME_STATE	1
#define PR_RESUME_OK_STATE	2
#define PR_SUSPEND_OK_STATE	3

static DECLARE_COMPLETION(bcm_pk_comp);
static DECLARE_COMPLETION(bcm_wd_comp);
static int bcm_suspend = PR_INIT_STATE;
static spinlock_t bcm_lock;

/*
 * Partial Resume State Machine:
    _______
   / else [INIT]________________________
   \______/   | notify_resume           \
           [IN_RESUME]              wait_for_ready
           /       \ vote_for_suspend   /
   vote_for_resume [SUSPEND_OK]________/
           \       / vote_for_resume  /
          [RESUME_OK]                /
                   \________________/
 */

static bool bcm_wifi_process_partial_resume(int action)
{
	bool suspend = false;
	int timeout = 0;

	if ((action != WIFI_PR_NOTIFY_RESUME) && (bcm_suspend == PR_INIT_STATE))
		return suspend;

	if (action == WIFI_PR_WAIT_FOR_READY)
		timeout = wait_for_completion_timeout(&bcm_pk_comp,
						      msecs_to_jiffies(50));

	spin_lock(&bcm_lock);
	switch (action) {
	case WIFI_PR_WAIT_FOR_READY:
		suspend = (bcm_suspend == PR_SUSPEND_OK_STATE) && (timeout != 0);
		if (suspend) {
			spin_unlock(&bcm_lock);
			timeout = wait_for_completion_timeout(&bcm_wd_comp,
							msecs_to_jiffies(100));
			spin_lock(&bcm_lock);
			suspend = (timeout != 0);
		}
		bcm_suspend = PR_INIT_STATE;
		break;
	case WIFI_PR_VOTE_FOR_RESUME:
		bcm_suspend = PR_RESUME_OK_STATE;
		complete(&bcm_pk_comp);
		break;
	case WIFI_PR_VOTE_FOR_SUSPEND:
		if (bcm_suspend == PR_IN_RESUME_STATE)
			bcm_suspend = PR_SUSPEND_OK_STATE;
		complete(&bcm_pk_comp);
		break;
	case WIFI_PR_NOTIFY_RESUME:
		INIT_COMPLETION(bcm_pk_comp);
		bcm_suspend = PR_IN_RESUME_STATE;
		break;
	case WIFI_PR_INIT:
		bcm_suspend = PR_INIT_STATE;
		break;
	case WIFI_PR_WD_INIT:
		INIT_COMPLETION(bcm_wd_comp);
		break;
	case WIFI_PR_WD_COMPLETE:
		complete(&bcm_wd_comp);
		break;
	}
	spin_unlock(&bcm_lock);
	return suspend;
}

bool wlan_vote_for_suspend(void)
{
	return bcm_wifi_process_partial_resume(WIFI_PR_VOTE_FOR_SUSPEND);
}
EXPORT_SYMBOL(wlan_vote_for_suspend);

static bool bcm_wifi_partial_resume(struct partial_resume *pr)
{
	bool suspend;

	suspend = bcm_wifi_process_partial_resume(WIFI_PR_WAIT_FOR_READY);
	pr_info("%s: vote %d\n", __func__, suspend);
	return suspend;
}
#endif

static struct wifi_platform_data brcm_wlan_control = {
	.set_power	= brcm_wlan_power,
	.set_reset	= brcm_wlan_reset,
	.set_carddetect	= brcm_wlan_set_carddetect,
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	.mem_prealloc	= brcm_wlan_mem_prealloc,
#endif
	.get_country_code = brcm_wlan_get_country_code,
#if CONFIG_PARTIALRESUME
	.partial_resume = bcm_wifi_process_partial_resume,
#endif
};

static struct platform_device brcm_device_wlan = {
	.name		= "bcmdhd_wlan",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(brcm_wlan_resources),
	.resource	= brcm_wlan_resources,
	.dev		= {
		.platform_data = &brcm_wlan_control,
	},
};

int __init brcm_wlan_init(void)
{
	printk(KERN_INFO"%s: start\n", __func__);

	brcm_wlan_resources[0].start = MSM_GPIO_TO_INT(get_gpio_wl_host_wake());
	brcm_wlan_resources[0].end = MSM_GPIO_TO_INT(get_gpio_wl_host_wake());

	brcm_wifi_init_gpio();
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	brcm_init_wlan_mem();
#endif
	return platform_device_register(&brcm_device_wlan);
}

#if CONFIG_PARTIALRESUME
static struct partial_resume smd_pr = {
	.irq = 122,
	.partial_resume = smd_partial_resume,
};


static struct partial_resume mpm_pr = {
	.irq = 52,
	.partial_resume = smd_partial_resume,
};

static struct partial_resume wlan_pr = {
	.partial_resume = bcm_wifi_partial_resume,
};

int __init wlan_partial_resume_init(void)
{
	int rc;

	spin_lock_init(&bcm_lock); /* Setup partial resume */
	complete(&bcm_wd_comp);    /* Prepare for case when WD is not set */
	wlan_pr.irq = brcm_device_wlan.resource->start;
	rc = register_partial_resume(&wlan_pr);
	pr_debug("%s: after registering %pF: %d\n", __func__,
		 wlan_pr.partial_resume, rc);
	rc = register_partial_resume(&mpm_pr);
	rc = register_partial_resume(&smd_pr);
	pr_debug("%s: after registering %pF: %d\n", __func__,
		 smd_pr.partial_resume, rc);
	return rc;
}

late_initcall(wlan_partial_resume_init);
#endif