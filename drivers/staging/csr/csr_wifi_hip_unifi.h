/*****************************************************************************

            (c) Cambridge Silicon Radio Limited 2011
            All rights reserved and confidential information of CSR

            Refer to LICENSE.txt included with this source for details
            on the license terms.

*****************************************************************************/

/*
 * ---------------------------------------------------------------------------
 *
 * FILE : csr_wifi_hip_unifi.h
 *
 * PURPOSE : Public API for the UniFi HIP core library.
 *
 * ---------------------------------------------------------------------------
 */
#ifndef __CSR_WIFI_HIP_UNIFI_H__
#define __CSR_WIFI_HIP_UNIFI_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CSR_WIFI_HIP_TA_DISABLE
#include "csr_wifi_router_ctrl_prim.h"
#include "csr_wifi_router_prim.h"
#else
#include "csr_time.h"
#endif

/* SDIO chip ID numbers */

/* Manufacturer id */
#define SDIO_MANF_ID_CSR              0x032a

/* Device id */
#define SDIO_CARD_ID_UNIFI_1          0x0001
#define SDIO_CARD_ID_UNIFI_2          0x0002
#define SDIO_CARD_ID_UNIFI_3          0x0007
#define SDIO_CARD_ID_UNIFI_4          0x0008

/* Function number for WLAN */
#define SDIO_WLAN_FUNC_ID_UNIFI_1          0x0001
#define SDIO_WLAN_FUNC_ID_UNIFI_2          0x0001
#define SDIO_WLAN_FUNC_ID_UNIFI_3          0x0001
#define SDIO_WLAN_FUNC_ID_UNIFI_4          0x0002

/* Maximum SDIO bus clock supported. */
#define UNIFI_SDIO_CLOCK_MAX_HZ    50000000  /* Hz */

/*
 * Initialisation SDIO bus clock.
 *
 * The initialisation clock speed should be used from when the chip has been
 * reset until the first MLME-reset has been received (i.e. during firmware
 * initialisation), unless UNIFI_SDIO_CLOCK_SAFE_HZ applies.
 */
#define UNIFI_SDIO_CLOCK_INIT_HZ    12500000 /* Hz */

/*
 * Safe SDIO bus clock.
 *
 * The safe speed should be used when the chip is in deep sleep or
 * it's state is unknown (just after reset / power on).
 */
#define UNIFI_SDIO_CLOCK_SAFE_HZ    1000000  /* Hz */

/* I/O default block size to use for UniFi. */
#define UNIFI_IO_BLOCK_SIZE     64

#define UNIFI_WOL_OFF   0
#define UNIFI_WOL_SDIO  1
#define UNIFI_WOL_PIO   2

/* The number of Tx traffic queues */
#define UNIFI_NO_OF_TX_QS              4

#define CSR_WIFI_HIP_RESERVED_HOST_TAG 0xFFFFFFFF

/*
 * The number of slots in the from-host queues.
 *
 * UNIFI_SOFT_TRAFFIC_Q_LENGTH is the number of slots in the traffic queues
 * and there will be UNIFI_NO_OF_TX_QS of them.
 * Traffic queues are used for data packets.
 *
 * UNIFI_SOFT_COMMAND_Q_LENGTH is the number of slots in the command queue.
 * The command queue is used for MLME management requests.
 *
 * Queues are ring buffers and so must always have 1 unused slot.
 */
#define UNIFI_SOFT_TRAFFIC_Q_LENGTH (20 + 1)
#define UNIFI_SOFT_COMMAND_Q_LENGTH (16 + 1)

#include "csr_types.h"          /* from the synergy porting folder */
#include "csr_framework_ext.h"  /* from the synergy porting folder */
#include "csr_sdio.h"           /* from the synergy porting folder */
#include "csr_pmem.h"           /* from the synergy porting folder */
#include "csr_util.h"           /* from the synergy porting folder */
#include "csr_formatted_io.h"   /* from the synergy gsp folder */
#include "csr_wifi_result.h"


