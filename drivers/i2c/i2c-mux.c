/*
 * Multiplexed I2C bus driver.
 *
 * Copyright (c) 2008-2009 Rodolfo Giometti <giometti@linux.it>
 * Copyright (c) 2008-2009 Eurotech S.p.A. <info@eurotech.it>
 * Copyright (c) 2009-2010 NSN GmbH & Co KG <michael.lawnick.ext@nsn.com>
 *
 * Simplifies access to complex multiplexed I2C bus topologies, by presenting
 * each multiplexed bus segment as an additional I2C adapter.
 * Supports multi-level mux'ing (mux behind a mux).
 *
 * Based on:
 *	i2c-virt.c from Kumar Gala <galak@kernel.crashing.org>
 *	i2c-virtual.c from Ken Harrenstien, Copyright (c) 2004 Google, Inc.
 *	i2c-virtual.c from Brian Kuschak <bkuschak@yahoo.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/of.h>
#include <linux/of_i2c.h>

/* multiplexer per channel data */
struct i2c_mux_priv_old {
	void *mux_priv;
	int (*select)(struct i2c_adapter *, void *mux_priv, u32 chan_id);
	int (*deselect)(struct i2c_adapter *, void *mux_priv, u32 chan_id);
};

struct i2c_mux_priv {
	struct i2c_adapter adap;
	struct i2c_algorithm algo;
	struct i2c_mux_core *muxc;
	u32 chan_id;
};

static int i2c_mux_master_xfer(struct i2c_adapter *adap,
			       struct i2c_msg msgs[], int num)
{
	struct i2c_mux_priv *priv = adap->algo_data;
	struct i2c_mux_core *muxc = priv->muxc;
	struct i2c_adapter *parent = muxc->parent;
	int ret;

	/* Switch to the right mux port and perform the transfer. */

	ret = muxc->select(muxc, priv->chan_id);
	if (ret >= 0)
		ret = __i2c_transfer(parent, msgs, num);
	if (muxc->deselect)
		muxc->deselect(muxc, priv->chan_id);

	return ret;
}

static int i2c_mux_smbus_xfer(struct i2c_adapter *adap,
			      u16 addr, unsigned short flags,
			      char read_write, u8 command,
			      int size, union i2c_smbus_data *data)
{
	struct i2c_mux_priv *priv = adap->algo_data;
	struct i2c_mux_core *muxc = priv->muxc;
	struct i2c_adapter *parent = muxc->parent;
	int ret;

	/* Select the right mux port and perform the transfer. */

	ret = muxc->select(muxc, priv->chan_id);
	if (ret >= 0)
		ret = parent->algo->smbus_xfer(parent, addr, flags,
					read_write, command, size, data);
	if (muxc->deselect)
		muxc->deselect(muxc, priv->chan_id);

	return ret;
}

/* Return the parent's functionality */
static u32 i2c_mux_functionality(struct i2c_adapter *adap)
{
	struct i2c_mux_priv *priv = adap->algo_data;
	struct i2c_adapter *parent = priv->muxc->parent;

	return parent->algo->functionality(parent);
}

/* Return all parent classes, merged */
static unsigned int i2c_mux_parent_classes(struct i2c_adapter *parent)
{
	unsigned int class = 0;

	do {
		class |= parent->class;
		parent = i2c_parent_is_i2c_adapter(parent);
	} while (parent);

	return class;
}

static int i2c_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
	struct i2c_mux_priv_old *priv = i2c_mux_priv(muxc);

	return priv->select(muxc->parent, priv->mux_priv, chan);
}

static int i2c_mux_deselect(struct i2c_mux_core *muxc, u32 chan)
{
	struct i2c_mux_priv_old *priv = i2c_mux_priv(muxc);

	return priv->deselect(muxc->parent, priv->mux_priv, chan);
}

struct i2c_adapter *i2c_add_mux_adapter(struct i2c_adapter *parent,
				struct device *mux_dev,
				void *mux_priv, u32 force_nr, u32 chan_id,
				unsigned int class,
				int (*select) (struct i2c_adapter *,
					       void *, u32),
				int (*deselect) (struct i2c_adapter *,
						 void *, u32))
{
	struct i2c_mux_core *muxc;
	struct i2c_mux_priv_old *priv;
	int ret;

	muxc = i2c_mux_alloc(parent, mux_dev, 1, sizeof(*priv), 0,
			     i2c_mux_select, i2c_mux_deselect);
	if (!muxc)
		return NULL;

	priv = i2c_mux_priv(muxc);
	priv->select = select;
	priv->deselect = deselect;
	priv->mux_priv = mux_priv;

	ret = i2c_mux_add_adapter(muxc, force_nr, chan_id, class);
	if (ret) {
		devm_kfree(mux_dev, muxc);
		return ret;
	}

	WARN(sysfs_create_link(&priv->adap.dev.kobj, &muxc->dev->kobj,
			       "mux_device"),
	     "can't create symlink to mux device\n");

	snprintf(symlink_name, sizeof(symlink_name), "channel-%u", chan_id);
	WARN(sysfs_create_link(&muxc->dev->kobj, &priv->adap.dev.kobj,
			       symlink_name),
	     "can't create symlink for channel %u\n", chan_id);
	dev_info(&parent->dev, "Added multiplexed i2c bus %d\n",
		 i2c_adapter_id(&priv->adap));

	muxc->adapter[muxc->num_adapters++] = &priv->adap;
	return 0;
}
EXPORT_SYMBOL_GPL(i2c_mux_add_adapter);

void i2c_mux_del_adapters(struct i2c_mux_core *muxc)
{
	char symlink_name[20];

	while (muxc->num_adapters) {
		struct i2c_adapter *adap = muxc->adapter[--muxc->num_adapters];
		struct i2c_mux_priv *priv = adap->algo_data;

		muxc->adapter[muxc->num_adapters] = NULL;

		snprintf(symlink_name, sizeof(symlink_name),
			 "channel-%u", priv->chan_id);
		sysfs_remove_link(&muxc->dev->kobj, symlink_name);

		sysfs_remove_link(&priv->adap.dev.kobj, "mux_device");
		i2c_del_adapter(adap);
		kfree(priv);
	}
}
EXPORT_SYMBOL_GPL(i2c_mux_del_adapters);

void i2c_del_mux_adapter(struct i2c_adapter *adap)
{
	struct i2c_mux_priv *priv = adap->algo_data;
	struct i2c_mux_core *muxc = priv->muxc;

	i2c_mux_del_adapters(muxc);
	devm_kfree(muxc->dev, muxc);
}
EXPORT_SYMBOL_GPL(i2c_del_mux_adapter);

MODULE_AUTHOR("Rodolfo Giometti <giometti@linux.it>");
MODULE_DESCRIPTION("I2C driver for multiplexed I2C busses");
MODULE_LICENSE("GPL v2");
