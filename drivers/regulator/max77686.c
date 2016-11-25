/*
 * max77686.c - Regulator driver for the Maxim 77686
 *
 * Copyright (C) 2012 Samsung Electronics
 * Chiwoong Byun <woong.byun@smasung.com>
 * Jonghwa Lee <jonghwa3.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This driver is based on max8997.c
 */

#include <linux/kernel.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/max77686.h>
#include <linux/mfd/max77686-private.h>

#define MAX77686_LDO_MINUV	800000
#define MAX77686_LDO_UVSTEP	50000
#define MAX77686_LDO_LOW_MINUV	800000
#define MAX77686_LDO_LOW_UVSTEP	25000
#define MAX77686_BUCK_MINUV	750000
#define MAX77686_BUCK_UVSTEP	50000
#define MAX77686_DVS_MINUV	600000
#define MAX77686_DVS_UVSTEP	12500

#define MAX77686_OPMODE_SHIFT	6
#define MAX77686_OPMODE_BUCK234_SHIFT	4
#define MAX77686_OPMODE_MASK	0x3

#define MAX77686_VSEL_MASK	0x3F
#define MAX77686_DVS_VSEL_MASK	0xFF

#define MAX77686_RAMP_RATE_MASK	0xC0

#define MAX77686_REGULATORS	MAX77686_REG_MAX
#define MAX77686_LDOS		26

enum max77686_ramp_rate {
	RAMP_RATE_13P75MV,
	RAMP_RATE_27P5MV,
	RAMP_RATE_55MV,
	RAMP_RATE_NO_CTRL,	/* 100mV/us */
};

struct max77686_data {
	struct device *dev;
	struct max77686_dev *iodev;
	struct regulator_dev **rdev;
	int ramp_delay; /* in mV/us */
};

static int max77686_set_dvs_voltage_time_sel(struct regulator_dev *rdev,
			unsigned int old_selector, unsigned int new_selector)
{
	struct max77686_data *max77686 = rdev_get_drvdata(rdev);
	int ramp_rate[] = {13, 27, 55, 100};
	return (DIV_ROUND_UP(rdev->desc->uV_step
			* abs(new_selector - old_selector),
			ramp_rate[max77686->ramp_delay]));
}

static int max77686_set_voltage_time_sel(struct regulator_dev *rdev,
			unsigned int old_selector, unsigned int new_selector)
{
	/* Unconditionally 100 mV/us */
	return (DIV_ROUND_UP(rdev->desc->uV_step
		 * abs(new_selector - old_selector), 100));
}

static struct regulator_ops max77686_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.set_voltage_time_sel	= max77686_set_voltage_time_sel,
};

static struct regulator_ops max77686_buck_dvs_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.set_voltage_time_sel	= max77686_set_dvs_voltage_time_sel,
};

