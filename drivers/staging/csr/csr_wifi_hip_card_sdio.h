/*****************************************************************************

            (c) Cambridge Silicon Radio Limited 2011
            All rights reserved and confidential information of CSR

            Refer to LICENSE.txt included with this source for details
            on the license terms.

*****************************************************************************/

/*
 * ---------------------------------------------------------------------------
 *
 *  FILE:     csr_wifi_hip_card_sdio.h
 *
 *  PURPOSE:
 *      Internal header for Card API for SDIO.
 * ---------------------------------------------------------------------------
 */
#ifndef __CARD_SDIO_H__
#define __CARD_SDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "csr_wifi_hip_unifi.h"
#include "csr_wifi_hip_unifi_udi.h"
#include "csr_wifi_hip_unifihw.h"
#include "csr_wifi_hip_unifiversion.h"
#ifndef CSR_WIFI_HIP_TA_DISABLE
#include "csr_wifi_hip_ta_sampling.h"
#endif
#include "csr_wifi_hip_xbv.h"
#include "csr_wifi_hip_chiphelper.h"


/*
 *
 * Configuration items.
 * Which of these should go in a platform unifi_config.h file?
 *
 */

/*
 * When the traffic queues contain more signals than there is space for on
 * UniFi, a limiting algorithm comes into play.
 * If a traffic queue has enough slots free to buffer more traffic from the
 * network stack, then the following check is applied. The number of free
 * slots is RESUME_XMIT_THRESHOLD.
 */
#define RESUME_XMIT_THRESHOLD           4


/*
 * When reading signals from UniFi, the host processes pending all signals
 * and then acknowledges them together in a single write to update the
 * to-host-chunks-read location.
 * When there is more than one bulk data transfer (e.g. one received data
 * packet and a request for the payload data of a transmitted packet), the
 * update can be delayed significantly. This ties up resources on chip.
 *
 * To remedy this problem, to-host-chunks-read is updated after processing
 * a signal if TO_HOST_FLUSH_THRESHOLD bytes of bulk data have been
 * transferred since the last update.
 */
#define TO_HOST_FLUSH_THRESHOLD (500 * 5)


/* SDIO Card Common Control Registers */
#define SDIO_CCCR_SDIO_REVISION     (0x00)
#define SDIO_SD_SPEC_REVISION       (0x01)
#define SDIO_IO_ENABLE              (0x02)
#define SDIO_IO_READY               (0x03)
#define SDIO_INT_ENABLE             (0x04)
#define SDIO_INT_PENDING            (0x05)
#define SDIO_IO_ABORT               (0x06)
#define SDIO_BUS_IFACE_CONTROL      (0x07)
#define SDIO_CARD_CAPABILOTY        (0x08)
#define SDIO_COMMON_CIS_POINTER     (0x09)
#define SDIO_BUS_SUSPEND            (0x0C)
#define SDIO_FUNCTION_SELECT        (0x0D)
#define SDIO_EXEC_FLAGS             (0x0E)
#define SDIO_READY_FLAGS            (0x0F)
#define SDIO_FN0_BLOCK_SIZE         (0x10)
#define SDIO_POWER_CONTROL          (0x12)
#define SDIO_VENDOR_START           (0xF0)

#define SDIO_CSR_HOST_WAKEUP        (0xf0)
#define SDIO_CSR_HOST_INT_CLEAR     (0xf1)
#define SDIO_CSR_FROM_HOST_SCRATCH0 (0xf2)
#define SDIO_CSR_FROM_HOST_SCRATCH1 (0xf3)
#define SDIO_CSR_TO_HOST_SCRATCH0   (0xf4)
#define SDIO_CSR_TO_HOST_SCRATCH1   (0xf5)
#define SDIO_CSR_FUNC_EN            (0xf6)
#define SDIO_CSR_CSPI_MODE          (0xf7)
#define SDIO_CSR_CSPI_STATUS        (0xf8)
#define SDIO_CSR_CSPI_PADDING       (0xf9)


#define UNIFI_SD_INT_ENABLE_IENM 0x0001    /* Master INT Enable */

#ifdef CSR_PRE_ALLOC_NET_DATA
#define BULK_DATA_PRE_ALLOC_NUM 16
#endif

/*
 * Structure to hold configuration information read from UniFi.
 */