/* Traffic queue ordered according to priority
 * EAPOL/Uncontrolled port Queue should be the last
 */
typedef enum
{
    UNIFI_TRAFFIC_Q_BK = 0,
    UNIFI_TRAFFIC_Q_BE,
    UNIFI_TRAFFIC_Q_VI,
    UNIFI_TRAFFIC_Q_VO,
    UNIFI_TRAFFIC_Q_EAPOL,    /* Non existant in HIP */
    UNIFI_TRAFFIC_Q_MAX,      /* Non existant */
    UNIFI_TRAFFIC_Q_MLME      /* Non existant */
} unifi_TrafficQueue;

/*
 * Structure describing a bulk data slot.
 * This structure is shared between the HIP core library and the OS
 * layer. See the definition of unifi_net_data_malloc() for more details.
 *
 * The data_length field is used to indicate empty/occupied state.
 * Needs to be defined before #include "unifi_os.h".
 */
typedef struct _bulk_data_desc
{
    const CsrUint8 *os_data_ptr;
    CsrUint32       data_length;
    const void     *os_net_buf_ptr;
    CsrUint32       net_buf_length;
} bulk_data_desc_t;

/* Structure of an entry in the Symbol Look Up Table (SLUT). */
typedef struct _symbol
{
    CsrUint16 id;
    CsrUint32 obj;
} symbol_t;

/*
 * Header files need to be included from the current directory,
 * the SME library, the synergy framework and the OS layer.
 * A thin OS layer needs to be implemented in the porting exercise.
 *
 * Note that unifi_os.h should be included only in unifi.h
 */

#include "unifi_os.h"

/*
 * Contains the HIP core definitions selected in the porting exercise, such as
 * UNIFI_PAD_BULK_DATA_TO_BLOCK_SIZE and UNIFI_PAD_SIGNALS_TO_BLOCK_SIZE.
 * Implemented in the OS layer, as part of the porting exersice.
 */
#include "unifi_config.h"

#include "csr_wifi_hip_signals.h" /* from this dir */

/*
 * The card structure is an opaque pointer that is used to pass context
 * to the upper-edge API functions.
 */
typedef struct card card_t;


/*
 * This structure describes all of the bulk data that 'might' be
 * associated with a signal.
 */
typedef struct _bulk_data_param
{
    bulk_data_desc_t d[UNIFI_MAX_DATA_REFERENCES];
} bulk_data_param_t;


/*
 * This structure describes the chip and HIP core lib
 * information that exposed to the OS layer.
 */
typedef struct _card_info
{
    CsrUint16 chip_id;
    CsrUint16 chip_version;
    CsrUint32 fw_build;
    CsrUint16 fw_hip_version;
    CsrUint32 sdio_block_size;
} card_info_t;


/*
 * Mini-coredump definitions
 */
/* Definition of XAP memory ranges used by the mini-coredump system.
 * Note that, these values are NOT the same as UNIFI_REGISTERS, etc
 * in unifihw.h which don't allow selection of register areas for each XAP.
 */
typedef enum unifi_coredump_space
{
    UNIFI_COREDUMP_MAC_REG,
    UNIFI_COREDUMP_PHY_REG,
    UNIFI_COREDUMP_SH_DMEM,
    UNIFI_COREDUMP_MAC_DMEM,
    UNIFI_COREDUMP_PHY_DMEM,
    UNIFI_COREDUMP_TRIGGER_MAGIC = 0xFEED
} unifi_coredump_space_t;

/* Structure used to request a register value from a mini-coredump buffer */
typedef struct unifi_coredump_req
{
    /* From user */
    CsrInt32               index;       /* 0=newest, -1=oldest */
    unifi_coredump_space_t space;       /* memory space */
    CsrUint32              offset;      /* register offset in space */
    /* From driver */
    CsrUint32 drv_build;                /* Driver build id */
    CsrUint32 chip_ver;                 /* Chip version */
    CsrUint32 fw_ver;                   /* Firmware version */
    CsrInt32  requestor;                /* Requestor: 0=auto dump, 1=manual */
    CsrTime   timestamp;                /* time of capture by driver */
    CsrUint32 serial;                   /* capture serial number */
    CsrInt32  value;                    /* register value */
} unifi_coredump_req_t;                 /* mini-coredumped reg value request */