#define regulator_desc_ldo(num)		{				\
	.name		= "LDO"#num,					\
	.id		= MAX77686_LDO##num,				\
	.ops		= &max77686_ops,				\
	.type		= REGULATOR_VOLTAGE,				\
	.owner		= THIS_MODULE,					\
	.min_uV		= MAX77686_LDO_MINUV,				\
	.uV_step	= MAX77686_LDO_UVSTEP,				\
	.n_voltages	= MAX77686_VSEL_MASK + 1,			\
	.vsel_reg	= MAX77686_REG_LDO1CTRL1 + num - 1,		\
	.vsel_mask	= MAX77686_VSEL_MASK,				\
	.enable_reg	= MAX77686_REG_LDO1CTRL1 + num - 1,		\
	.enable_mask	= MAX77686_OPMODE_MASK				\
			<< MAX77686_OPMODE_SHIFT,			\
}
#define regulator_desc_ldo_low(num)		{			\
	.name		= "LDO"#num,					\
	.id		= MAX77686_LDO##num,				\
	.ops		= &max77686_ops,				\
	.type		= REGULATOR_VOLTAGE,				\
	.owner		= THIS_MODULE,					\
	.min_uV		= MAX77686_LDO_LOW_MINUV,			\
	.uV_step	= MAX77686_LDO_LOW_UVSTEP,			\
	.n_voltages	= MAX77686_VSEL_MASK + 1,			\
	.vsel_reg	= MAX77686_REG_LDO1CTRL1 + num - 1,		\
	.vsel_mask	= MAX77686_VSEL_MASK,				\
	.enable_reg	= MAX77686_REG_LDO1CTRL1 + num - 1,		\
	.enable_mask	= MAX77686_OPMODE_MASK				\
			<< MAX77686_OPMODE_SHIFT,			\
}
#define regulator_desc_buck(num)		{			\
	.name		= "BUCK"#num,					\
	.id		= MAX77686_BUCK##num,				\
	.ops		= &max77686_ops,				\
	.type		= REGULATOR_VOLTAGE,				\
	.owner		= THIS_MODULE,					\
	.min_uV		= MAX77686_BUCK_MINUV,				\
	.uV_step	= MAX77686_BUCK_UVSTEP,				\
	.n_voltages	= MAX77686_VSEL_MASK + 1,			\
	.vsel_reg	= MAX77686_REG_BUCK5OUT + (num - 5) * 2,	\
	.vsel_mask	= MAX77686_VSEL_MASK,				\
	.enable_reg	= MAX77686_REG_BUCK5CTRL + (num - 5) * 2,	\
	.enable_mask	= MAX77686_OPMODE_MASK,				\
}
#define regulator_desc_buck1(num)		{			\
	.name		= "BUCK"#num,					\
	.id		= MAX77686_BUCK##num,				\
	.ops		= &max77686_ops,				\
	.type		= REGULATOR_VOLTAGE,				\
	.owner		= THIS_MODULE,					\
	.min_uV		= MAX77686_BUCK_MINUV,				\
	.uV_step	= MAX77686_BUCK_UVSTEP,				\
	.n_voltages	= MAX77686_VSEL_MASK + 1,			\
	.vsel_reg	= MAX77686_REG_BUCK1OUT,			\
	.vsel_mask	= MAX77686_VSEL_MASK,				\
	.enable_reg	= MAX77686_REG_BUCK1CTRL,			\
	.enable_mask	= MAX77686_OPMODE_MASK,				\
}
#define regulator_desc_buck_dvs(num)		{			\
	.name		= "BUCK"#num,					\
	.id		= MAX77686_BUCK##num,				\
	.ops		= &max77686_buck_dvs_ops,				\
	.type		= REGULATOR_VOLTAGE,				\
	.owner		= THIS_MODULE,					\
	.min_uV		= MAX77686_DVS_MINUV,				\
	.uV_step	= MAX77686_DVS_UVSTEP,				\
	.n_voltages	= MAX77686_DVS_VSEL_MASK + 1,			\
	.vsel_reg	= MAX77686_REG_BUCK2DVS1 + (num - 2) * 10,	\
	.vsel_mask	= MAX77686_DVS_VSEL_MASK,			\
	.enable_reg	= MAX77686_REG_BUCK2CTRL1 + (num - 2) * 10,	\
	.enable_mask	= MAX77686_OPMODE_MASK				\
			<< MAX77686_OPMODE_BUCK234_SHIFT,		\
}

static struct regulator_desc regulators[] = {
	regulator_desc_ldo_low(1),
	regulator_desc_ldo_low(2),
	regulator_desc_ldo(3),
	regulator_desc_ldo(4),
	regulator_desc_ldo(5),
	regulator_desc_ldo_low(6),
	regulator_desc_ldo_low(7),
	regulator_desc_ldo_low(8),
	regulator_desc_ldo(9),
	regulator_desc_ldo(10),
	regulator_desc_ldo(11),
	regulator_desc_ldo(12),
	regulator_desc_ldo(13),
	regulator_desc_ldo(14),
	regulator_desc_ldo_low(15),
	regulator_desc_ldo(16),
	regulator_desc_ldo(17),
	regulator_desc_ldo(18),
	regulator_desc_ldo(19),
	regulator_desc_ldo(20),
	regulator_desc_ldo(21),
	regulator_desc_ldo(22),
	regulator_desc_ldo(23),
	regulator_desc_ldo(24),
	regulator_desc_ldo(25),
	regulator_desc_ldo(26),
	regulator_desc_buck1(1),
	regulator_desc_buck_dvs(2),
	regulator_desc_buck_dvs(3),
	regulator_desc_buck_dvs(4),
	regulator_desc_buck(5),
	regulator_desc_buck(6),
	regulator_desc_buck(7),
	regulator_desc_buck(8),
	regulator_desc_buck(9),
};

