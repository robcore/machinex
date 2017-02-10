#ifndef __UAPI_TTS_COMMON_H__
#define __UAPI_TTS_COMMON_H__


#define	TTS_ERR_LOG(...)	snprintf(tts_err_log,TTS_MAX_RES_LEN,__VA_ARGS__)
#define DEBUG_TIMATEST
#ifdef  DEBUG_TIMATEST 
#define	TTS_DBG_LOG(...)	printk(__VA_ARGS__)
#else
#define	TTS_DBG_LOG(...)
#endif

#define TTS_TC_RET_SUCCESS  0xf0f0
#define TTS_TC_RET_FAILURE  0xf1f1

#define UTEST_TC_KERNEL       0xabcd
#define TTS_MAX_RES_LEN       1024
#endif/*__UAPI_TTS_COMMON_H__*/