typedef struct
{
    /*
     * The version of the SDIO signal queues and bulk data pools
     * configuration structure. The MSB is the major version number, used to
     * indicate incompatible changes. The LSB gives the minor revision number,
     * used to indicate changes that maintain backwards compatibility.
     */
    CsrUint16 version;

    /*
     * offset from the start of the shared data memory to the SD IO
     * control structure.
     */
    CsrUint16 sdio_ctrl_offset;

    /* Buffer handle of the from-host signal queue */
    CsrUint16 fromhost_sigbuf_handle;

    /* Buffer handle of the to-host signal queue */
    CsrUint16 tohost_sigbuf_handle;

    /*
     * Maximum number of signal primitive or bulk data command fragments that may be
     * pending in the to-hw signal queue.
     */
    CsrUint16 num_fromhost_sig_frags;

    /*
     * Number of signal primitive or bulk data command fragments that must be pending
     * in the to-host signal queue before the host will generate an interrupt
     * to indicate that it has read a signal. This will usually be the total
     * capacity of the to-host signal buffer less the size of the largest signal
     * primitive divided by the signal primitive fragment size, but may be set
     * to 1 to request interrupts every time that the host read a signal.
     * Note that the hw may place more signals in the to-host signal queue
     * than indicated by this field.
     */
    CsrUint16 num_tohost_sig_frags;

    /*
     * Number of to-hw bulk data slots. Slots are numbered from 0 (zero) to
     * one less than the value in this field
     */
    CsrUint16 num_fromhost_data_slots;

    /*
     * Number of frm-hw bulk data slots. Slots are numbered from 0 (zero) to
     * one less than the value in this field
     */
    CsrUint16 num_tohost_data_slots;

    /*
     * Size of the bulk data slots (2 octets)
     * The size of the bulk data slots in octets. This will usually be
     * the size of the largest MSDU. The value should always be even.
     */
    CsrUint16 data_slot_size;

    /*
     * Indicates that the host has finished the initialisation sequence.
     * Initialised to 0x0000 by the firmware, and set to 0x0001 by us.
     */
    CsrUint16 initialised;

    /* Added by protocol version 0x0001 */
    CsrUint32 overlay_size;

    /* Added by protocol version 0x0300 */
    CsrUint16 data_slot_round;
    CsrUint16 sig_frag_size;

    /* Added by protocol version 0x0500 */
    CsrUint16 tohost_signal_padding;
} sdio_config_data_t;

/*
 * These values may change with versions of the Host Interface Protocol.
 */
/*
 * Size of config info block pointed to by the CSR_SLT_SDIO_SLOT_CONFIG
 * entry in the f/w symbol table
 */
#define SDIO_CONFIG_DATA_SIZE 30

/* Offset of the INIT flag in the config info block. */
#define SDIO_INIT_FLAG_OFFSET 0x12
#define SDIO_TO_HOST_SIG_PADDING_OFFSET 0x1C


/* Structure for a bulk data transfer command */
typedef struct
{
    CsrUint16 cmd_and_len;   /* bits 12-15 cmd, bits 0-11 len */
    CsrUint16 data_slot;     /* slot number, perhaps OR'd with SLOT_DIR_TO_HOST */
    CsrUint16 offset;
    CsrUint16 buffer_handle;
} bulk_data_cmd_t;


/* Bulk Data signal command values */
#define SDIO_CMD_SIGNAL                 0x00
#define SDIO_CMD_TO_HOST_TRANSFER       0x01
#define SDIO_CMD_TO_HOST_TRANSFER_ACK   0x02 /*deprecated*/
#define SDIO_CMD_FROM_HOST_TRANSFER     0x03
#define SDIO_CMD_FROM_HOST_TRANSFER_ACK 0x04 /*deprecated*/
#define SDIO_CMD_CLEAR_SLOT             0x05
#define SDIO_CMD_OVERLAY_TRANSFER       0x06
#define SDIO_CMD_OVERLAY_TRANSFER_ACK   0x07 /*deprecated*/
#define SDIO_CMD_FROM_HOST_AND_CLEAR    0x08
#define SDIO_CMD_PADDING                0x0f

#define SLOT_DIR_TO_HOST 0x8000


/* Initialise bulkdata slot
 *  params:
 *      bulk_data_desc_t *bulk_data_slot
 */
#define UNIFI_INIT_BULK_DATA(bulk_data_slot)        \
    {                                               \
        (bulk_data_slot)->os_data_ptr = NULL;       \
        (bulk_data_slot)->data_length = 0;          \
        (bulk_data_slot)->os_net_buf_ptr = NULL;    \
        (bulk_data_slot)->net_buf_length = 0;       \
    }

