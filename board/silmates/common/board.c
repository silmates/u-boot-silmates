// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2024 Silmates Private Limited
 */


#include <common.h>
#include <env.h>
#include <log.h>
#include <asm/global_data.h>
#include <asm/sections.h>
#include <dm/uclass.h>
#include <i2c.h>
#include <linux/sizes.h>
#include <malloc.h>
#include "board.h"
#include <dm.h>
#include <i2c_eeprom.h>
#include <net.h>
#include <generated/dt.h>
#include <soc.h>
#include <linux/ctype.h>

#include "fru.h"

#if defined(CONFIG_SM_I2C_MAC_OFFSET)
int silmates_board_read_rom_ethaddr(char * name,unsigned char *ethaddr)
{
    int ret = -EINVAL;
    struct udevice *dev;
    ofnode eeprom;

	eeprom = ofnode_get_aliases_node(name);
	if (!ofnode_valid(eeprom)){
        printf("%s: %s not found\n", __func__,name);
		return -ENODEV;
	}

    ret = uclass_get_device_by_ofnode(UCLASS_I2C_EEPROM, eeprom, &dev);
    if (ret) {
        printf("%s: Failed to get EEPROM device (err=%d)\n", __func__, ret);
        return ret;
    }

    ret = dm_i2c_read(dev, CONFIG_SM_I2C_MAC_OFFSET, ethaddr, 8);
    if (ret) {
        printf("%s: I2C EEPROM MAC read failed (err=%d)\n", __func__, ret);
        return ret;
    }

    debug("EEPROM MAC %pM (offset 0x%X)\n",
            ethaddr, CONFIG_SM_I2C_MAC_OFFSET);

    return 0;
}
#endif

#define EEPROM_HEADER_MAGIC			0x534C4D54
#define EEPROM_HDR_MANUFACTURER_LEN	16
#define EEPROM_HDR_NAME_LEN			16
#define EEPROM_HDR_REV_LEN			8
#define EEPROM_HDR_SERIAL_LEN		20
#define EEPROM_HDR_NO_OF_MAC_ADDR	4
#define EEPROM_HDR_ETH_ALEN			ETH_ALEN

struct silmates_board_description {
	u32 header;
	char manufacturer[EEPROM_HDR_MANUFACTURER_LEN + 1];
	char name[EEPROM_HDR_NAME_LEN + 1];
	char revision[EEPROM_HDR_REV_LEN + 1];
	char serial[EEPROM_HDR_SERIAL_LEN + 1];
	u8 mac_addr[EEPROM_HDR_NO_OF_MAC_ADDR][EEPROM_HDR_ETH_ALEN + 1];
};

static int highest_id = -1;
static struct silmates_board_description *board_info;

#define SILMATES_I2C_DETECTION_BITS	sizeof(struct fru_common_hdr)

static int silmates_read_eeprom_fru(struct udevice *dev, char *name,
				  struct silmates_board_description *desc)
{
	int i, ret, eeprom_size;
	u8 *fru_content;

	eeprom_size = sizeof(fru_data);

	fru_content = calloc(1, eeprom_size);
	if (!fru_content)
		return -ENOMEM;

	debug("%s: I2C EEPROM read pass data at %p\n", __func__,
	      fru_content);

	ret = dm_i2c_read(dev, 0, (uchar *)fru_content,
			  eeprom_size);
	if (ret) {
		debug("%s: I2C EEPROM read failed\n", __func__);
		goto end;
	}

	fru_capture((unsigned long)fru_content);
	if (_DEBUG && CONFIG_IS_ENABLED(DTB_RESELECT)) {
		debug("Silmates I2C FRU format at %s:\n", name);
		ret = fru_display(0);
		if (ret) {
			printf("FRU format decoding failed.\n");
			goto end;
		}
	}

	if (desc->header == EEPROM_HEADER_MAGIC) {
		debug("Information already filled\n");
		ret = -EINVAL;
		goto end;
	}

	/* It is clear that FRU was captured and structures were filled */
	strncpy(desc->manufacturer, (char *)fru_data.brd.manufacturer_name,
		sizeof(desc->manufacturer));
	strncpy(desc->name, (char *)fru_data.brd.product_name,
		sizeof(desc->name));
	strim(desc->name);
	strncpy(desc->revision, (char *)fru_data.brd.rev,
		sizeof(desc->revision));
	strncpy(desc->serial, (char *)fru_data.brd.serial_number,
		sizeof(desc->serial));
	desc->header = EEPROM_HEADER_MAGIC;

end:
	free(fru_content);
	return ret;
}

