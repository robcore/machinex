/*
 * WCD93xx Sound Engine
 *
 * Copyright (C) 2013, Paul Reioux
 * Copyright (C) 2016, The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/bitops.h>
#include <linux/kallsyms.h>
#include <linux/export.h>
#include <linux/mfd/wcd9xxx/core.h>
#include <linux/mfd/wcd9xxx/wcd9310_registers.h>
#include <linux/sysfs_helpers.h>

#define SOUND_CONTROL_MAJOR_VERSION	5
#define SOUND_CONTROL_MINOR_VERSION	3

extern struct snd_soc_codec *snd_engine_codec_ptr;

unsigned int snd_ctrl_enabled = 0;
unsigned int snd_ctrl_locked;
#define HARDMIN (-84)
#define HARDMAX 50
static const int snd_ctrl_min = HARDMIN;
static const int snd_ctrl_max = HARDMAX;

unsigned int tabla_read(struct snd_soc_codec *codec, unsigned int reg);
int tabla_write(struct snd_soc_codec *codec, unsigned int reg,
		unsigned int value);

#define REG_SZ	5
static unsigned int cached_regs[] = { 0, 0, 0, 0, 0 };

static unsigned int *cache_select(unsigned int reg)
{
	unsigned int *out = NULL;

	switch (reg) {
		case TABLA_A_CDC_RX1_VOL_CTL_B2_CTL:
			out = &cached_regs[0];
			break;
		case TABLA_A_CDC_RX2_VOL_CTL_B2_CTL:
			out = &cached_regs[1];
			break;
		case TABLA_A_CDC_RX5_VOL_CTL_B2_CTL:
			out = &cached_regs[2];
			break;
		case TABLA_A_CDC_TX6_VOL_CTL_GAIN:
			out = &cached_regs[3];
			break;
		case TABLA_A_CDC_TX7_VOL_CTL_GAIN:
			out = &cached_regs[4];
			break;
	}

	return out;
}

void snd_cache_write(unsigned int reg, unsigned int value)
{
	unsigned int *tmp = cache_select(reg);

	if (tmp != NULL)
		*tmp = value;
}
EXPORT_SYMBOL(snd_cache_write);

unsigned int snd_cache_read(unsigned int reg)
{
	if (cache_select(reg) != NULL)
		return *cache_select(reg);
	else
		return -ENOMEM;
}
EXPORT_SYMBOL(snd_cache_read);

int snd_reg_access(unsigned int reg)
{
	int ret = 1;

	switch (reg) {
		/* Digital Headphones/Speaker Gain */
		case TABLA_A_CDC_RX1_VOL_CTL_B2_CTL:
		case TABLA_A_CDC_RX2_VOL_CTL_B2_CTL:
		case TABLA_A_CDC_RX5_VOL_CTL_B2_CTL:
			if (snd_ctrl_enabled && snd_ctrl_locked)
				ret = 0;
			break;
		/* Incall MIC Gain */
		case TABLA_A_CDC_TX6_VOL_CTL_GAIN:
		/* Camera MIC Gain */
		case TABLA_A_CDC_TX7_VOL_CTL_GAIN:
			if (snd_ctrl_enabled && snd_ctrl_locked)
				ret = 0;
			break;
		default:
			break;
	}

	return ret;
}
EXPORT_SYMBOL(snd_reg_access);

static int show_sound_value(int val)
{
	if (val > 50)
	val -= 256;

	return val;
}

static ssize_t sound_control_enabled_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", snd_ctrl_enabled);
}

static ssize_t sound_control_enabled_store(struct kobject *kobj,
        struct kobj_attribute *attr, const char *buf, size_t count)
{
    unsigned int val;

    sscanf(buf, "%u", &val);

    if (val >= 1) {
        val = 1;
		snd_ctrl_locked = 1;
	}

	if (val == 0) {
		val = 0;
		snd_ctrl_locked = 0;
	}

