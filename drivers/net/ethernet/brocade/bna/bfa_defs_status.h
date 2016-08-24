/*
 * Linux network driver for Brocade Converged Network Adapter.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (GPL) Version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
/*
 * Copyright (c) 2005-2010 Brocade Communications Systems, Inc.
 * All rights reserved
 * www.brocade.com
 */
#ifndef __BFA_DEFS_STATUS_H__
#define __BFA_DEFS_STATUS_H__

/**
 * API status return values
 *
 * NOTE: The error msgs are auto generated from the comments. Only singe line
 * comments are supported
 */
enum bfa_status {
	BFA_STATUS_OK = 0,
	BFA_STATUS_FAILED = 1,
	BFA_STATUS_EINVAL = 2,
	BFA_STATUS_ENOMEM = 3,
	BFA_STATUS_ENOSYS = 4,
	BFA_STATUS_ETIMER = 5,
	BFA_STATUS_EPROTOCOL = 6,
	BFA_STATUS_ENOFCPORTS = 7,
	BFA_STATUS_NOFLASH = 8,
	BFA_STATUS_BADFLASH = 9,
	BFA_STATUS_SFP_UNSUPP = 10,
	BFA_STATUS_UNKNOWN_VFID = 11,
	BFA_STATUS_DATACORRUPTED = 12,
	BFA_STATUS_DEVBUSY = 13,
	BFA_STATUS_ABORTED = 14,
	BFA_STATUS_NODEV = 15,
	BFA_STATUS_HDMA_FAILED = 16,
	BFA_STATUS_FLASH_BAD_LEN = 17,
	BFA_STATUS_UNKNOWN_LWWN = 18,
	BFA_STATUS_UNKNOWN_RWWN = 19,
	BFA_STATUS_FCPT_LS_RJT = 20,
	BFA_STATUS_VPORT_EXISTS = 21,
	BFA_STATUS_VPORT_MAX = 22,
	BFA_STATUS_UNSUPP_SPEED = 23,
	BFA_STATUS_INVLD_DFSZ = 24,
	BFA_STATUS_CNFG_FAILED = 25,
	BFA_STATUS_CMD_NOTSUPP = 26,
	BFA_STATUS_NO_ADAPTER = 27,
	BFA_STATUS_LINKDOWN = 28,
	BFA_STATUS_FABRIC_RJT = 29,
	BFA_STATUS_UNKNOWN_VWWN = 30,
	BFA_STATUS_NSLOGIN_FAILED = 31,
	BFA_STATUS_NO_RPORTS = 32,
	BFA_STATUS_NSQUERY_FAILED = 33,
	BFA_STATUS_PORT_OFFLINE = 34,
	BFA_STATUS_RPORT_OFFLINE = 35,
	BFA_STATUS_TGTOPEN_FAILED = 36,
	BFA_STATUS_BAD_LUNS = 37,
	BFA_STATUS_IO_FAILURE = 38,
	BFA_STATUS_NO_FABRIC = 39,
	BFA_STATUS_EBADF = 40,
	BFA_STATUS_EINTR = 41,
	BFA_STATUS_EIO = 42,
	BFA_STATUS_ENOTTY = 43,
	BFA_STATUS_ENXIO = 44,
	BFA_STATUS_EFOPEN = 45,
	BFA_STATUS_VPORT_WWN_BP = 46,
	BFA_STATUS_PORT_NOT_DISABLED = 47,
	BFA_STATUS_BADFRMHDR = 48,
	BFA_STATUS_BADFRMSZ = 49,
	BFA_STATUS_MISSINGFRM = 50,
	BFA_STATUS_LINKTIMEOUT = 51,
	BFA_STATUS_NO_FCPIM_NEXUS = 52,
	BFA_STATUS_CHECKSUM_FAIL = 53,
	BFA_STATUS_GZME_FAILED = 54,
	BFA_STATUS_SCSISTART_REQD = 55,
	BFA_STATUS_IOC_FAILURE = 56,
	BFA_STATUS_INVALID_WWN = 57,
	BFA_STATUS_MISMATCH = 58,
	BFA_STATUS_IOC_ENABLED = 59,
	BFA_STATUS_ADAPTER_ENABLED = 60,
	BFA_STATUS_IOC_NON_OP = 61,
	BFA_STATUS_ADDR_MAP_FAILURE = 62,
	BFA_STATUS_SAME_NAME = 63,
	BFA_STATUS_PENDING = 64,
	BFA_STATUS_8G_SPD = 65,
	BFA_STATUS_4G_SPD = 66,
	BFA_STATUS_AD_IS_ENABLE = 67,
	BFA_STATUS_EINVAL_TOV = 68,
	BFA_STATUS_EINVAL_QDEPTH = 69,
	BFA_STATUS_VERSION_FAIL = 70,
	BFA_STATUS_DIAG_BUSY = 71,
	BFA_STATUS_BEACON_ON = 72,
	BFA_STATUS_BEACON_OFF = 73,
	BFA_STATUS_LBEACON_ON = 74,
	BFA_STATUS_LBEACON_OFF = 75,
	BFA_STATUS_PORT_NOT_INITED = 76,
	BFA_STATUS_RPSC_ENABLED = 77,
	BFA_STATUS_ENOFSAVE = 78,
	BFA_STATUS_BAD_FILE = 79,
	BFA_STATUS_RLIM_EN = 80,
	BFA_STATUS_RLIM_DIS = 81,
	BFA_STATUS_IOC_DISABLED = 82,
	BFA_STATUS_ADAPTER_DISABLED = 83,
	BFA_STATUS_BIOS_DISABLED = 84,
	BFA_STATUS_AUTH_ENABLED = 85,
	BFA_STATUS_AUTH_DISABLED = 86,
	BFA_STATUS_ERROR_TRL_ENABLED = 87,
	BFA_STATUS_ERROR_QOS_ENABLED = 88,
	BFA_STATUS_NO_SFP_DEV = 89,
	BFA_STATUS_MEMTEST_FAILED = 90,
	BFA_STATUS_INVALID_DEVID = 91,
	BFA_STATUS_QOS_ENABLED = 92,
	BFA_STATUS_QOS_DISABLED = 93,
	BFA_STATUS_INCORRECT_DRV_CONFIG = 94,
	BFA_STATUS_REG_FAIL = 95,
	BFA_STATUS_IM_INV_CODE = 96,
	BFA_STATUS_IM_INV_VLAN = 97,
	BFA_STATUS_IM_INV_ADAPT_NAME = 98,
	BFA_STATUS_IM_LOW_RESOURCES = 99,
	BFA_STATUS_IM_VLANID_IS_PVID = 100,
	BFA_STATUS_IM_VLANID_EXISTS = 101,
	BFA_STATUS_IM_FW_UPDATE_FAIL = 102,
	BFA_STATUS_PORTLOG_ENABLED = 103,
	BFA_STATUS_PORTLOG_DISABLED = 104,
	BFA_STATUS_FILE_NOT_FOUND = 105,
	BFA_STATUS_QOS_FC_ONLY = 106,
	BFA_STATUS_RLIM_FC_ONLY = 107,
	BFA_STATUS_CT_SPD = 108,
	BFA_STATUS_LEDTEST_OP = 109,
	BFA_STATUS_CEE_NOT_DN = 110,
	BFA_STATUS_10G_SPD = 111,
	BFA_STATUS_IM_INV_TEAM_NAME = 112,
	BFA_STATUS_IM_DUP_TEAM_NAME = 113,
	BFA_STATUS_IM_ADAPT_ALREADY_IN_TEAM = 114,
	BFA_STATUS_IM_ADAPT_HAS_VLANS = 115,
	BFA_STATUS_IM_PVID_MISMATCH = 116,
	BFA_STATUS_IM_LINK_SPEED_MISMATCH = 117,
	BFA_STATUS_IM_MTU_MISMATCH = 118,
	BFA_STATUS_IM_RSS_MISMATCH = 119,
	BFA_STATUS_IM_HDS_MISMATCH = 120,
	BFA_STATUS_IM_OFFLOAD_MISMATCH = 121,
	BFA_STATUS_IM_PORT_PARAMS = 122,
	BFA_STATUS_IM_PORT_NOT_IN_TEAM = 123,
	BFA_STATUS_IM_CANNOT_REM_PRI = 124,
	BFA_STATUS_IM_MAX_PORTS_REACHED = 125,
	BFA_STATUS_IM_LAST_PORT_DELETE = 126,
	BFA_STATUS_IM_NO_DRIVER = 127,
	BFA_STATUS_IM_MAX_VLANS_REACHED = 128,
	BFA_STATUS_TOMCAT_SPD_NOT_ALLOWED = 129,
	BFA_STATUS_NO_MINPORT_DRIVER = 130,
	BFA_STATUS_CARD_TYPE_MISMATCH = 131,
	BFA_STATUS_BAD_ASICBLK = 132,
	BFA_STATUS_NO_DRIVER = 133,
	BFA_STATUS_INVALID_MAC = 134,
	BFA_STATUS_IM_NO_VLAN = 135,
	BFA_STATUS_IM_ETH_LB_FAILED = 136,
	BFA_STATUS_IM_PVID_REMOVE = 137,
	BFA_STATUS_IM_PVID_EDIT = 138,
	BFA_STATUS_CNA_NO_BOOT = 139,
	BFA_STATUS_IM_PVID_NON_ZERO = 140,
	BFA_STATUS_IM_INETCFG_LOCK_FAILED = 141,
	BFA_STATUS_IM_GET_INETCFG_FAILED = 142,
	BFA_STATUS_IM_NOT_BOUND = 143,
	BFA_STATUS_INSUFFICIENT_PERMS = 144,
	BFA_STATUS_IM_INV_VLAN_NAME = 145,
	BFA_STATUS_CMD_NOTSUPP_CNA = 146,
	BFA_STATUS_IM_PASSTHRU_EDIT = 147,
	BFA_STATUS_IM_BIND_FAILED = 148,
	BFA_STATUS_IM_UNBIND_FAILED = 149,
	BFA_STATUS_IM_PORT_IN_TEAM = 150,
	BFA_STATUS_IM_VLAN_NOT_FOUND = 151,
	BFA_STATUS_IM_TEAM_NOT_FOUND = 152,
	BFA_STATUS_IM_TEAM_CFG_NOT_ALLOWED = 153,
	BFA_STATUS_PBC = 154,
	BFA_STATUS_DEVID_MISSING = 155,
	BFA_STATUS_BAD_FWCFG = 156,
	BFA_STATUS_CREATE_FILE = 157,
	BFA_STATUS_INVALID_VENDOR = 158,
	BFA_STATUS_SFP_NOT_READY = 159,
	BFA_STATUS_FLASH_UNINIT = 160,
	BFA_STATUS_FLASH_EMPTY = 161,
	BFA_STATUS_FLASH_CKFAIL = 162,
	BFA_STATUS_TRUNK_UNSUPP = 163,
	BFA_STATUS_TRUNK_ENABLED = 164,
	BFA_STATUS_TRUNK_DISABLED = 165,
	BFA_STATUS_TRUNK_ERROR_TRL_ENABLED = 166,
	BFA_STATUS_BOOT_CODE_UPDATED = 167,
	BFA_STATUS_BOOT_VERSION = 168,
	BFA_STATUS_CARDTYPE_MISSING = 169,
	BFA_STATUS_INVALID_CARDTYPE = 170,
	BFA_STATUS_NO_TOPOLOGY_FOR_CNA = 171,
	BFA_STATUS_IM_VLAN_OVER_TEAM_DELETE_FAILED = 172,
	BFA_STATUS_ETHBOOT_ENABLED = 173,
	BFA_STATUS_ETHBOOT_DISABLED = 174,
	BFA_STATUS_IOPROFILE_OFF = 175,
	BFA_STATUS_NO_PORT_INSTANCE = 176,
	BFA_STATUS_BOOT_CODE_TIMEDOUT = 177,
	BFA_STATUS_NO_VPORT_LOCK = 178,
	BFA_STATUS_VPORT_NO_CNFG = 179,
	BFA_STATUS_MAX_VAL
};

enum bfa_eproto_status {
	BFA_EPROTO_BAD_ACCEPT = 0,
	BFA_EPROTO_UNKNOWN_RSP = 1
};

#endif /* __BFA_DEFS_STATUS_H__ */