/**
 * @defgroup upperedge Upper edge API
 *
 * The following functions are implemented in the HIP core lib.
 */

/**
 *
 * Initialise the HIP core lib.
 * Note that the OS layer must initialise the SDIO glue layer and obtain
 * an SDIO function context, prior to this call.
 *
 * @param sdiopriv the SDIO function context.
 *
 * @param ospriv the OS layer context.
 *
 * @return \p card_t the HIP core lib API context.
 *
 * @ingroup upperedge
 */
card_t* unifi_alloc_card(CsrSdioFunction *sdiopriv, void *ospriv);


/**
 *
 * Initialise the UniFi chip.
 *
 * @param card the HIP core lib API context.
 *
 * @param led_mask the led mask to apply to UniFi.
 *
 * @return \b 0 if UniFi is initialized.
 *
 * @return \b -CSR_EIO if an I/O error occured while initializing UniFi
 *
 * @return \b -CSR_ENODEV if the card is no longer present.
 *
 * @ingroup upperedge
 */
CsrResult unifi_init_card(card_t *card, CsrInt32 led_mask);

/**
 *
 * De-Initialise the HIP core lib.
 *
 * @param card the HIP core lib API context.
 *
 * @ingroup upperedge
 */
void unifi_free_card(card_t *card);

/**
 *
 * Cancel all the signals pending in the HIP core lib.
 * Normally used during a system suspend when the power is retained on UniFi.
 *
 * @param card the HIP core lib API context.
 *
 * @ingroup upperedge
 */
void unifi_cancel_pending_signals(card_t *card);

/**
 *
 * Send a signal to UniFi.
 * Normally it is called from unifi_sys_hip_req() and the OS layer
 * Tx data plane.
 *
 * Note that the bulkdata buffers ownership is passed to the HIP core lib.
 * These buffers must be allocated using unifi_net_data_malloc().
 *
 * @param card the HIP core lib API context.
 *
 * @param sigptr pointer to the signal.
 *
 * @param siglen size of the signal.
 *
 * @param bulkdata pointer to the bulk data associated with the signal.
 *
 * @return \b 0 signal is sent.
 *
 * @return \b -CSR_EIO if an error occured while sending the signal
 *
 * @return \b -CSR_ENODEV if the card is no longer present.
 *
 * @ingroup upperedge
 */
CsrResult unifi_send_signal(card_t *card, const CsrUint8 *sigptr,
                            CsrUint32 siglen,
                            const bulk_data_param_t *bulkdata);

/**
 *
 * Check if the HIP core lib has resources to send a signal.
 * Normally there no need to use this function.
 *
 * @param card the HIP core lib API context.
 *
 * @param sigptr pointer to the signal.
 *
 * @return \b 0 if there are resources for the signal.
 *
 * @return \b -CSR_ENOSPC if there are not enough resources
 *
 * @ingroup upperedge
 */
CsrResult unifi_send_resources_available(card_t *card, const CsrUint8 *sigptr);

/**
 *
 * Read the UniFi chip and the HIP core lib information.
 *
 * @param card the HIP core lib API context.
 *
 * @param card_info pointer to save the information.
 *
 * @ingroup upperedge
 */
void unifi_card_info(card_t *card, card_info_t *card_info);

/**
 *
 * Print the UniFi I/O and Interrupt status.
 * Normally it is used for debug purposes only.
 *
 * @param card the HIP core lib API context.

 * @param status buffer for the chip status
 *
 * @return \b 0 if the check was performed.
 *
 * @return \b -CSR_EIO if an error occured while checking the status.
 *
 * @return \b -CSR_ENODEV if the card is no longer present.
 *
 * @ingroup upperedge
 */
CsrResult unifi_check_io_status(card_t *card, CsrInt32 *status);


