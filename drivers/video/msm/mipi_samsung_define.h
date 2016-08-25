#ifndef __MIPI_S6EVR02_PARAM_H__
#define __MIPI_S6EVR02_PARAM_H__

#include "msm_fb.h"
#include "msm_fb_panel.h"
#include "mipi_dsi.h"

#define GAMMA_PARAM_SIZE	34
#define ACL_PARAM_SIZE	ARRAY_SIZE(acl_cutoff_33)
#define ELVSS_PARAM_SIZE	ARRAY_SIZE(elvss_control_set_20)

#define MTP_REGISTER	0xC8
#define MTP_DATA_SIZE 33
#define ELVSS_DATA_SIZE 3

#define LUMINANCE_MAX 35
#define GAMMA_SET_MAX 33
#define BIT_SHIFT 14

#define MAX_GRADATION		300
#define PANEL_ID_MAX		3

struct dsi_cmd_desc_LCD {
	int lux;
	char strID[8];
	struct dsi_cmd_desc *cmd;
};

enum mipi_samsung_cmd_list {
#ifdef CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT
	PANEL_READY_TO_ON_FAST,
#endif
	PANEL_READY_TO_READ_MTP,
	PANEL_READY_TO_ON,
	PANEL_READY_TO_ON2,
	PANEL_READY_TO_OFF,
	PANEL_ON,
	PANEL_OFF,
	PANEL_LATE_ON,
	PANEL_EARLY_OFF,
	PANEL_GAMMA_UPDATE,
	PANEL_ELVSS_UPDATE,
	PANEL_ACL_ON,
	PANEL_ACL_OFF,
	PANEL_ACL_UPDATE,
	PANEL_AID_CTRL,
	MTP_READ_ENABLE,
	PANEL_BRIGHT_CTRL,
};

enum {
	MIPI_SUSPEND_STATE,
	MIPI_RESUME_STATE,
};

enum gamma_mode_list {
	GAMMA_2_2 = 0,
	GAMMA_1_9 = 1,
	GAMMA_SMART = 2,
};

enum {
	GAMMA_20CD,
	GAMMA_30CD,
	GAMMA_40CD,
	GAMMA_50CD,
	GAMMA_60CD,
	GAMMA_70CD,
	GAMMA_80CD,
	GAMMA_90CD,
	GAMMA_100CD,
	GAMMA_102CD,
	GAMMA_104CD,
	GAMMA_106CD,
	GAMMA_108CD,
	GAMMA_110CD,
	GAMMA_120CD,
	GAMMA_130CD,
	GAMMA_140CD,
	GAMMA_150CD,
	GAMMA_160CD,
	GAMMA_170CD,
	GAMMA_180CD,
	GAMMA_182CD,
	GAMMA_184CD,
	GAMMA_186CD,
	GAMMA_188CD,
	GAMMA_190CD,
	GAMMA_200CD,
	GAMMA_210CD,
	GAMMA_220CD,
	GAMMA_230CD,
	GAMMA_240CD,
	GAMMA_250CD,
	GAMMA_300CD,
	GAMMA_MAX
};

enum {
	ELVSS_STATUS_20,
	ELVSS_STATUS_30,
	ELVSS_STATUS_40,
	ELVSS_STATUS_50,
	ELVSS_STATUS_60,
	ELVSS_STATUS_70,
	ELVSS_STATUS_80,
	ELVSS_STATUS_90,
	ELVSS_STATUS_100,
	ELVSS_STATUS_110,
	ELVSS_STATUS_120,
	ELVSS_STATUS_130,
	ELVSS_STATUS_140,
	ELVSS_STATUS_150,
	ELVSS_STATUS_160,
	ELVSS_STATUS_170,
	ELVSS_STATUS_180,
	ELVSS_STATUS_190,
	ELVSS_STATUS_200,
	ELVSS_STATUS_210,
	ELVSS_STATUS_220,
	ELVSS_STATUS_230,
	ELVSS_STATUS_240,
	ELVSS_STATUS_250,
	ELVSS_STATUS_300,
	ELVSS_STATUS_MAX
};

enum {
	ACL_STATUS_0P = 0,
	ACL_STATUS_33P,
	ACL_STATUS_40P,
	ACL_STATUS_50P,
	ACL_STATUS_MAX
};

enum {
	LDI_TYPE_MAGNA,
	LDI_TYPE_LSI,
	LDI_TYPE_MAX
};

enum {
	CI_RED      = 0,
	CI_GREEN    = 1,
	CI_BLUE     = 2,
	CI_MAX      = 3,
};


enum {
	IV_VT ,
	IV_3 ,
	IV_11 ,
	IV_23 ,
	IV_35  ,
	IV_51,
	IV_87 ,
	IV_151,
	IV_203,
	IV_255,
	IV_MAX,
	IV_TABLE_MAX,
};


