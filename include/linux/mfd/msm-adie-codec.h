#ifndef __LINUX_MFD_MSM_ADIE_CODEC_H
#define __LINUX_MFD_MSM_ADIE_CODEC_H

#include <uapi/linux/mfd/msm-adie-codec.h>

int adie_codec_register_codec_operations(
				const struct adie_codec_operations *codec_ops);
int adie_codec_open(struct adie_codec_dev_profile *profile,
	struct adie_codec_path **path_pptr);
int adie_codec_setpath(struct adie_codec_path *path_ptr,
	u32 freq_plan, u32 osr);
int adie_codec_proceed_stage(struct adie_codec_path *path_ptr, u32 state);
int adie_codec_close(struct adie_codec_path *path_ptr);
u32 adie_codec_freq_supported(struct adie_codec_dev_profile *profile,
							u32 requested_freq);
int adie_codec_enable_sidetone(struct adie_codec_path *rx_path_ptr, u32 enable);
int adie_codec_enable_anc(struct adie_codec_path *rx_path_ptr, u32 enable,
	struct adie_codec_anc_data *calibration_writes);
int adie_codec_set_device_digital_volume(struct adie_codec_path *path_ptr,
		u32 num_channels, u32 vol_percentage /* in percentage */);

int adie_codec_set_device_analog_volume(struct adie_codec_path *path_ptr,
		u32 num_channels, u32 volume /* in percentage */);

int adie_codec_set_master_mode(struct adie_codec_path *path_ptr, u8 master);

#endif