static bool silmates_detect_fru(u8 *buffer)
{
	u8 checksum = 0;
	int i;

	checksum = fru_checksum((u8 *)buffer, sizeof(struct fru_common_hdr));
	if (checksum) {
		debug("%s Common header CRC FAIL\n", __func__);
		return false;
	}

	bool all_zeros = true;
	/* Checksum over all zeros is also zero that's why detect this case */
	for (i = 0; i < sizeof(struct fru_common_hdr); i++) {
		if (buffer[i] != 0)
			all_zeros = false;
	}

	if (all_zeros)
		return false;

	debug("%s Common header CRC PASS\n", __func__);
	return true;
}

static int silmates_read_eeprom_single(char *name,
				     struct silmates_board_description *desc)
{
	int ret;
	struct udevice *dev;
	ofnode eeprom;
	u8 buffer[SILMATES_I2C_DETECTION_BITS];

	eeprom = ofnode_get_aliases_node(name);
	if (!ofnode_valid(eeprom))
		return -ENODEV;

	ret = uclass_get_device_by_ofnode(UCLASS_I2C_EEPROM, eeprom, &dev);
	if (ret)
		return ret;

	ret = dm_i2c_read(dev, 0, buffer, sizeof(buffer));
	if (ret) {
		debug("%s: I2C EEPROM read failed\n", __func__);
		return ret;
	}

	debug("%s: i2c memory detected: %s\n", __func__, name);

	if (CONFIG_IS_ENABLED(CMD_FRU) && silmates_detect_fru(buffer))
		return silmates_read_eeprom_fru(dev, name, desc);

	return -ENODEV;
}

__maybe_unused int silmates_read_eeprom(void)
{
	int id;
	char name_buf[30]; /* 8 bytes should be enough for nvmem+number */
	struct silmates_board_description *desc;

	highest_id = dev_read_alias_highest_id("silmates,eeprom");
	/* No silmates,eeprom aliases present */
	if (highest_id < 0)
		return -EINVAL;

	board_info = calloc(1, sizeof(*desc) * (highest_id + 1));
	if (!board_info)
		return -ENOMEM;

	for (id = 0; id <= highest_id; id++) {
		// snprintf(name_buf, sizeof(name_buf), "nvmem%d", id);
		snprintf(name_buf, sizeof(name_buf), "silmates,eeprom%d",id);

		/* Alloc structure */
		desc = &board_info[id];

		/* Ignoring return value for supporting multiple chips */
		silmates_read_eeprom_single(name_buf, desc);
	}

	snprintf(name_buf, sizeof(name_buf), "silmates,eeprom0");
	desc = &board_info[0];
	silmates_board_read_rom_ethaddr(name_buf,desc->mac_addr[0]);

	/*
	 * Consider to clean board_info structure when board/cards are not
	 * detected.
	 */

	return 0;
}