/**
 *
 * Run the HIP core lib Botton-Half.
 * Whenever the HIP core lib want this function to be called
 * by the OS layer, it calls unifi_run_bh().
 *
 * @param card the HIP core lib API context.
 *
 * @param remaining pointer to return the time (in msecs) that this function
 * should be re-scheduled. A return value of 0 means that no re-scheduling
 * is required. If unifi_bh() is called before the timeout expires,
 * the caller must pass in the remaining time.
 *
 * @return \b 0 if no error occured.
 *
 * @return \b -CSR_ENODEV if the card is no longer present.
 *
 * @return \b -CSR_E* if an error occured while running the bottom half.
 *
 * @ingroup upperedge
 */
CsrResult unifi_bh(card_t *card, CsrUint32 *remaining);


/**
 * UniFi Low Power Mode (Deep Sleep Signaling)
 *
 * unifi_low_power_mode defines the UniFi Deep Sleep Signaling status.
 * Use with unifi_configure_low_power_mode() to enable/disable
 * the Deep Sleep Signaling.
 */
enum unifi_low_power_mode
{
    UNIFI_LOW_POWER_DISABLED,
    UNIFI_LOW_POWER_ENABLED
};

/**
 * Periodic Wake Host Mode
 *
 * unifi_periodic_wake_mode defines the Periodic Wake Host Mode.
 * It can only be set to UNIFI_PERIODIC_WAKE_HOST_ENABLED if
 * low_power_mode == UNIFI_LOW_POWER_ENABLED.
 */
enum unifi_periodic_wake_mode
{
    UNIFI_PERIODIC_WAKE_HOST_DISABLED,
    UNIFI_PERIODIC_WAKE_HOST_ENABLED
};

/**
 *
 * Run the HIP core lib Botton-Half.
 * Whenever the HIP core lib want this function to be called
 * by the OS layer, it calls unifi_run_bh().
 *
 * Typically, the SME is responsible for configuring these parameters,
 * so unifi_sys_configure_power_mode_req() is usually implemented
 * as a direct call to unifi_configure_low_power_mode().
 *
 * Note: When polling mode is used instead of interrupts,
 * low_power_mode must never be set to UNIFI_LOW_POWER_ENABLED.
 *
 * @param card the HIP core lib API context.
 *
 * @param low_power_mode the Low Power Mode.
 *
 * @param periodic_wake_mode the Periodic Wake Mode.
 *
 * @return \b 0 if no error occured.
 *
 * @return \b -CSR_E* if the request failed.
 *
 * @ingroup upperedge
 */
CsrResult unifi_configure_low_power_mode(card_t                       *card,
                                         enum unifi_low_power_mode     low_power_mode,
                                         enum unifi_periodic_wake_mode periodic_wake_mode);

/**
 *
 * Forces the UniFi chip to enter a Deep Sleep state.
 * This is normally called by the OS layer when the platform suspends.
 *
 * Note that if the UniFi Low Power Mode is disabled this call fails.
 *
 * @param card the HIP core lib API context.
 *
 * @return \b 0 if no error occured.
 *
 * @return \b -CSR_ENODEV if the card is no longer present.
 *
 * @return \b -CSR_E* if the request failed.
 *
 * @ingroup upperedge
 */
CsrResult unifi_force_low_power_mode(card_t *card);

#ifndef CSR_WIFI_HIP_TA_DISABLE
/**
 * Configure the Traffic Analysis sampling
 *
 * Enable or disable statistics gathering.
 * Enable or disable particular packet detection.
 *
 * @param card the HIP core context
 * @param config_type the item to configure
 * @param config pointer to struct containing config info
 *
 * @return \b 0 if configuration was successful
 *
 * @return \b -CSR_EINVAL if a parameter had an invalid value
 *
 * @ingroup upperedge
 */
CsrResult unifi_ta_configure(card_t                               *card,
                             CsrWifiRouterCtrlTrafficConfigType    config_type,
                             const CsrWifiRouterCtrlTrafficConfig *config);