    snd_ctrl_enabled = val;

    return count;
}

static unsigned int selected_reg = 0xdeadbeef;

static ssize_t speaker_gain_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",
			show_sound_value(tabla_read(snd_engine_codec_ptr,
				TABLA_A_CDC_RX5_VOL_CTL_B2_CTL)));
}

static ssize_t speaker_gain_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;
	int addval, checksum;
	int lval, rval;

	sscanf(buf, "%d", &val);

	if (!snd_ctrl_enabled || snd_engine_codec_ptr == NULL)
		return count;

	mxsanitizer(val, snd_ctrl_min, snd_ctrl_max);

	lval = val;
	rval = val;

	addval = lval + rval;
	checksum = 255 - addval;
	if (checksum > 255) {
		checksum -=256;
		lval += 256;
		rval += 256;
	}

	snd_ctrl_locked = 0;
	tabla_write(snd_engine_codec_ptr,
		TABLA_A_CDC_RX5_VOL_CTL_B2_CTL, lval);
	tabla_write(snd_engine_codec_ptr,
		TABLA_A_CDC_RX5_VOL_CTL_B2_CTL, rval);
	snd_ctrl_locked = 1;

	count = checksum;

	return count;
}

static ssize_t headphone_gain_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",
			show_sound_value(tabla_read(snd_engine_codec_ptr,
				TABLA_A_CDC_RX1_VOL_CTL_B2_CTL)));
}

static ssize_t headphone_gain_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;
	int addval, checksum;
	int lval, rval;

	sscanf(buf, "%d", &val);

	if (!snd_ctrl_enabled || snd_engine_codec_ptr == NULL)
		return count;

	mxsanitizer(val, snd_ctrl_min, snd_ctrl_max);

	lval = val;
	rval = val;

	addval = lval + rval;
	checksum = 255 - addval;
	if (checksum > 255) {
		checksum -=256;
		lval += 256;
		rval += 256;
	}

	snd_ctrl_locked = 0;
	tabla_write(snd_engine_codec_ptr,
		TABLA_A_CDC_RX1_VOL_CTL_B2_CTL, lval);
	tabla_write(snd_engine_codec_ptr,
		TABLA_A_CDC_RX2_VOL_CTL_B2_CTL, rval);
	snd_ctrl_locked = 1;

	count = checksum;

	return count;
}

static ssize_t cam_mic_gain_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",
		show_sound_value(tabla_read(snd_engine_codec_ptr,
			TABLA_A_CDC_TX6_VOL_CTL_GAIN)));
}

static ssize_t cam_mic_gain_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;
	int sum, checksum;

	sscanf(buf, "%d", &val);

	if (!snd_ctrl_enabled || snd_engine_codec_ptr == NULL)
		return count;

	mxsanitizer(val, snd_ctrl_min, snd_ctrl_max);

	checksum = 255 - val;
		if (val < 0) {
			sum = 1 + val;
			checksum = sum * -1;
			val = val + 256;
		}

	snd_ctrl_locked = 0;
	tabla_write(snd_engine_codec_ptr,
		TABLA_A_CDC_TX6_VOL_CTL_GAIN, val);
	snd_ctrl_locked = 1;

	count = checksum;

	return count;
}

static ssize_t mic_gain_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",
		show_sound_value(tabla_read(snd_engine_codec_ptr,
			TABLA_A_CDC_TX7_VOL_CTL_GAIN)));
}

static ssize_t mic_gain_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;
	int sum, checksum;

	sscanf(buf, "%d", &val);

	if (!snd_ctrl_enabled || snd_engine_codec_ptr == NULL)
		return count;

	mxsanitizer(val, snd_ctrl_min, snd_ctrl_max);

	checksum = 255 - val;
		if (val < 0) {
			sum = 1 + val;
			checksum = sum * -1;
			val = val + 256;
		}

	snd_ctrl_locked = 0;
	tabla_write(snd_engine_codec_ptr,
		TABLA_A_CDC_TX7_VOL_CTL_GAIN, val);
	snd_ctrl_locked = 1;

	count = checksum;

	return count;
}