/*
 * Structure to contain a SIGNAL datagram.
 * This is used to build signal queues between the main driver and the
 * i/o thread.
 * The fields are:
 *      sigbuf          Contains the HIP signal is wire-format (i.e. packed,
 *                      little-endian)
 *      bulkdata        Contains a copy of any associated bulk data
 *      signal_length   The size of the signal in the sigbuf
 */
typedef struct card_signal
{
    CsrUint8 sigbuf[UNIFI_PACKED_SIGBUF_SIZE];

    /* Length of the SIGNAL inside sigbuf */
    CsrUint16 signal_length;

    bulk_data_desc_t bulkdata[UNIFI_MAX_DATA_REFERENCES];
} card_signal_t;


/*
 * Control structure for a generic ring buffer.
 */
#define UNIFI_QUEUE_NAME_MAX_LENGTH     16
typedef struct
{
    card_signal_t *q_body;

    /* Num elements in queue (capacity is one less than this!) */
    CsrUint16 q_length;

    CsrUint16 q_wr_ptr;
    CsrUint16 q_rd_ptr;

    CsrCharString name[UNIFI_QUEUE_NAME_MAX_LENGTH];
} q_t;


#define UNIFI_RESERVED_COMMAND_SLOTS   2

/* Considering approx 500 us per packet giving 0.5 secs */
#define UNIFI_PACKETS_INTERVAL         1000

/*
 * Dynamic slot reservation for QoS
 */
typedef struct
{
    CsrUint16 from_host_used_slots[UNIFI_NO_OF_TX_QS];
    CsrUint16 from_host_max_slots[UNIFI_NO_OF_TX_QS];
    CsrUint16 from_host_reserved_slots[UNIFI_NO_OF_TX_QS];

    /* Parameters to determine if a queue was active.
       If number of packets sent is greater than the threshold
       for the queue, the queue is considered active and no
       re reservation is done, it is important not to keep this
       value too low */
    /* Packets sent during this interval */
    CsrUint16 packets_txed[UNIFI_NO_OF_TX_QS];
    CsrUint16 total_packets_txed;

    /* Number of packets to see if slots need to be reassigned */
    CsrUint16 packets_interval;

    /* Once a queue reaches a stable state, avoid processing */
    CsrBool queue_stable[UNIFI_NO_OF_TX_QS];
} card_dynamic_slot_t;


/* These are type-safe and don't write incorrect values to the
 * structure. */

/* Return queue slots used count
 *  params:
 *      const q_t *q
 *  returns:
 *      CsrUint16
 */
#define CSR_WIFI_HIP_Q_SLOTS_USED(q)     \
    (((q)->q_wr_ptr - (q)->q_rd_ptr < 0)? \
     ((q)->q_wr_ptr - (q)->q_rd_ptr + (q)->q_length) : ((q)->q_wr_ptr - (q)->q_rd_ptr))

/* Return queue slots free count
 *  params:
 *      const q_t *q
 *  returns:
 *      CsrUint16
 */
#define CSR_WIFI_HIP_Q_SLOTS_FREE(q)     \
    ((q)->q_length - CSR_WIFI_HIP_Q_SLOTS_USED((q)) - 1)

/* Return slot signal data pointer
 *  params:
 *      const q_t *q
 *      CsrUint16 slot
 *  returns:
 *      card_signal_t *
 */
#define CSR_WIFI_HIP_Q_SLOT_DATA(q, slot)    \
    ((q)->q_body + slot)

/* Return queue next read slot
 *  params:
 *      const q_t *q
 *  returns:
 *      CsrUint16 slot offset
 */
#define CSR_WIFI_HIP_Q_NEXT_R_SLOT(q)    \
    ((q)->q_rd_ptr)

/* Return queue next write slot
 *  params:
 *      const q_t *q
 *  returns:
 *      CsrUint16 slot offset
 */
#define CSR_WIFI_HIP_Q_NEXT_W_SLOT(q)    \
    ((q)->q_wr_ptr)

/* Return updated queue pointer wrapped around its length
 *  params:
 *      const q_t *q
 *      CsrUint16 x     amount to add to queue pointer
 *  returns:
 *      CsrUint16 wrapped queue pointer
 */