/**
 * Pass a packet for Traffic Analysis sampling
 *
 * @param card the HIP core context
 * @param direction the direction (Rx or Tx) of the frame.
 * @param data pointer to bulkdata struct containing the packet
 * @param saddr the source address of the packet
 * @param sta_macaddr the MAC address of the UniFi chip
 * @param timestamp the current time in msecs
 *
 * @ingroup upperedge
 */
void unifi_ta_sample(card_t                            *card,
                     CsrWifiRouterCtrlProtocolDirection direction,
                     const bulk_data_desc_t            *data,
                     const CsrUint8                    *saddr,
                     const CsrUint8                    *sta_macaddr,
                     CsrUint32                          timestamp,
                     CsrUint16                          rate);

/**
 * Notify the HIP core lib for a detected Traffic Classification.
 * Typically, the SME is responsible for configuring these parameters,
 * so unifi_sys_traffic_classification_req() is usually implemented
 * as a direct call to unifi_ta_classification().
 *
 * @param card the HIP core context.
 * @param traffic_type the detected traffic type.
 * @param period The detected period of the traffic.
 *
 * @ingroup upperedge
 */
void unifi_ta_classification(card_t                      *card,
                             CsrWifiRouterCtrlTrafficType traffic_type,
                             CsrUint16                    period);

#endif
/**
 * Use software to hard reset the chip.
 * This is a subset of the unifi_init_card() functionality and should
 * only be used only to reset a paniced chip before a coredump is taken.
 *
 * @param card the HIP core context.
 *
 * @ingroup upperedge
 */
CsrResult unifi_card_hard_reset(card_t *card);


CsrResult unifi_card_readn(card_t *card, CsrUint32 unifi_addr, void *pdata, CsrUint16 len);
CsrResult unifi_card_read16(card_t *card, CsrUint32 unifi_addr, CsrUint16 *pdata);
CsrResult unifi_card_write16(card_t *card, CsrUint32 unifi_addr, CsrUint16 data);


enum unifi_dbg_processors_select
{
    UNIFI_PROC_MAC,
    UNIFI_PROC_PHY,
    UNIFI_PROC_BT,
    UNIFI_PROC_BOTH,
    UNIFI_PROC_INVALID
};

CsrResult unifi_card_stop_processor(card_t *card, enum unifi_dbg_processors_select which);

/**
 * Call-outs from the HIP core lib to the OS layer.
 * The following functions need to be implemented during the porting exercise.
 */

/**
 * Selects appropriate queue according to priority
 * Helps maintain uniformity in queue selection between the HIP
 * and the OS layers.
 *
 * @param priority priority of the packet
 *
 * @return \b Traffic queue to which a packet of this priority belongs
 *
 * @ingroup upperedge
 */
unifi_TrafficQueue
unifi_frame_priority_to_queue(CSR_PRIORITY priority);

/**
 * Returns the priority corresponding to a particular Queue when that is used
 * when downgrading a packet to a lower AC.
 * Helps maintain uniformity in queue - priority mapping between the HIP
 * and the OS layers.
 *
 * @param queue
 *
 * @return \b Highest priority corresponding to this queue
 *
 * @ingroup upperedge
 */
CSR_PRIORITY unifi_get_default_downgrade_priority(unifi_TrafficQueue queue);

/**
 *
 * Flow control callbacks.
 * unifi_pause_xmit() is called when the HIP core lib does not have any
 * resources to store data packets. The OS layer needs to pause
 * the Tx data plane until unifi_restart_xmit() is called.
 *
 * @param ospriv the OS layer context.
 *
 * @ingroup upperedge
 */
void unifi_pause_xmit(void *ospriv, unifi_TrafficQueue queue);
void unifi_restart_xmit(void *ospriv, unifi_TrafficQueue queue);

/**
 *
 * Request to run the Bottom-Half.
 * The HIP core lib calls this function to request that unifi_bh()
 * needs to be run by the OS layer. It can be called anytime, i.e.
 * when the unifi_bh() is running.
 * Since unifi_bh() is not re-entrant, usually unifi_run_bh() sets
 * an event to a thread that schedules a call to unifi_bh().
 *
 * @param ospriv the OS layer context.
 *
 * @ingroup upperedge
 */