#if defined(CONFIG_OF_BOARD)
void *board_fdt_blob_setup(int *err)
{
	void *fdt_blob;

	*err = 0;
	if (!IS_ENABLED(CONFIG_SPL_BUILD) {
		fdt_blob = (void *)CONFIG_SILMATES_OF_BOARD_DTB_ADDR;

		if (fdt_magic(fdt_blob) == FDT_MAGIC)
			return fdt_blob;

		debug("DTB is not passed via %p\n", fdt_blob);
	}

	if (IS_ENABLED(CONFIG_SPL_BUILD)) {
		/*
		 * FDT is at end of BSS unless it is in a different memory
		 * region
		 */
		if (IS_ENABLED(CONFIG_SPL_SEPARATE_BSS))
			fdt_blob = (ulong *)&_image_binary_end;
		else
			fdt_blob = (ulong *)&__bss_end;
	} else {
		/* FDT is at end of image */
		fdt_blob = (ulong *)&_end;
	}

	if (fdt_magic(fdt_blob) == FDT_MAGIC)
		return fdt_blob;

	debug("DTB is also not passed via %p\n", fdt_blob);

	*err = -EINVAL;
	return NULL;
}
#endif

int board_late_init_silmates(void)
{
        int id;
        char mac_str[18];
        struct silmates_board_description *desc;
        phys_size_t bootm_size = gd->ram_top - gd->ram_base;

        env_set_addr("bootm_low", (void *)gd->ram_base);
        env_set_addr("bootm_size", (void *)bootm_size);

        id = 0;
        desc = &board_info[id];

        if (desc && desc->header == EEPROM_HEADER_MAGIC) {
                if (desc->manufacturer[0])
                        env_set("manufacturer", desc->manufacturer);
                if (desc->name[0])
                        env_set("board_name", desc->name);
                if (desc->revision[0])
                        env_set("board_rev", desc->revision);
                if (desc->serial[0])
                        env_set("serial#", desc->serial);
        }

		u8 *mac = (u8 *)desc->mac_addr[0];
        u8 ethaddr[6];

        ethaddr[0] = mac[0];
        ethaddr[1] = mac[1];
        ethaddr[2] = mac[2];
        ethaddr[3] = mac[5];
        ethaddr[4] = mac[6];
        ethaddr[5] = mac[7];

        snprintf(mac_str, sizeof(mac_str),
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 ethaddr[0], ethaddr[1], ethaddr[2],
                 ethaddr[3], ethaddr[4], ethaddr[5]);

        env_set("ethaddr", mac_str);
        // printf("ethaddr set: %s\n", mac_str);

        return 0;
}

static char *board_name = DEVICE_TREE;

int __maybe_unused board_fit_config_name_match(const char *name)
{
	debug("%s: Check %s, default %s\n", __func__, name, board_name);

	// if (!strcmp(name, board_name))
		return 0;

	return -1;
}


#if CONFIG_IS_ENABLED(DTB_RESELECT)
#define MAX_NAME_LENGTH	50

char * __maybe_unused __weak board_name_decode(void)
{
	char *board_local_name;
	struct silmates_board_description *desc;
	int i, id;

	board_local_name = calloc(1, MAX_NAME_LENGTH);
	if (!board_info)
		return NULL;

	for (id = 0; id <= highest_id; id++) {
		desc = &board_info[id];

		/* No board description */
		if (!desc)
			goto error;

		/* Board is not detected */
		if (desc->header != EEPROM_HEADER_MAGIC)
			continue;

		/* The first string should be soc name */
		if (!id)
			strcat(board_local_name, CONFIG_SYS_BOARD);

		/*
		 * For two purpose here:
		 * soc_name- eg: zynqmp-
		 * and between base board and CC eg: ..revA-sck...
		 */
		strcat(board_local_name, "-");

		if (desc->name[0]) {
			/* For DT composition name needs to be lowercase */
			for (i = 0; i < sizeof(desc->name); i++)
				desc->name[i] = tolower(desc->name[i]);

			strcat(board_local_name, desc->name);
		}
		if (desc->revision[0]) {
			strcat(board_local_name, "-rev");

			/* And revision needs to be uppercase */
			for (i = 0; i < sizeof(desc->revision); i++)
				desc->revision[i] = toupper(desc->revision[i]);

			strcat(board_local_name, desc->revision);
		}
	}

	/*
	 * Longer strings will end up with buffer overflow and potential
	 * attacks that's why check it
	 */
	if (strlen(board_local_name) >= MAX_NAME_LENGTH)
		panic("Board name can't be determined\n");

	if (strlen(board_local_name))
		return board_local_name;

error:
	free(board_local_name);
	return NULL;
}

bool __maybe_unused __weak board_detection(void)
{
	if (CONFIG_IS_ENABLED(DM_I2C) && CONFIG_IS_ENABLED(I2C_EEPROM)) {
		int ret;

		ret = silmates_read_eeprom();
		return !ret ? true : false;
	}

	return false;
}

int embedded_dtb_select(void)
{
	if (board_detection()) {
		char *board_local_name;

		board_local_name = board_name_decode();
		if (board_local_name) {
			board_name = board_local_name;
			/* Time to change DTB on fly */
			/* Both ways should work here */
			/* fdtdec_resetup(&rescan); */
			fdtdec_setup();
		}
	}
	return 0;
}
#endif