#define CSR_WIFI_HIP_Q_WRAP(q, x)    \
    ((((x) >= (q)->q_length)?((x) % (q)->q_length) : (x)))

/* Advance queue read pointer
 *  params:
 *      const q_t *q
 */
#define CSR_WIFI_HIP_Q_INC_R(q)  \
    ((q)->q_rd_ptr = CSR_WIFI_HIP_Q_WRAP((q), (q)->q_rd_ptr + 1))

/* Advance queue write pointer
 *  params:
 *      const q_t *q
 */
#define CSR_WIFI_HIP_Q_INC_W(q)  \
    ((q)->q_wr_ptr = CSR_WIFI_HIP_Q_WRAP((q), (q)->q_wr_ptr + 1))

enum unifi_host_state
{
    UNIFI_HOST_STATE_AWAKE   = 0,
    UNIFI_HOST_STATE_DROWSY  = 1,
    UNIFI_HOST_STATE_TORPID  = 2
};

typedef struct
{
    bulk_data_desc_t   bd;
    unifi_TrafficQueue queue; /* Used for dynamic slot reservation */
} slot_desc_t;

/*
 * Structure describing a UniFi SDIO card.
 */
struct card
{
    /*
     * Back pointer for the higher level OS code. This is passed as
     * an argument to callbacks (e.g. for received data and indications).
     */
    void *ospriv;

    /*
     * mapping of HIP slot to MA-PACKET.req host tag, the
     * array is indexed by slot numbers and each index stores
     * information of the last host tag it was used for
     */
    CsrUint32 *fh_slot_host_tag_record;


    /* Info read from Symbol Table during probe */
    CsrUint32     build_id;
    CsrCharString build_id_string[128];

    /* Retrieve from SDIO driver. */
    CsrUint16 chip_id;

    /* Read from GBL_CHIP_VERSION. */
    CsrUint16 chip_version;

    /* From the SDIO driver (probably 1) */
    CsrUint8 function;

    /* This is sused to get the register addresses and things. */
    ChipDescript *helper;

    /*
     * Bit mask of PIOs for the loader to waggle during download.
     * We assume these are connected to LEDs. The main firmware gets
     * the mask from a MIB entry.
     */
    CsrInt32 loader_led_mask;

    /*
     * Support for flow control. When the from-host queue of signals
     * is full, we ask the host upper layer to stop sending packets. When
     * the queue drains we tell it that it can send packets again.
     * We use this flag to remember the current state.
     */
#define card_is_tx_q_paused(card, q)   (card->tx_q_paused_flag[q])
#define card_tx_q_unpause(card, q)   (card->tx_q_paused_flag[q] = 0)
#define card_tx_q_pause(card, q)   (card->tx_q_paused_flag[q] = 1)

    CsrUint16 tx_q_paused_flag[UNIFI_TRAFFIC_Q_MAX + 1 + UNIFI_NO_OF_TX_QS]; /* defensive more than big enough */

    /* UDI callback for logging UniFi interactions */
    udi_func_t udi_hook;

    CsrUint8 bh_reason_host;
    CsrUint8 bh_reason_unifi;

    /* SDIO clock speed request from OS layer */
    CsrUint8 request_max_clock;

    /* Last SDIO clock frequency set */
    CsrUint32 sdio_clock_speed;

    /*
     * Current host state (copy of value in IOABORT register and
     * spinlock to protect it.
     */
    enum unifi_host_state host_state;

    enum unifi_low_power_mode     low_power_mode;
    enum unifi_periodic_wake_mode periodic_wake_mode;

    /*
     * Ring buffer of signal structs for a queue of data packets from
     * the host.
     * The queue is empty when fh_data_q_num_rd == fh_data_q_num_wr.
     * To add a packet to the queue, copy it to index given by
     * (fh_data_q_num_wr%UNIFI_SOFT_Q_LENGTH) and advance fh_data_q_num_wr.
     * To take a packet from the queue, copy data from index given by
     * (fh_data_q_num_rd%UNIFI_SOFT_Q_LENGTH) and advance fh_data_q_num_rd.
     * fh_data_q_num_rd and fh_data_q_num_rd are both modulo 256.
     */
    card_signal_t fh_command_q_body[UNIFI_SOFT_COMMAND_Q_LENGTH];
    q_t           fh_command_queue;

