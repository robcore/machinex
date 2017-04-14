/*
 * Runtime Override Control for wakeup status of Samsung Galaxy S4 (qcom) gpio keys.
 * Hooks into platform data for the gpio_keys.c driver. Useless without hooking the
 * ".wakeup" platform data. See arch/arm/mach-msm/board-jf_eur.c for implementation.
 *
 * Copyright 2017 Rob Patershuk (@robcore)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/machinex_wakeup_keys.h>

/* These become the actual control for the
 * wake status of the volume/home keys. Thus,
 * we keep them at their original values by
 * default
 */
static bool volume_key_override = false;
module_param_named(vol_key_wake, volume_key_override, bool, 0644);

static bool home_key_override = true;
(home_key_wake, home_key_override, bool, 0644);

/* The below functions simply return the values set above */
bool is_volume_wake()
{
	return volume_key_override;
}

bool is_home_wake()
{
	return home_key_override;
}

/* A dummy init method for now that will be updated as
 * as the driver is refined and matures.
*/
static void dummy_init(void);
{
	pr_info("[MACHINEX] Key Override Module Init Success!\n");
}

late_initcall(dummy_init);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("Wake control Override for GPIO Keys");
