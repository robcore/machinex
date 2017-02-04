#ifndef __LINUX_QSEECOM_H_
#define __LINUX_QSEECOM_H_

#include <uapi/linux/qseecom.h>

#ifdef  TIMA_ENABLED
/* lkmauth functions */
int qseecom_k_open(void);

int qseecom_k_release(void);

int qseecom_k_check_tzapps(void);

//int qseecom_k_unload_app();

int qseecom_k_send_cmd(void *argp);

//int __qseecom_k_process_incomplete_cmd(void *tmpdata, void *tmpresp);
#endif

#endif /* __QSEECOM_H_ */