    card_signal_t fh_traffic_q_body[UNIFI_NO_OF_TX_QS][UNIFI_SOFT_TRAFFIC_Q_LENGTH];
    q_t           fh_traffic_queue[UNIFI_NO_OF_TX_QS];

    /*
     * Signal counts from UniFi SDIO Control Data Structure.
     * These are cached and synchronised with the UniFi before and after
     * a batch of operations.
     *
     * These are the modulo-256 count of signals written to or read from UniFi
     * The value is incremented for every signal.
     */
    CsrInt32 from_host_signals_w;
    CsrInt32 from_host_signals_r;
    CsrInt32 to_host_signals_r;
    CsrInt32 to_host_signals_w;


    /* Should specify buffer size as a number of signals */
    /*
     * Enough for 10 th and 10 fh data slots:
     *   1 * 10 * 8 =  80
     *   2 * 10 * 8 = 160
     */
#define UNIFI_FH_BUF_SIZE 1024
    struct sigbuf
    {
        CsrUint8 *buf;     /* buffer area */
        CsrUint8 *ptr;     /* current pos */
        CsrUint16 count;   /* signal count */
        CsrUint16 bufsize;
    } fh_buffer;
    struct sigbuf th_buffer;


    /*
     * Field to use for the incrementing value to write to the UniFi
     * SHARED_IO_INTERRUPT register.
     * Flag to say we need to generate an interrupt at end of processing.
     */
    CsrUint32 unifi_interrupt_seq;
    CsrUint8  generate_interrupt;


    /* Pointers to the bulk data slots */
    slot_desc_t      *from_host_data;
    bulk_data_desc_t *to_host_data;


    /*
     * Index of the next (hopefully) free data slot.
     * This is an optimisation that starts searching at a more likely point
     * than the beginning.
     */
    CsrInt16 from_host_data_head;

    /* Dynamic slot allocation for queues */
    card_dynamic_slot_t dynamic_slot_data;

    /*
     * SDIO specific fields
     */

    /* Interface pointer for the SDIO library */
    CsrSdioFunction *sdio_if;

    /* Copy of config_data struct from the card */
    sdio_config_data_t config_data;

    /* SDIO address of the Initialised flag and Control Data struct */
    CsrUint32 init_flag_addr;
    CsrUint32 sdio_ctrl_addr;

    /* The last value written to the Shared Data Memory Page register */
    CsrUint32 proc_select;
    CsrUint32 dmem_page;
    CsrUint32 pmem_page;

    /* SDIO traffic counters limited to 32 bits for Synergy compatibility */
    CsrUint32 sdio_bytes_read;
    CsrUint32 sdio_bytes_written;

    CsrUint8 memory_resources_allocated;

    /* UniFi SDIO I/O Block size. */
    CsrUint16 sdio_io_block_size;

    /* Pad transfer sizes to SDIO block boundaries */
    CsrBool sdio_io_block_pad;

    /* Read from the XBV */
    struct FWOV fwov;

#ifndef CSR_WIFI_HIP_TA_DISABLE
    /* TA sampling */
    ta_data_t ta_sampling;
#endif

    /* Auto-coredump */
    CsrInt16             request_coredump_on_reset; /* request coredump on next reset */
    struct coredump_buf *dump_buf;                  /* root node */
    struct coredump_buf *dump_next_write;           /* node to fill at next dump */
    struct coredump_buf *dump_cur_read;             /* valid node to read, or NULL */

#ifdef CSR_WIFI_HIP_DATA_PLANE_PROFILE
    struct cmd_profile
    {
        CsrUint32 cmd52_count;
        CsrUint32 cmd53_count;
        CsrUint32 tx_count;
        CsrUint32 tx_cfm_count;
        CsrUint32 rx_count;
        CsrUint32 bh_count;
        CsrUint32 process_count;
        CsrUint32 protocol_count;

        CsrUint32 cmd52_f0_r_count;
        CsrUint32 cmd52_f0_w_count;
        CsrUint32 cmd52_r8or16_count;
        CsrUint32 cmd52_w8or16_count;
        CsrUint32 cmd52_r16_count;
        CsrUint32 cmd52_w16_count;
        CsrUint32 cmd52_r32_count;

        CsrUint32 sdio_cmd_signal;
        CsrUint32 sdio_cmd_clear_slot;
        CsrUint32 sdio_cmd_to_host;
        CsrUint32 sdio_cmd_from_host;
        CsrUint32 sdio_cmd_from_host_and_clear;
    } hip_prof;
    struct cmd_profile cmd_prof;
#endif

