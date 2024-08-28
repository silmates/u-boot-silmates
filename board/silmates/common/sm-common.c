// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2016 Silmates, Inc.
 */

#include <common.h>
#include <env.h>
#include <g_dnl.h>
#include <init.h>
#include <linux/libfdt.h>

#ifdef CONFIG_DM_VIDEO
#include <bmp_logo.h>
#include <dm.h>
#include <splash.h>
#include <video.h>
#endif

#include "sm-config.h"
#include <asm/setup.h>
#include "sm-common.h"


#ifdef CONFIG_SM_CFG_BLOCK
static char sm_serial_str[9];
static char sm_board_rev_str[6];

#ifdef CONFIG_SM_CFG_BLOCK_EXTRA
//static char sm_car_serial_str[9];
//static char sm_car_rev_str[6];
//static char *sm_carrier_board_name;
#endif

#if defined(CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG)
u32 get_board_revision(void)
{
	printf(" Test : %s \n",__func__);
	/* Check validity */
	if (!sm_hw_tag.ver_major)
		return 0;

	return ((sm_hw_tag.ver_major & 0xff) << 8) |
		((sm_hw_tag.ver_minor & 0xf) << 4) |
		((sm_hw_tag.ver_assembly & 0xf) + 0xa);
}
#endif /* CONFIG_SM_CFG_BLOCK */

#ifndef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	printf(" Test : %s \n",__func__);
	int array[8];
	unsigned int serial = sm_hw_tag.sm_serial;
	int i;

	serialnr->low = 0;
	serialnr->high = 0;

	/* Check validity */
	if (serial) {
		/*
		 * Convert to Linux serial number format (hexadecimal coded
		 * decimal)
		 */
		i = 7;
		while (serial) {
			array[i--] = serial % 10;
			serial /= 10;
		}
		while (i >= 0)
			array[i--] = 0;
		serial = array[0];
		for (i = 1; i < 8; i++) {
			serial *= 16;
			serial += array[i];
		}

		serialnr->low = serial;
	}
}
#endif /* CONFIG_SERIAL_TAG */

int show_board_info(void)
{
	printf(" Test : %s \n",__func__);
	unsigned char ethaddr[6];

	mac_read_from_eeprom();

	if (read_sm_cfg_block()) {
		printf("MISSING SILMATES CONFIG BLOCK\n");
//		sm_eth_addr.oui = htonl(SILMATES_OUI << 8);
//		sm_eth_addr.nic = htonl(sm_serial << 8);
		checkboard();
	} else {
		sprintf(sm_serial_str, "%08u", sm_hw_tag.sm_serial);
		sprintf(sm_board_rev_str, "V%1d.%1d%c",
			sm_hw_tag.ver_major,
			sm_hw_tag.ver_minor,
			(char)sm_hw_tag.ver_assembly + 'A');

		env_set("serial#", sm_serial_str);

		printf("Model: Silmates %s %s %s %s %s %s \n",
		       silmates_modules[sm_hw_tag.prodid.processor],
		       silmates_wifi_modules[sm_hw_tag.prodid.wifi],
		       silmates_storage_modules[sm_hw_tag.prodid.storage],
		       silmates_ram_size_modules[sm_hw_tag.prodid.ram_size],
		       silmates_storage_size_modules[sm_hw_tag.prodid.storage_size],
		       sm_board_rev_str);
		printf("Serial# %s\n",
		       sm_serial_str);
#ifdef CONFIG_SM_CFG_BLOCK_EXTRA

#endif
	}

	/*
	 * Check if environment contains a valid MAC address,
	 * set the one from config block if not
	 */
//	if (!eth_env_get_enetaddr("ethaddr", ethaddr))
//		eth_env_set_enetaddr("ethaddr", (u8 *)&sm_eth_addr);

#ifdef CONFIG_SM_CFG_BLOCK_2ND_ETHADDR
	if (!eth_env_get_enetaddr("eth1addr", ethaddr)) {
		/*
		 * Secondary MAC address is allocated from block
		 * 0x100000 higher then the first MAC address
		 */
//		memcpy(ethaddr, &sm_eth_addr, 6);
		ethaddr[3] += 0x10;
		eth_env_set_enetaddr("eth1addr", ethaddr);
	}
#endif

	return 0;
}

#ifdef CONFIG_SM_CFG_BLOCK_USB_GADGET_PID
int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	printf(" Test : %s \n",__func__);
	unsigned short usb_pid;

	usb_pid = SILMATES_USB_PRODUCT_NUM_OFFSET + sm_hw_tag.prodid.processor;
	
	put_unaligned(usb_pid, &dev->idProduct);

	return 0;
}
#endif

#if defined(CONFIG_OF_LIBFDT)
int ft_common_board_setup(void *blob, struct bd_info *bd)
{
	printf(" Test : %s \n",__func__);
	if (sm_hw_tag.sm_serial) {
		fdt_setprop(blob, 0, "serial-number", sm_serial_str,
			    strlen(sm_serial_str) + 1);
	}

	if (sm_hw_tag.ver_major) {
		char prod_id[5];

		sprintf(prod_id, "%04u", sm_hw_tag.prodid.processor);
		fdt_setprop(blob, 0, "silmates,product-id", prod_id, 5);

		sprintf(prod_id, "%04u", sm_hw_tag.prodid.wifi);
		fdt_setprop(blob, 0, "silmates,wifi-id", prod_id, 5);

		sprintf(prod_id, "%04u", sm_hw_tag.prodid.storage);
		fdt_setprop(blob, 0, "silmates,storage-id", prod_id, 5);

		sprintf(prod_id, "%04u", sm_hw_tag.prodid.ram_size);
		fdt_setprop(blob, 0, "silmates,ram_size-id", prod_id, 5);

		sprintf(prod_id, "%04u", sm_hw_tag.prodid.storage_size);
		fdt_setprop(blob, 0, "silmates,storage_size-id", prod_id, 5);

		fdt_setprop(blob, 0, "silmates,board-rev", sm_board_rev_str,
			    strlen(sm_board_rev_str) + 1);
	}

	return 0;
}
#endif

#else /* CONFIG_SM_CFG_BLOCK */

#if defined(CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG)
u32 get_board_revision(void)
{
	printf(" Test : %s \n",__func__);
	return 0;
}
#endif /* CONFIG_REVISION_TAG */

#ifdef CONFIG_SERIAL_TAG
u32 get_board_serial(void)
{
	printf(" Test : %s \n",__func__);
	return 0;
}
#endif /* CONFIG_SERIAL_TAG */

int ft_common_board_setup(void *blob, struct bd_info *bd)
{
	printf(" Test : %s \n",__func__);
	return 0;
}

#endif /* CONFIG_SM_CFG_BLOCK */
