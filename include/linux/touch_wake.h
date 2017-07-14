/* include/linux/touch_wake.h */

#ifndef _LINUX_TOUCH_WAKE_H
#define _LINUX_TOUCH_WAKE_H

#include <linux/input.h>
#include "../../drivers/sensorhub/stm32f/ssp.h"

#define TOUCHWAKE_DEBUG_PRINT

#define TOUCHWAKE_VERSION "1.5 by Yank555.lu"
#define TIME_LONGPRESS 500
#define POWERPRESS_DELAY 60
#define POWERPRESS_TIMEOUT 1000

void powerkey_pressed(void);
void powerkey_released(void);
void touch_press(void);
bool device_is_suspended(void);
bool touchwake_is_active(void);

#endif