    /* Interrupt processing mode flags */
    CsrUint32 intmode;

#ifdef UNIFI_DEBUG
    CsrUint8 lsb;
#endif

    /* Historic firmware panic codes */
    CsrUint32 panic_data_phy_addr;
    CsrUint32 panic_data_mac_addr;
    CsrUint16 last_phy_panic_code;
    CsrUint16 last_phy_panic_arg;
    CsrUint16 last_mac_panic_code;
    CsrUint16 last_mac_panic_arg;
#ifdef CSR_PRE_ALLOC_NET_DATA
    bulk_data_desc_t bulk_data_desc_list[BULK_DATA_PRE_ALLOC_NUM];
    CsrUint16        prealloc_netdata_r;
    CsrUint16        prealloc_netdata_w;
#endif
}; /* struct card */


/* Reset types */
enum unifi_reset_type
{
    UNIFI_COLD_RESET = 1,
    UNIFI_WARM_RESET = 2
};

/*
 * unifi_set_host_state() implements signalling for waking UniFi from
 * deep sleep. The host indicates to UniFi that it is in one of three states:
 *   Torpid - host has nothing to send, UniFi can go to sleep.
 *   Drowsy - host has data to send to UniFi. UniFi will respond with an
 *            SDIO interrupt. When hosts responds it moves to Awake.
 *   Awake  - host has data to transfer, UniFi must stay awake.
 *            When host has finished, it moves to Torpid.
 */
CsrResult unifi_set_host_state(card_t *card, enum unifi_host_state state);


CsrResult unifi_set_proc_select(card_t *card, enum unifi_dbg_processors_select select);
CsrInt32 card_read_signal_counts(card_t *card);
bulk_data_desc_t* card_find_data_slot(card_t *card, CsrInt16 slot);


CsrResult unifi_read32(card_t *card, CsrUint32 unifi_addr, CsrUint32 *pdata);
CsrResult unifi_readnz(card_t *card, CsrUint32 unifi_addr,
                       void *pdata, CsrUint16 len);
CsrInt32 unifi_read_shared_count(card_t *card, CsrUint32 addr);

CsrResult unifi_writen(card_t *card, CsrUint32 unifi_addr, void *pdata, CsrUint16 len);

CsrResult unifi_bulk_rw(card_t *card, CsrUint32 handle,
                        void *pdata, CsrUint32 len, CsrInt16 direction);
CsrResult unifi_bulk_rw_noretry(card_t *card, CsrUint32 handle,
                                void *pdata, CsrUint32 len, CsrInt16 direction);
#define UNIFI_SDIO_READ       0
#define UNIFI_SDIO_WRITE      1

CsrResult unifi_read_8_or_16(card_t *card, CsrUint32 unifi_addr, CsrUint8 *pdata);
CsrResult unifi_write_8_or_16(card_t *card, CsrUint32 unifi_addr, CsrUint8 data);
CsrResult unifi_read_direct_8_or_16(card_t *card, CsrUint32 addr, CsrUint8 *pdata);
CsrResult unifi_write_direct_8_or_16(card_t *card, CsrUint32 addr, CsrUint8 data);

CsrResult unifi_read_direct16(card_t *card, CsrUint32 addr, CsrUint16 *pdata);
CsrResult unifi_read_direct32(card_t *card, CsrUint32 addr, CsrUint32 *pdata);
CsrResult unifi_read_directn(card_t *card, CsrUint32 addr, void *pdata, CsrUint16 len);

CsrResult unifi_write_direct16(card_t *card, CsrUint32 addr, CsrUint16 data);
CsrResult unifi_write_directn(card_t *card, CsrUint32 addr, void *pdata, CsrUint16 len);

CsrResult sdio_read_f0(card_t *card, CsrUint32 addr, CsrUint8 *pdata);
CsrResult sdio_write_f0(card_t *card, CsrUint32 addr, CsrUint8 data);

void unifi_read_panic(card_t *card);
#ifdef CSR_PRE_ALLOC_NET_DATA
void prealloc_netdata_free(card_t *card);
CsrResult prealloc_netdata_alloc(card_t *card);
#endif
/* For diagnostic use */
void dump(void *mem, CsrUint16 len);
void dump16(void *mem, CsrUint16 len);

#ifdef __cplusplus
}
#endif

#endif /* __CARD_SDIO_H__ */