enum {
	AD_IVT,
	AD_IV3,
	AD_IV11,
	AD_IV23 ,
	AD_IV35,
	AD_IV51 ,
	AD_IV87  ,
	AD_IV151 ,
	AD_IV203  ,
	AD_IV255  ,
	AD_IVMAX  ,
};


enum {
	G_21,
	G_213,
	G_215,
	G_218,
	G_22,
	G_221,
	G_222,
	G_223,
	G_224,
	G_225,
	G_MAX,
};


struct str_voltage_entry {
	u32 v[CI_MAX];
};


struct str_table_info {
	/* et : start gray value */
	u8 st;
	/* end gray value, st + count */
	u8 et;
	u8 count;
	const u8 *offset_table;
	/* rv : ratio value */
	u32 rv;
};


struct str_flookup_table {
	u16 entry;
	u16 count;
};


struct str_smart_dim {
	u8 panelid[PANEL_ID_MAX];
	s16 mtp[CI_MAX][IV_MAX];
	struct str_voltage_entry ve[256];
	const u8 *default_gamma;
	struct str_table_info t_info[IV_TABLE_MAX];
	const struct str_flookup_table *flooktbl;
	const u32 *g22_tbl;
	const u32 *gamma_table[G_MAX];
	const u32 *g300_gra_tbl;
	u32 adjust_volt[CI_MAX][AD_IVMAX];
};

struct rgb_offset_info {
	unsigned int	candela_idx;
	unsigned int	gray;
	unsigned int	rgb;
	int		offset;
};

struct gamma_table {
	char *table;
	int table_cnt;
	int data_size;
};

struct display_status {
	unsigned char acl_on;
	unsigned char gamma_mode; /* 1: 1.9 gamma, 0: 2.2 gamma */
	unsigned char is_smart_dim_loaded;
	unsigned char auto_brightness;
};

struct mipi_samsung_driver_data {
	struct dsi_buf samsung_tx_buf;
	struct dsi_buf samsung_rx_buf;
	struct msm_panel_common_pdata *mipi_samsung_disp_pdata;
	struct mipi_panel_data *mpd;
	struct display_status dstat;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_LCD_CLASS_DEVICE)
	struct platform_device *msm_pdev;
#endif
#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
	boolean esd_refresh;
#endif
};

struct cmd_set {
	struct dsi_cmd_desc *cmd;
	int size;
};

struct illuminance_table {
	int lux;
	char b3[3];
	char aor[2];
	char gamma_setting[34];
} __packed;

struct SMART_DIM {
	/* Because of AID funtion, below members are added*/
	int lux_table_max;
	int *plux_table;
	struct illuminance_table gen_table[35];

	int brightness_level;
	int ldi_revision;
} __packed;

struct mipi_panel_data {
	const char panel_name[20];
	struct cmd_set ready_to_on_lsi;
	struct cmd_set ready_to_on2_lsi;
	struct cmd_set ready_to_read_mtp;
	struct cmd_set ready_to_on_magna;
	struct cmd_set ready_to_on2_magna;
	struct cmd_set ready_to_off;
	struct cmd_set on;
	struct cmd_set off;
	struct cmd_set late_on;
	struct cmd_set early_off;
	struct cmd_set gamma_update;
	struct cmd_set elvss_update;
	struct cmd_set mtp_read_enable;
	struct cmd_set acl_update;
	struct cmd_set acl_on;
	struct cmd_set acl_off;
	struct cmd_set aid_ctrl;
	boolean ldi_acl_stat;
	struct str_smart_dim smart;
	signed char lcd_current_cd_idx;
	unsigned char lcd_mtp_data[MTP_DATA_SIZE] ;
	unsigned char lcd_elvss_data[ELVSS_DATA_SIZE];
	unsigned char *gamma_smartdim;
	unsigned char *gamma_initial;

	int *lux_table;
	int lux_table_max_cnt;
	struct SMART_DIM smart_s6e8aa0x01;
	struct cmd_set backlight_ctrl_magna;
	struct cmd_set backlight_ctrl_lsi;

	unsigned int manufacture_id;

	int (*set_acl)(int bl_level);
	int (*set_elvss)(int bl_level, int lcd_type);
#ifdef CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT
	int (*set_elvss_4_8)(int bl_level);
	int (*prepare_brightness_control_cmd_array)(struct illuminance_table *bright_table, int bl_level, int lcd_type);
	void  (*prepare_fast_cmd_array)(int lcd_type);

#endif
	struct mipi_samsung_driver_data* msd;
};

#endif /* __MIPI_S6EVR02_PARAM_H__ */