CsrResult unifi_run_bh(void *ospriv);

/**
 *
 * Delivers a signal received from UniFi to the OS layer.
 * Normally, the data signals should be delivered to the data plane
 * and all the rest to the SME (unifi_sys_hip_ind()).
 *
 * Note that the OS layer is responsible for freeing the bulkdata
 * buffers, using unifi_net_data_free().
 *
 * @param ospriv the OS layer context.
 *
 * @param sigptr pointer to the signal.
 *
 * @param siglen size of the signal.
 *
 * @param bulkdata pointer to the bulk data associated with the signal.
 *
 * @ingroup upperedge
 */
void unifi_receive_event(void *ospriv,
                         CsrUint8 *sigdata, CsrUint32 siglen,
                         const bulk_data_param_t *bulkdata);


typedef struct
{
    CsrUint16 free_fh_sig_queue_slots[UNIFI_NO_OF_TX_QS];
    CsrUint16 free_fh_bulkdata_slots;
    CsrUint16 free_fh_fw_slots;
} unifi_HipQosInfo;

void unifi_get_hip_qos_info(card_t *card, unifi_HipQosInfo *hipqosinfo);


/**
 * Functions that read a portion of a firmware file.
 *
 * Note: If the UniFi chip runs the f/w from ROM, the HIP core may never
 * call these functions. Also, the HIP core may call these functions even if
 * a f/w file is not available. In this case, it is safe to fail the request.
 */
#define UNIFI_FW_STA    1   /* Identify STA firmware file */

/**
 *
 * Ask the OS layer to initialise a read from a f/w file.
 *
 * @param ospriv the OS layer context.
 *
 * @param is_fw if 0 the request if for the loader file, if 1 the request
 * is for a f/w file.
 *
 * @param info a card_info_t structure containing versions information.
 * Note that some members of the structure may not be initialised.
 *
 * @return \p NULL if the file is not available, or a pointer which contains
 * OS specific information for the file (typically the contents of the file)
 * that the HIP core uses when calling unifi_fw_read() and unifi_fw_read_stop()
 *
 * @ingroup upperedge
 */
void* unifi_fw_read_start(void *ospriv, CsrInt8 is_fw, const card_info_t *info);

/**
 *
 * Ask the OS layer to return a portion from a f/w file.
 *
 * @param ospriv the OS layer context.
 *
 * @param arg the OS pointer returned by unifi_fw_read_start().
 *
 * @param offset the offset in the f/w file to read the read from.
 *
 * @param buf the buffer to store the returned data.
 *
 * @param len the size in bytes of the requested read.
 *
 * @ingroup upperedge
 */
CsrInt32 unifi_fw_read(void *ospriv, void *arg, CsrUint32 offset, void *buf, CsrUint32 len);

/**
 *
 * Ask the OS layer to finish reading from a f/w file.
 *
 * @param ospriv the OS layer context.
 *
 * @param dlpriv the OS pointer returned by unifi_fw_read_start().
 *
 * @ingroup upperedge
 */
void unifi_fw_read_stop(void *ospriv, void *dlpriv);

/**
 *
 * Ask OS layer for a handle to a dynamically allocated firmware buffer
 * (primarily intended for production test images which may need conversion)
 *
 * @param ospriv the OS layer context.
 *
 * @param fwbuf pointer to dynamically allocated buffer
 *
 * @param len length of provided buffer in bytes
 *
 * @ingroup upperedge
 */
void* unifi_fw_open_buffer(void *ospriv, void *fwbuf, CsrUint32 len);

/**
 *
 * Release a handle to a dynamically allocated firmware buffer
 * (primarily intended for production test images which may need conversion)
 *
 * @param ospriv the OS layer context.
 *
 * @param fwbuf pointer to dynamically allocated buffer
 *
 * @ingroup upperedge
 */
void unifi_fw_close_buffer(void *ospriv, void *fwbuf);