static ssize_t sound_control_register_list_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Headphone Left: %u\n, \
						 Headphone Right: %u\n, \
						 Speaker: %u\n, \
						 In Call Mic: %u\n, \
						 Camera Mic: %u\n", \
			TABLA_A_CDC_RX1_VOL_CTL_B2_CTL, \
			TABLA_A_CDC_RX2_VOL_CTL_B2_CTL, \
			TABLA_A_CDC_RX5_VOL_CTL_B2_CTL, \
			TABLA_A_CDC_TX6_VOL_CTL_GAIN, \
			TABLA_A_CDC_TX7_VOL_CTL_GAIN);
}

static ssize_t sound_control_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Sound Control Engine: %u.%u\n",
			SOUND_CONTROL_MAJOR_VERSION,
			SOUND_CONTROL_MINOR_VERSION);
}

static struct kobj_attribute sound_control_enabled_attribute =
	__ATTR(gpl_sound_control_enabled,
		0666,
		sound_control_enabled_show,
		sound_control_enabled_store);

static struct kobj_attribute headphone_gain_attribute =
	__ATTR(gpl_headphone_gain,
		0666,
		headphone_gain_show,
		headphone_gain_store);

static struct kobj_attribute speaker_gain_attribute =
	__ATTR(gpl_speaker_gain,
		0666,
		speaker_gain_show,
		speaker_gain_store);

static struct kobj_attribute cam_mic_gain_attribute =
	__ATTR(gpl_cam_mic_gain,
		0666,
		cam_mic_gain_show,
		cam_mic_gain_store);

static struct kobj_attribute mic_gain_attribute =
	__ATTR(gpl_mic_gain,
		0666,
		mic_gain_show,
		mic_gain_store);

static struct kobj_attribute sound_control_version_attribute =
	__ATTR(gpl_sound_control_version,
		0444,
		sound_control_version_show, NULL);

static struct kobj_attribute sound_control_register_list_attribute =
	__ATTR(gpl_sound_control_register_list,
		0444,
		sound_control_register_list_show, NULL);

static struct attribute *sound_control_attrs[] =
{
	&sound_control_enabled_attribute.attr,
	&headphone_gain_attribute.attr,
	&speaker_gain_attribute.attr,
	&cam_mic_gain_attribute.attr,
	&mic_gain_attribute.attr,
	&sound_control_version_attribute.attr,
	&sound_control_register_list_attribute.attr,
	NULL,
};

static const struct attribute_group sound_control_attr_group =
{
	.attrs = sound_control_attrs,
};

static struct kobject *sound_control_kobj;

static int sound_control_init(void)
{
	int sysfs_result;
	int ret = 0;

	snd_ctrl_enabled = 0;

	sound_control_kobj =
		kobject_create_and_add("sound_control_3", kernel_kobj);

	if (!sound_control_kobj) {
		pr_err("%s sound_control_kobj create failed!\n",
			__FUNCTION__);
		ret = -ENOMEM;
		goto complete;
	}

	sysfs_result = sysfs_create_group(sound_control_kobj,
			&sound_control_attr_group);

	if (sysfs_result) {
		pr_info("%s sysfs create failed!\n", __FUNCTION__);
		kobject_put(sound_control_kobj);
		ret = -ENOMEM;
		goto complete;
	}

complete:
	return ret;
}

static void sound_control_exit(void)
{
	if (sound_control_kobj != NULL)
		kobject_put(sound_control_kobj);
}
module_init(sound_control_init);
module_exit(sound_control_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Paul Reioux <reioux@gmail.com>");
MODULE_DESCRIPTION("WCD93xx Sound Engine v4.x");

