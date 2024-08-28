// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2020 Silmates
 */

#include <dm.h>
#include <i2c_eeprom.h>
#include <asm/global_data.h>
#include <linux/errno.h>
#include "i2c_common.h"
#include <env.h>
#include <i2c.h>

#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN	1

DECLARE_GLOBAL_DATA_PTR;

#define I2C_EEPROM_ADDRESS		0x50

static int get_sm_eeprom(u32 eeprom_id, struct udevice **devp)
{
/*
	int ret = 0;
	int node;
	ofnode eeprom;
	char eeprom_str[16];
	const char *path;

	if (!gd->fdt_blob) {
		printf("%s: don't have a valid gd->fdt_blob!\n", __func__);
		return -EFAULT;
	}

	node = fdt_path_offset(gd->fdt_blob, "/aliases");
	if (node < 0)
		return -ENODEV;

	sprintf(eeprom_str, "eeprom%d", eeprom_id);

	path = fdt_getprop(gd->fdt_blob, node, eeprom_str, NULL);
	if (!path) {
		printf("%s: no alias for %s\n", __func__, eeprom_str);
		return -ENODEV;
	}

	eeprom = ofnode_path(path);
	if (!ofnode_valid(eeprom)) {
		printf("%s: invalid hardware path to EEPROM\n", __func__);
		return -ENODEV;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_I2C_EEPROM, eeprom, devp);
	if (ret) {
		printf("%s: cannot find EEPROM by node\n", __func__);
		return ret;
	}
	return ret;
*/
}

int read_sm_eeprom_data(u32 eeprom_id, int offset, u8 *buf,
			 int size)
{

#if !CONFIG_IS_ENABLED(DM_I2C)
	i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, offset, CONFIG_SYS_I2C_EEPROM_ADDR_LEN,
		buf, size);
#else
	struct udevice *dev;
	int ret;
#ifdef CONFIG_SYS_EEPROM_BUS_NUM
	ret = i2c_get_chip_for_busnum(CONFIG_SYS_EEPROM_BUS_NUM,
				      CONFIG_SYS_I2C_EEPROM_ADDR,
				      CONFIG_SYS_I2C_EEPROM_ADDR_LEN,
				      &dev);
#else
	ret = i2c_get_chip_for_busnum(0, CONFIG_SYS_I2C_EEPROM_ADDR,
				      CONFIG_SYS_I2C_EEPROM_ADDR_LEN,
				      &dev);
#endif
	if (!ret)
		dm_i2c_read(dev, offset, buf, size);
#endif

	printf("read offset : 0x%02x  size : %d \n", offset, size);
	for(ret = 0 ; ret < size ; ret++)
	{
		printf("0x%02x ", buf[ret]);
		if((ret % 0x0F) == 0)
			printf("\n");
	}
	printf("\n");

	return 0;
}

int write_sm_eeprom_data(u32 eeprom_id, int offset, u8 *buf,
			  int size)
{
	int ret = 0;
	int i;
	void *p;
#ifdef CONFIG_SYS_EEPROM_BUS_NUM
#if !CONFIG_IS_ENABLED(DM_I2C)
	unsigned int bus;
#endif
#endif

#if !CONFIG_IS_ENABLED(DM_I2C)
#ifdef CONFIG_SYS_EEPROM_BUS_NUM
	bus = i2c_get_bus_num();
	i2c_set_bus_num(CONFIG_SYS_EEPROM_BUS_NUM);
#endif
#endif

	/*
	 * The AT24C02 datasheet says that data can only be written in page
	 * mode, which means 8 bytes at a time, and it takes up to 5ms to
	 * complete a given write.
	 */
	for (i = 0, p = buf; i < size; i += 8, p += 8) {
#if !CONFIG_IS_ENABLED(DM_I2C)
		ret = i2c_write(CONFIG_SYS_I2C_EEPROM_ADDR, i,
				CONFIG_SYS_I2C_EEPROM_ADDR_LEN,
				p, min((int)(size - i), 8));
#else
		struct udevice *dev;
#ifdef CONFIG_SYS_EEPROM_BUS_NUM
		ret = i2c_get_chip_for_busnum(CONFIG_SYS_EEPROM_BUS_NUM,
					      CONFIG_SYS_I2C_EEPROM_ADDR,
					      CONFIG_SYS_I2C_EEPROM_ADDR_LEN,
					      &dev);
#else
		ret = i2c_get_chip_for_busnum(0, CONFIG_SYS_I2C_EEPROM_ADDR,
					      CONFIG_SYS_I2C_EEPROM_ADDR_LEN,
					      &dev);
#endif
		if (!ret)
			ret = dm_i2c_write(dev, i, p, min((int)(size - i),
							  8));
#endif
		if (ret)
			break;
		udelay(5000);	/* 5ms write cycle timing */
	}
	
	printf("write offset : 0x%02x  size : %d \n", offset, size);
	for(ret = 0 ; ret < size ; ret++)
	{
		printf("0x%02x ", buf[ret]);
		if((ret % 0x0F) == 0)
			printf("\n");
	}
	printf("\n");

	return 0;
}
