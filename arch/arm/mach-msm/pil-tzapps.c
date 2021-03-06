/* Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/elf.h>
#include <linux/err.h>

#include "peripheral-loader.h"
#include "scm-pas.h"

static int pil_tzapps_init_image(struct pil_desc *pil, const u8 *metadata,
		size_t size)
{
	return pas_init_image(PAS_TZAPPS, metadata, size);
}

static int pil_tzapps_reset(struct pil_desc *pil)
{
	return pas_auth_and_reset(PAS_TZAPPS);
}

static int pil_tzapps_shutdown(struct pil_desc *pil)
{
	return pas_shutdown(PAS_TZAPPS);
}

static struct pil_reset_ops pil_tzapps_ops = {
	.init_image = pil_tzapps_init_image,
	.auth_and_reset = pil_tzapps_reset,
	.shutdown = pil_tzapps_shutdown,
};

static int pil_tzapps_driver_probe(struct platform_device *pdev)
{
	struct pil_desc *desc;
	struct pil_device *pil;

	if (pas_supported(PAS_TZAPPS) < 0)
		return -ENOSYS;

	desc = devm_kzalloc(&pdev->dev, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	desc->name = "tzapps";
	desc->dev = &pdev->dev;
	desc->ops = &pil_tzapps_ops;
	desc->owner = THIS_MODULE;
	pil = msm_pil_register(desc);
	if (IS_ERR(pil)) {
		devm_kfree(&pdev->dev, desc);
		return PTR_ERR(pil);
	}
	platform_set_drvdata(pdev, pil);
	return 0;
}

static int pil_tzapps_driver_exit(struct platform_device *pdev)
{
	struct pil_device *pil = platform_get_drvdata(pdev);
	msm_pil_unregister(pil);
	return 0;
}

static struct platform_driver pil_tzapps_driver = {
	.probe = pil_tzapps_driver_probe,
	.remove = pil_tzapps_driver_exit,
	.driver = {
		.name = "pil_tzapps",
		.owner = THIS_MODULE,
	},
};

static int __init pil_tzapps_init(void)
{
	return platform_driver_register(&pil_tzapps_driver);
}
module_init(pil_tzapps_init);

static void __exit pil_tzapps_exit(void)
{
	platform_driver_unregister(&pil_tzapps_driver);
}
module_exit(pil_tzapps_exit);

MODULE_DESCRIPTION("Support for booting TZApps images");
MODULE_LICENSE("GPL v2");