#ifndef CSR_WIFI_HIP_TA_DISABLE
/*
 * Driver must provide these.
 *
 * A simple implementation will just call
 * unifi_sys_traffic_protocol_ind() or unifi_sys_traffic_classification_ind()
 * respectively. See sme_csr_userspace/sme_userspace.c.
 */
/**
 *
 * Indicates a detected packet of type packet_type.
 * Typically, this information is processed by the SME so
 * unifi_ta_indicate_protocol() needs to schedule a call to
 * unifi_sys_traffic_protocol_ind().
 *
 * @param ospriv the OS layer context.
 *
 * @param packet_type the detected packet type.
 *
 * @param direction the direction of the packet (Rx, Tx).
 *
 * @param src_addr the source address of the packet.
 *
 * @ingroup upperedge
 */
void unifi_ta_indicate_protocol(void                              *ospriv,
                                CsrWifiRouterCtrlTrafficPacketType packet_type,
                                CsrWifiRouterCtrlProtocolDirection direction,
                                const CsrWifiMacAddress           *src_addr);

/**
 *
 * Indicates statistics for the sample data over a period.
 * Typically, this information is processed by the SME so
 * unifi_ta_indicate_sampling() needs to schedule a call to
 * unifi_sys_traffic_sample_ind().
 *
 * @param ospriv the OS layer context.
 *
 * @param stats the pointer to the structure that contains the statistics.
 *
 * @ingroup upperedge
 */
void unifi_ta_indicate_sampling(void *ospriv, CsrWifiRouterCtrlTrafficStats *stats);
void unifi_ta_indicate_l4stats(void     *ospriv,
                               CsrUint32 rxTcpThroughput,
                               CsrUint32 txTcpThroughput,
                               CsrUint32 rxUdpThroughput,
                               CsrUint32 txUdpThroughput);
#endif

void unifi_rx_queue_flush(void *ospriv);

/**
 * Call-out from the SDIO glue layer.
 *
 * The glue layer needs to call unifi_sdio_interrupt_handler() every time
 * an interrupts occurs.
 *
 * @param card the HIP core context.
 *
 * @ingroup bottomedge
 */
void unifi_sdio_interrupt_handler(card_t *card);


/* HELPER FUNCTIONS */

/*
 * unifi_init() and unifi_download() implement a subset of unifi_init_card functionality
 * that excludes HIP initialization.
 */
CsrResult unifi_init(card_t *card);
CsrResult unifi_download(card_t *card, CsrInt32 led_mask);

/*
 * unifi_start_processors() ensures both on-chip processors are running
 */
CsrResult unifi_start_processors(card_t *card);

CsrResult unifi_capture_panic(card_t *card);

/*
 * Configure HIP interrupt processing mode
 */
#define CSR_WIFI_INTMODE_DEFAULT        0
#define CSR_WIFI_INTMODE_RUN_BH_ONCE    1       /* Run BH once per interrupt */

void unifi_set_interrupt_mode(card_t *card, CsrUint32 mode);

/*
 * unifi_request_max_clock() requests that max SDIO clock speed is set at the
 * next suitable opportunity.
 */
void unifi_request_max_sdio_clock(card_t *card);


/* Functions to lookup bulk data command names. */
const CsrCharString* lookup_bulkcmd_name(CsrUint16 id);

/* Function to log HIP's global debug buffer */
#ifdef CSR_WIFI_HIP_DEBUG_OFFLINE
void unifi_debug_buf_dump(void);
#endif

/* Mini-coredump utility functions */
CsrResult unifi_coredump_get_value(card_t *card, struct unifi_coredump_req *req);
CsrResult unifi_coredump_capture(card_t *card, struct unifi_coredump_req *req);
CsrResult unifi_coredump_request_at_next_reset(card_t *card, CsrInt8 enable);
CsrResult unifi_coredump_init(card_t *card, CsrUint16 num_dump_buffers);
void unifi_coredump_free(card_t *card);

#ifdef __cplusplus
}
#endif

#endif /* __CSR_WIFI_HIP_UNIFI_H__ */
