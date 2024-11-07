// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <i2c.h>
#include <linux/err.h>

struct lt9711_priv {
	unsigned int addr;
	unsigned int addr_cec;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	struct udevice *cec_dev;
};
/*
static const struct display_timing default_timing = {
	.pixelclock.typ		= 148500000,
	.hactive.typ		= 1920,
	.hfront_porch.typ	= 88,
	.hback_porch.typ	= 148,
	.hsync_len.typ		= 44,
	.vactive.typ		= 1080,
	.vfront_porch.typ	= 4,
	.vback_porch.typ	= 36,
	.vsync_len.typ		= 5,
};
*/


static const struct display_timing default_timing = {
	.pixelclock.typ		= 76300000,//148500000,
	.hactive.typ		= 1366,
	.hfront_porch.typ	= 48,
	.hback_porch.typ	= 146,
	.hsync_len.typ		= 32,
	.vactive.typ		= 768,
	.vfront_porch.typ	= 3,
	.vback_porch.typ	= 15,
	.vsync_len.typ		= 6,
};

/*
static int lt9711_i2c_reg_write(struct udevice *dev, uint addr, uint mask, uint data)
{
	uint8_t valb;
	int err;

	if (mask != 0xff) {
		err = dm_i2c_read(dev, addr, &valb, 1);
		if (err)
			return err;

		valb &= ~mask;
		valb |= data;
	} else {
		valb = data;
	}

	err = dm_i2c_write(dev, addr, &valb, 1);
	return err;
}

static int lt9711_i2c_reg_read(struct udevice *dev, uint8_t addr, uint8_t *data)
{
	uint8_t valb;
	int err;

	err = dm_i2c_read(dev, addr, &valb, 1);
	if (err)
		return err;

	*data = (int)valb;
	return 0;
}
*/
static int lt9711_enable(struct udevice *dev)
{
//	struct lt9711_priv *priv = dev_get_priv(dev);
	uint8_t val;

//	lt9711_i2c_reg_read(dev, 0x00, &val);
	debug("Chip revision: 0x%x (expected: 0x14)\n", val);
//	lt9711_i2c_reg_read(priv->cec_dev, 0x00, &val);
	debug("Chip ID MSB: 0x%x (expected: 0x75)\n", val);
//	lt9711_i2c_reg_read(priv->cec_dev, 0x01, &val);
	debug("Chip ID LSB: 0x%x (expected: 0x33)\n", val);

	return 0;
}

static int lt9711_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
		return ret;

	return 0;
}

static int lt9711_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	struct lt9711_priv *priv = dev_get_priv(dev);

	memcpy(timings, &default_timing, sizeof(*timings));

	/* fill characteristics of DSI data link */
	if (device) {
		device->lanes = priv->lanes;
		device->format = priv->format;
		device->mode_flags = priv->mode_flags;
	}

	return 0;
}

static int lt9711_probe(struct udevice *dev)
{
	struct lt9711_priv *priv = dev_get_priv(dev);
	int ret;

	priv->format = MIPI_DSI_FMT_RGB888;
	priv->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
				MIPI_DSI_MODE_VIDEO_HSE;

	priv->addr  = dev_read_addr(dev);
	if (priv->addr  == 0){
		dev_err(dev, "Failed to probe\n");
		return -ENODEV;
	}

	ret = dev_read_u32(dev, "lt,dsi-lanes", &priv->lanes);
	if (ret) {
		dev_err(dev, "Failed to get dsi-lanes property (%d)\n", ret);
		return ret;
	}

	if (priv->lanes < 1 || priv->lanes > 4) {
		dev_err(dev, "Invalid dsi-lanes: %d\n", priv->lanes);
		return -EINVAL;
	}

	lt9711_enable(dev);

	return 0;
}

static const struct panel_ops lt9711_ops = {
	.enable_backlight = lt9711_enable_backlight,
	.get_display_timing = lt9711_get_display_timing,
};

static const struct udevice_id lt9711_ids[] = {
	{ .compatible = "lontium,lt9711" },
	{ }
};

U_BOOT_DRIVER(lt9711_mipi2hdmi) = {
	.name			  = "lt9711_mipi2hdmi",
	.id			  = UCLASS_PANEL,
	.of_match		  = lt9711_ids,
	.ops			  = &lt9711_ops,
	.probe			  = lt9711_probe,
	.plat_auto = sizeof(struct mipi_dsi_panel_plat),
	.priv_auto	= sizeof(struct lt9711_priv),
};