static __devinit int max77686_pmic_probe(struct platform_device *pdev)
{
	struct max77686_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct max77686_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct regulator_dev **rdev;
	struct max77686_data *max77686;
	int i,  size;
	int ret = 0;
	struct regulator_config config;

	dev_dbg(&pdev->dev, "%s\n", __func__);

	if (!pdata || pdata->num_regulators != MAX77686_REGULATORS) {
		dev_err(&pdev->dev,
			"Invalid initial data for regulator's initialiation\n");
		return -EINVAL;
	}

	max77686 = devm_kzalloc(&pdev->dev, sizeof(struct max77686_data),
				GFP_KERNEL);
	if (!max77686)
		return -ENOMEM;

	size = sizeof(struct regulator_dev *) * MAX77686_REGULATORS;
	max77686->rdev = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);
	if (!max77686->rdev)
		return -ENOMEM;

	rdev = max77686->rdev;
	max77686->dev = &pdev->dev;
	max77686->iodev = iodev;
	platform_set_drvdata(pdev, max77686);

	max77686->ramp_delay = RAMP_RATE_NO_CTRL; /* Set 0x3 for RAMP */
	regmap_update_bits(max77686->iodev->regmap,
			MAX77686_REG_BUCK2CTRL1, MAX77686_RAMP_RATE_MASK,
			max77686->ramp_delay << 6);
	regmap_update_bits(max77686->iodev->regmap,
			MAX77686_REG_BUCK3CTRL1, MAX77686_RAMP_RATE_MASK,
			max77686->ramp_delay << 6);
	regmap_update_bits(max77686->iodev->regmap,
			MAX77686_REG_BUCK4CTRL1, MAX77686_RAMP_RATE_MASK,
			max77686->ramp_delay << 6);

	for (i = 0; i < MAX77686_REGULATORS; i++) {
		config.dev = max77686->dev;
		config.regmap = iodev->regmap;
		config.driver_data = max77686;
		config.init_data = pdata->regulators[i].initdata;

		rdev[i] = regulator_register(&regulators[i], &config);
		if (IS_ERR(rdev[i])) {
			ret = PTR_ERR(rdev[i]);
			dev_err(max77686->dev,
				"regulator init failed for %d\n", i);
				rdev[i] = NULL;
				goto err;
		}
	}

	return 0;
err:
	while (--i >= 0)
		regulator_unregister(rdev[i]);
	return ret;
}

static int __devexit max77686_pmic_remove(struct platform_device *pdev)
{
	struct max77686_data *max77686 = platform_get_drvdata(pdev);
	struct regulator_dev **rdev = max77686->rdev;
	int i;

	for (i = 0; i < MAX77686_REGULATORS; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);

	return 0;
}

static const struct platform_device_id max77686_pmic_id[] = {
	{"max77686-pmic", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, max77686_pmic_id);

static struct platform_driver max77686_pmic_driver = {
	.driver = {
		.name = "max77686-pmic",
		.owner = THIS_MODULE,
	},
	.probe = max77686_pmic_probe,
	.remove = __devexit_p(max77686_pmic_remove),
	.id_table = max77686_pmic_id,
};

static int __init max77686_pmic_init(void)
{
	return platform_driver_register(&max77686_pmic_driver);
}
subsys_initcall(max77686_pmic_init);

static void __exit max77686_pmic_cleanup(void)
{
	platform_driver_unregister(&max77686_pmic_driver);
}
module_exit(max77686_pmic_cleanup);

MODULE_DESCRIPTION("MAXIM 77686 Regulator Driver");
MODULE_AUTHOR("Chiwoong Byun <woong.byun@samsung.com>");
MODULE_LICENSE("GPL");
