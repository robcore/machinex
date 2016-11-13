/*
 * dek_ioctl.h
 *
 *  Created on: Jul 4, 2014
 *      Author: olic
 */

#ifndef DEK_IOCTL_H_
#define DEK_IOCTL_H_

#include <sdp/dek_common.h>
#define __DEKIOC		        0x77

typedef struct _dek_arg_generate_dek {
	int userid;
	dek_t dek;
}dek_arg_generate_dek;

typedef struct _dek_arg_encrypt_dek {
	int userid;
	dek_t plain_dek;
	dek_t enc_dek;
}dek_arg_encrypt_dek;

typedef struct _dek_arg_decrypt_dek {
	int userid;
	dek_t plain_dek;
	dek_t enc_dek;
}dek_arg_decrypt_dek;

typedef struct _dek_arg_get_kek {
	int userid;
	int kek_type;
	kek_t key;
}dek_arg_get_kek;

/*
 * DEK_ON_BOOT indicates that there's persona in the system.
 *
 * The driver will load public key and encrypted private key.
 */
typedef struct _dek_arg_on_boot {
	int userid;
	kek_t SDPK_Rpub;
	kek_t SDPK_Dpub;
}dek_arg_on_boot;

typedef struct _dek_arg_on_device_locked {
	int userid;
}dek_arg_on_device_locked;

typedef struct _dek_arg_on_device_unlocked {
	int userid;
	kek_t SDPK_Rpri;
	kek_t SDPK_Dpri;
	kek_t SDPK_sym;
}dek_arg_on_device_unlocked;

typedef struct _dek_arg_on_user_added {
	int userid;
	kek_t SDPK_Rpub;
	kek_t SDPK_Dpub;
}dek_arg_on_user_added;

typedef struct _dek_arg_on_user_removed {
	int userid;
}dek_arg_on_user_removed, dek_arg_disk_cache_cleanup;

// SDP driver events
#define DEK_ON_BOOT              _IOW(__DEKIOC, 0, unsigned int)
#define DEK_ON_DEVICE_LOCKED     _IOW(__DEKIOC, 4, unsigned int)
#define DEK_ON_DEVICE_UNLOCKED   _IOW(__DEKIOC, 5, unsigned int)
#define DEK_ON_USER_ADDED        _IOW(__DEKIOC, 6, unsigned int)
#define DEK_ON_USER_REMOVED      _IOW(__DEKIOC, 7, unsigned int)
#define DEK_ON_CHANGE_PASSWORD   _IOW(__DEKIOC, 8, unsigned int)  // @Deprecated

// SDP driver DEK requests
#define DEK_GENERATE_DEK         _IOW(__DEKIOC, 1, unsigned int)
#define DEK_ENCRYPT_DEK          _IOW(__DEKIOC, 2, unsigned int)
#define DEK_DECRYPT_DEK          _IOR(__DEKIOC, 3, unsigned int)
#define DEK_GET_KEK         	 _IOW(__DEKIOC, 9, unsigned int)
#define DEK_DISK_CACHE_CLEANUP   _IOW(__DEKIOC, 10, unsigned int)

#endif /* DEK_IOCTL_H_ */
