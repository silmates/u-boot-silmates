// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2016-2020 Silmates
 */

#include <common.h>
#include <asm/global_data.h>
#include "sm-config.h"
#include "sm-eeprom.h"
#include <command.h>
#include <asm/cache.h>
#include <asm/arch/sys_proto.h>
#include <cli.h>
#include <console.h>
#include <env.h>
#include <flash.h>
#include <malloc.h>
#include <mmc.h>
#include <nand.h>
#include <asm/mach-types.h>
#include "sm-common.h"

DECLARE_GLOBAL_DATA_PTR;

#define TAG_VALID	0x534D	// SM -> 0x53 0x4D
#define TAG_MAC		0x0000
#define TAG_CAR_SERIAL	0x0021
#define TAG_HW		0x0008
#define TAG_INVALID	0xffff

#define TAG_FLAG_VALID	0x1

#define SM_EEPROM_ID_MODULE		0
#define SM_EEPROM_ID_CARRIER		1

#if defined(CONFIG_SM_CFG_BLOCK_IS_IN_MMC)
#define SM_CFG_BLOCK_MAX_SIZE 512
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_NAND)
#define SM_CFG_BLOCK_MAX_SIZE 64
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_NOR)
#define SM_CFG_BLOCK_MAX_SIZE 64
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_EEPROM)
#define SM_CFG_BLOCK_MAX_SIZE 64
#else
#error Silmates config block location not set
#endif

#ifdef CONFIG_SM_CFG_BLOCK_EXTRA
#define SM_CFG_BLOCK_EXTRA_MAX_SIZE 64
#endif

struct silmates_tag {
	u32 len:14;
	u32 flags:2;
	u32 id:16;
};

bool valid_smcfg;
struct silmates_hw sm_hw_tag;

const char * processor_title = "Select board type? ";
const char * wifi_title = "Is WIFI/BT available on Board? ";
const char * storage_title = "Select storage type available on Board? ";
const char * ram_size_title = "Select RAM size available on Board? ";
const char * storage_size_title = "Select Storage size available on Board? ";
const char * hw_major_title = "Please enter HW version major? ";
const char * hw_minor_title = "Please enter HW version minor? ";
const char * serial_number_title = "Please enter HW serial number? ";

const char * silmates_modules[] = {
	[0] = "Invalid Module",
	[1] = "iMX8M Mini-MEZ",
	[2] = "iMX8M Nano-MEZ",
};

const char * silmates_wifi_modules[] = {
	[0] = "Invalid WIFI/BT",
	[1] = "Without WIFI/BT",
	[2] = "With WIFI/BT",
};

const char * silmates_storage_modules[] = {
	[0] = "Invalid Storage type",
	[1] = "eMMC",
	[2] = "NAND",
	[3] = "uSD",
	[4] = "NOR",
};

const char * silmates_ram_size_modules[] = {
	[0] = "Invalid RAM size",
	[1] = "128 MB",
	[2] = "256 MB",
	[3] = "512 MB",
	[4] = "1 GB",
	[5] = "2 GB",
	[6] = "4 GB",
	[7] = "8 GB",
};

const char * silmates_storage_size_modules[] = {
	[0] = "Invalid Storage size",
	[1] = "128 MB",
	[2] = "256 MB",
	[3] = "512 MB",
	[4] = "1 GB",
	[5] = "2 GB",
	[6] = "4 GB",
	[7] = "8 GB",
	[8] = "16 GB",
	[9] = "32 GB",
	[10] = "64 GB",
};


static int smcfg_print()
{
	printf( "Module       - [ %2d ]: %s \n",sm_hw_tag.prodid.processor,
			silmates_modules[sm_hw_tag.prodid.processor]);
	printf( "WIFI         - [ %2d ]: %s \n",sm_hw_tag.prodid.wifi,
			silmates_wifi_modules[sm_hw_tag.prodid.wifi]);
	printf( "Storage type - [ %2d ]: %s \n",sm_hw_tag.prodid.storage,
			silmates_storage_modules[sm_hw_tag.prodid.storage]);
	printf( "RAM size     - [ %2d ]: %s \n",sm_hw_tag.prodid.ram_size,
			silmates_ram_size_modules[sm_hw_tag.prodid.ram_size]);
	printf( "Sotage size  - [ %2d ]: %s \n",sm_hw_tag.prodid.storage_size,
			silmates_storage_size_modules[sm_hw_tag.prodid.storage_size]);
	printf( "HW version   - %d.%d \n",sm_hw_tag.ver_major,
			sm_hw_tag.ver_minor);
	printf( "Serialnumber - %d \n",sm_hw_tag.sm_serial);
	printf( "MAC id       - %02x:%02x:%02x:%02x:%02x:%02x \n",
			sm_hw_tag.eth_addr[0],sm_hw_tag.eth_addr[1],
			sm_hw_tag.eth_addr[2],sm_hw_tag.eth_addr[3],
			sm_hw_tag.eth_addr[4],sm_hw_tag.eth_addr[5]);
}

#ifdef CONFIG_SM_CFG_BLOCK_IS_IN_MMC
static int sm_cfg_block_mmc_storage(u8 *config_block, int write)
{
	printf(" Test : %s \n",__func__);
	struct mmc *mmc;
	int dev = CONFIG_SM_CFG_BLOCK_DEV;
	int offset = CONFIG_SM_CFG_BLOCK_OFFSET;
	uint part = CONFIG_SM_CFG_BLOCK_PART;
	uint blk_start;
	int ret = 0;

	/* Read production parameter config block from eMMC */
	mmc = find_mmc_device(dev);
	if (!mmc) {
		puts("No MMC card found\n");
		ret = -ENODEV;
		goto out;
	}
	if (mmc_init(mmc)) {
		puts("MMC init failed\n");
		return -EINVAL;
	}
	if (part != mmc_get_blk_desc(mmc)->hwpart) {
		if (blk_select_hwpart_devnum(IF_TYPE_MMC, dev, part)) {
			puts("MMC partition switch failed\n");
			ret = -ENODEV;
			goto out;
		}
	}
	if (offset < 0)
		offset += mmc->capacity;
	blk_start = ALIGN(offset, mmc->write_bl_len) / mmc->write_bl_len;

	if (!write) {
		/* Careful reads a whole block of 512 bytes into config_block */
		if (blk_dread(mmc_get_blk_desc(mmc), blk_start, 1,
			      (unsigned char *)config_block) != 1) {
			ret = -EIO;
			goto out;
		}
	} else {
		/* Just writing one 512 byte block */
		if (blk_dwrite(mmc_get_blk_desc(mmc), blk_start, 1,
			       (unsigned char *)config_block) != 1) {
			ret = -EIO;
			goto out;
		}
	}

out:
	/* Switch back to regular eMMC user partition */
	blk_select_hwpart_devnum(IF_TYPE_MMC, 0, 0);

	return ret;
}
#endif

#ifdef CONFIG_SM_CFG_BLOCK_IS_IN_NAND
static int read_sm_cfg_block_from_nand(unsigned char *config_block)
{
	printf(" Test : %s \n",__func__);
	size_t size = SM_CFG_BLOCK_MAX_SIZE;
	struct mtd_info *mtd = get_nand_dev_by_index(0);

	if (!mtd)
		return -ENODEV;

	/* Read production parameter config block from NAND page */
	return nand_read_skip_bad(mtd, CONFIG_SM_CFG_BLOCK_OFFSET,
				  &size, NULL, SM_CFG_BLOCK_MAX_SIZE,
				  config_block);
}

static int write_sm_cfg_block_to_nand(unsigned char *config_block)
{
	printf(" Test : %s \n",__func__);
	size_t size = SM_CFG_BLOCK_MAX_SIZE;

	/* Write production parameter config block to NAND page */
	return nand_write_skip_bad(get_nand_dev_by_index(0),
				   CONFIG_SM_CFG_BLOCK_OFFSET,
				   &size, NULL, SM_CFG_BLOCK_MAX_SIZE,
				   config_block, WITH_WR_VERIFY);
}
#endif

#ifdef CONFIG_SM_CFG_BLOCK_IS_IN_NOR
static int read_sm_cfg_block_from_nor(unsigned char *config_block)
{
	printf(" Test : %s \n",__func__);
	/* Read production parameter config block from NOR flash */
	memcpy(config_block, (void *)CONFIG_SM_CFG_BLOCK_OFFSET,
	       SM_CFG_BLOCK_MAX_SIZE);
	return 0;
}

static int write_sm_cfg_block_to_nor(unsigned char *config_block)
{
	printf(" Test : %s \n",__func__);
	/* Write production parameter config block to NOR flash */
	return flash_write((void *)config_block, CONFIG_SM_CFG_BLOCK_OFFSET,
			   SM_CFG_BLOCK_MAX_SIZE);
}
#endif

#ifdef CONFIG_SM_CFG_BLOCK_IS_IN_EEPROM
static int read_sm_cfg_block_from_eeprom(unsigned char *config_block)
{
	printf(" Test : %s \n",__func__);
	return read_sm_eeprom_data(SM_EEPROM_ID_MODULE, 0x0, config_block,
				    SM_CFG_BLOCK_MAX_SIZE);
}

static int write_sm_cfg_block_to_eeprom(unsigned char *config_block)
{
	printf(" Test : %s \n",__func__);
	return write_sm_eeprom_data(SM_EEPROM_ID_MODULE, 0x0, config_block,
				     SM_CFG_BLOCK_MAX_SIZE);
}
#endif

static int write_smcfg(u8 *config_block)
{
	printf(" Test : %s \n",__func__);
	int offset = 0;
	int ret = CMD_RET_SUCCESS;
	int err;

	memset(config_block, 0xFF, SM_CFG_BLOCK_MAX_SIZE);

	memcpy(config_block,&sm_hw_tag,sizeof(sm_hw_tag));

#if defined(CONFIG_SM_CFG_BLOCK_IS_IN_MMC)
	err = sm_cfg_block_mmc_storage(config_block, 1);
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_NAND)
	err = write_sm_cfg_block_to_nand(config_block);
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_NOR)
	err = write_sm_cfg_block_to_nor(config_block);
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_EEPROM)
	err = write_sm_cfg_block_to_eeprom(config_block);
#else
	err = -EINVAL;
#endif
	if (err) {
		printf("Failed to write Silmates config block: %d\n", ret);
		ret = CMD_RET_FAILURE;
		goto out;
	}

out:
	printf("Silmates config block successfully written\n");
	return ret;
}


/**
 *  update_crc - update the CRC
 *
 *  This function should be called after each update to the EEPROM structure,
 *  to make sure the CRC is always correct.
 */
static void update_crc(void)
{
	printf(" Test : %s \n",__func__);
	u32 crc;

	crc = crc32(0, (void *)&sm_hw_tag, sizeof(sm_hw_tag) - 4);
	sm_hw_tag.crc = cpu_to_be32(crc);
}

static int process_smcfg(u8 *config_block)
{
	printf(" Test : %s \n",__func__);
	unsigned int crc;

	crc = crc32(0, config_block, sizeof(sm_hw_tag) - 4);

	if (crc != be32_to_cpu(sm_hw_tag.crc)) {
		printf("CRC: %08x (should be %08x)\n",
			be32_to_cpu(sm_hw_tag.crc), crc);
		goto cfg_update;
	} else {
		printf("CRC: %08x\n", be32_to_cpu(sm_hw_tag.crc));
	}

	memcpy(&sm_hw_tag,config_block,sizeof(sm_hw_tag));
	if(sm_hw_tag.tag_id == TAG_VALID) {

		/* Cap product id to avoid issues with a yet unknown one */
		sm_hw_tag.prodid.processor = VALIDATE_PARAM_RANGE(sm_hw_tag.prodid.processor,0,SM_PROCESSOR_MAX);

		/* Cap wifi id to avoid issues with a yet unknown one */
		sm_hw_tag.prodid.wifi = VALIDATE_PARAM_RANGE(sm_hw_tag.prodid.wifi,0,SM_WIFI_MAX);

		/* Cap storage id to avoid issues with a yet unknown one */
		sm_hw_tag.prodid.storage = VALIDATE_PARAM_RANGE(sm_hw_tag.prodid.storage,0,SM_STORAGE_MAX);

		/* Cap ram size id to avoid issues with a yet unknown one */
		sm_hw_tag.prodid.ram_size = VALIDATE_PARAM_RANGE(sm_hw_tag.prodid.ram_size,0,SM_RAM_SIZE_MAX);

		/* Cap storage size id to avoid issues with a yet unknown one */
		sm_hw_tag.prodid.storage_size = VALIDATE_PARAM_RANGE(sm_hw_tag.prodid.storage_size,0,SM_STORAGE_SIZE_MAX);

		sm_hw_tag.ver_major = VALIDATE_PARAM_RANGE(sm_hw_tag.ver_major,0,U16_MAX);

		sm_hw_tag.ver_minor = VALIDATE_PARAM_RANGE(sm_hw_tag.ver_minor,0,U16_MAX);

		sm_hw_tag.ver_assembly = VALIDATE_PARAM_RANGE(sm_hw_tag.ver_assembly,0,U16_MAX);

		sm_hw_tag.sm_serial = VALIDATE_PARAM_RANGE(sm_hw_tag.sm_serial,0,U32_MAX);

//		smcfg_print();
	} else {
cfg_update:
		memset(&sm_hw_tag,0xFF,sizeof(sm_hw_tag));
		
		sm_hw_tag.tag_id = TAG_VALID;
		
		mac_read_from_eeprom();
		
		update_crc();
		
		write_smcfg(config_block);
	}
	return 0;
}

int read_sm_cfg_block(void)
{
	printf(" Test : %s \n",__func__);
	int ret = 0;
	u8 *config_block = NULL;
	struct silmates_tag *tag;
	size_t size = SM_CFG_BLOCK_MAX_SIZE;
	int offset;

	/* Allocate RAM area for config block */
	config_block = memalign(ARCH_DMA_MINALIGN, size);
	if (!config_block) {
		printf("Not enough malloc space available!\n");
		return -ENOMEM;
	}

	memset(config_block, 0, size);

#if defined(CONFIG_SM_CFG_BLOCK_IS_IN_MMC)
	ret = sm_cfg_block_mmc_storage(config_block, 0);
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_NAND)
	ret = read_sm_cfg_block_from_nand(config_block);
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_NOR)
	ret = read_sm_cfg_block_from_nor(config_block);
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_EEPROM)
	ret = read_sm_cfg_block_from_eeprom(config_block);
#else
	ret = -EINVAL;
#endif
	printf("stage 1 : %d \n",ret);
	if (ret)
		goto out;

	ret = process_smcfg(config_block);

//	ret = 1;
	printf("stage 2 : %d \n",ret);

out:
	free(config_block);
	return ret;
}

static int get_smcfg_barcode(char *barcode, struct silmates_hw *tag,
				u32 *serial)
{
	printf(" Test : %s \n",__func__);
	char revision[3] = {barcode[6], barcode[7], '\0'};

	if (strlen(barcode) < 16) {
		printf("Argument too short, barcode is 16 chars long\n");
		return -1;
	}

	/* Get hardware information from the first 8 digits */
	tag->ver_major = barcode[4] - '0';
	tag->ver_minor = barcode[5] - '0';
	tag->ver_assembly = dectoul(revision, NULL);

	barcode[4] = '\0';
//	tag->prodid = dectoul(barcode, NULL);

	/* Parse second part of the barcode (serial number */
	barcode += 8;
	*serial = dectoul(barcode, NULL);

	return 0;
}

static int write_tag(u8 *config_block, int *offset, int tag_id,
		     u8 *tag_data, size_t tag_data_size)
{
	printf(" Test : %s \n",__func__);
	struct silmates_tag *tag;

	if (!offset || !config_block)
		return -EINVAL;

	tag = (struct silmates_tag *)(config_block + *offset);
	tag->id = tag_id;
	tag->flags = TAG_FLAG_VALID;
	/* len is provided as number of 32bit values after the tag */
	tag->len = (tag_data_size + sizeof(u32) - 1) / sizeof(u32);
	*offset += sizeof(struct silmates_tag);
	if (tag_data && tag_data_size) {
		memcpy(config_block + *offset, tag_data,
		       tag_data_size);
		*offset += tag_data_size;
	}

	return 0;
}

static int do_smcfg_create(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	u8 *config_block;
	size_t size = SM_CFG_BLOCK_MAX_SIZE;
	int offset = 0;
	int ret = CMD_RET_SUCCESS;
	int err;
	int force_overwrite = 0;

	if (argc >= 3) {
		if (argv[2][0] == '-' && argv[2][1] == 'y')
			force_overwrite = 1;
	}

	/* Allocate RAM area for config block */
	config_block = memalign(ARCH_DMA_MINALIGN, size);
	if (!config_block) {
		printf("Not enough malloc space available!\n");
		return CMD_RET_FAILURE;
	}

	memset(config_block, 0xff, size);

	read_sm_cfg_block();
	if (valid_smcfg) {
#if defined(CONFIG_SM_CFG_BLOCK_IS_IN_NAND)
		/*
		 * On NAND devices, recreation is only allowed if the page is
		 * empty (config block invalid...)
		 */
		printf("NAND erase block %d need to be erased before creating a Silmates config block\n",
		       CONFIG_SM_CFG_BLOCK_OFFSET /
		       get_nand_dev_by_index(0)->erasesize);
		goto out;
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_NOR)
		/*
		 * On NOR devices, recreation is only allowed if the sector is
		 * empty and write protection is off (config block invalid...)
		 */
		printf("NOR sector at offset 0x%02x need to be erased and unprotected before creating a Silmates config block\n",
		       CONFIG_SM_CFG_BLOCK_OFFSET);
		goto out;
#else
		if (!force_overwrite) {
			char message[CONFIG_SYS_CBSIZE];

			sprintf(message,
				"A valid Silmates config block is present, still recreate? [y/N] ");
/*
			if (!cli_readline(message))
				goto out;

			if (console_buffer[0] != 'y' &&
			    console_buffer[0] != 'Y')
				goto out;
*/
		}
#endif
	}

	memset(config_block + offset, 0, 32 - offset);
#if defined(CONFIG_SM_CFG_BLOCK_IS_IN_MMC)
	err = sm_cfg_block_mmc_storage(config_block, 1);
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_NAND)
	err = write_sm_cfg_block_to_nand(config_block);
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_NOR)
	err = write_sm_cfg_block_to_nor(config_block);
#elif defined(CONFIG_SM_CFG_BLOCK_IS_IN_EEPROM)
	err = write_sm_cfg_block_to_eeprom(config_block);
#else
	err = -EINVAL;
#endif
	if (err) {
		printf("Failed to write Silmates config block: %d\n", ret);
		ret = CMD_RET_FAILURE;
		goto out;
	}

	printf("Silmates config block successfully written\n");

out:
	free(config_block);
	return ret;
}

static int do_smcfg_a(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	int ret = CMD_RET_SUCCESS;
	int i=0;
	int list_len=0;
	int selected_index=0;
	char *endptr;

	if (argc == 3) {
		selected_index = simple_strtoul(argv[2], &endptr, 10);
		if (*endptr != '\0') {
			printf( "Invalid input parsing for a %d %s \n",selected_index,argv[2]);
			return ret;
		}

		if((selected_index <= SM_PROCESSOR_NONE) | (selected_index >= SM_PROCESSOR_MAX)) {
			printf( "Invalid values %d valid value ranges from %d to %d \n",
					selected_index,SM_PROCESSOR_NONE + 1,SM_PROCESSOR_MAX - 1);
			selected_index = 0;
			goto print_usage;
		}

		sm_hw_tag.prodid.processor = selected_index;
		printf( "Selected value - [ %2d ]: %s \n",selected_index,silmates_modules[selected_index]);
		return ret;
	}

print_usage:
	printf( "=> %s \n",processor_title);
	list_len = SM_PROCESSOR_MAX;
	for(i = 1 ; i < list_len ; i++)
	{
		printf( "    [ %2d ] : %s \n",i,silmates_modules[i]);
	}

	return ret;
}

static int do_smcfg_b(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	int ret = CMD_RET_SUCCESS;
	int i=0;
	int list_len=0;
	int selected_index=0;
	char *endptr;

	if (argc == 3) {
		selected_index = simple_strtoul(argv[2], &endptr, 10);
		if (*endptr != '\0') {
			printf( "Invalid input parsing for a %d %s \n",selected_index,argv[2]);
			return ret;
		}

		if((selected_index <= SM_WIFI_NONE) | (selected_index >= SM_WIFI_MAX)) {
			printf( "Invalid values %d valid value ranges from %d to %d \n",
					selected_index,SM_WIFI_NONE + 1,SM_WIFI_MAX - 1);
			selected_index = 0;
			goto print_usage;
		}

		sm_hw_tag.prodid.wifi = selected_index;
		printf( "Selected value - [ %2d ]: %s \n",selected_index,silmates_wifi_modules[selected_index]);
		return ret;
	}

print_usage:
	printf( "=> %s \n",wifi_title);
	list_len = SM_WIFI_MAX;
	for(i = 1 ; i < list_len ; i++)
	{
		printf( "    [ %2d ] : %s \n",i,silmates_wifi_modules[i]);
	}

	return ret;
}

static int do_smcfg_c(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	int ret = CMD_RET_SUCCESS;
	int i=0;
	int list_len=0;
	int selected_index=0;
	char *endptr;

	if (argc == 3) {
		selected_index = simple_strtoul(argv[2], &endptr, 10);
		if (*endptr != '\0') {
			printf( "Invalid input parsing for a %d %s \n",selected_index,argv[2]);
			return ret;
		}

		if((selected_index <= SM_STORAGE_NONE) | (selected_index >= SM_STORAGE_MAX)) {
			printf( "Invalid values %d valid value ranges from %d to %d \n",
					selected_index,SM_STORAGE_NONE + 1,SM_STORAGE_MAX - 1);
			selected_index = 0;
			goto print_usage;
		}

		sm_hw_tag.prodid.storage = selected_index;
		printf( "Selected value - [ %2d ]: %s \n",selected_index,silmates_storage_modules[selected_index]);
		return ret;
	}

print_usage:
	printf( "=> %s \n",storage_title);
	list_len = SM_STORAGE_MAX;
	for(i = 1 ; i < list_len ; i++)
	{
		printf( "    [ %2d ] : %s \n",i,silmates_storage_modules[i]);
	}

	return ret;
}

static int do_smcfg_d(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	int ret = CMD_RET_SUCCESS;
	int i=0;
	int list_len=0;
	int selected_index=0;
	char *endptr;

	if (argc == 3) {
		selected_index = simple_strtoul(argv[2], &endptr, 10);
		if (*endptr != '\0') {
			printf( "Invalid input parsing for a %d %s \n",selected_index,argv[2]);
			return ret;
		}

		if((selected_index <= SM_RAM_SIZE_NONE) | (selected_index >= SM_RAM_SIZE_MAX)) {
			printf( "Invalid values %d valid value ranges from %d to %d \n",
					selected_index,SM_RAM_SIZE_NONE + 1,SM_RAM_SIZE_MAX - 1);
			selected_index = 0;
			goto print_usage;
		}

		sm_hw_tag.prodid.ram_size = selected_index;
		printf( "Selected value - [ %2d ]: %s \n",selected_index,silmates_ram_size_modules[selected_index]);
		return ret;
	}

print_usage:
	printf( "=> %s \n",ram_size_title);
	list_len = SM_RAM_SIZE_MAX;
	for(i = 1 ; i < list_len ; i++)
	{
		printf( "    [ %2d ] : %s \n",i,silmates_ram_size_modules[i]);
	}

	return ret;
}

static int do_smcfg_e(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	int ret = CMD_RET_SUCCESS;
	int i=0;
	int list_len=0;
	int selected_index=0;
	char *endptr;

	if (argc == 3) {
		selected_index = simple_strtoul(argv[2], &endptr, 10);
		if (*endptr != '\0') {
			printf( "Invalid input parsing for a %d %s \n",selected_index,argv[2]);
			return ret;
		}

		if((selected_index <= SM_STORAGE_SIZE_NONE) | (selected_index >= SM_STORAGE_SIZE_MAX)) {
			printf( "Invalid values %d valid value ranges from %d to %d \n",
					selected_index,SM_STORAGE_SIZE_NONE + 1,SM_STORAGE_SIZE_MAX - 1);
			selected_index = 0;
			goto print_usage;
		}

		sm_hw_tag.prodid.storage_size = selected_index;
		printf( "Selected value - [ %2d ]: %s \n",selected_index,silmates_storage_size_modules[selected_index]);
		return ret;
	}

print_usage:
	printf( "=> %s \n",storage_size_title);
	list_len = SM_STORAGE_SIZE_MAX;
	for(i = 1 ; i < list_len ; i++)
	{
		printf( "    [ %2d ] : %s \n",i,silmates_storage_size_modules[i]);
	}

	return ret;
}


static int do_smcfg_f(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	int ret = CMD_RET_SUCCESS;
	u32 selected_index=0;
	char *endptr;

	if (argc == 3) {
		selected_index = simple_strtoul(argv[2], &endptr, 10);
		if (*endptr != '\0') {
			printf( "Invalid input parsing for a %d %s \n",selected_index,argv[2]);
			return ret;
		}

		sm_hw_tag.ver_major = selected_index;
		printf( "Entered value - %d \n",selected_index);
		return ret;
	}

	printf( "=> %s \n",hw_major_title);
	printf( " Major - %d \n",sm_hw_tag.ver_major);

	return ret;
}

static int do_smcfg_g(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	int ret = CMD_RET_SUCCESS;
	u32 selected_index=0;
	char *endptr;

	if (argc == 3) {
		selected_index = simple_strtoul(argv[2], &endptr, 10);
		if (*endptr != '\0') {
			printf( "Invalid input parsing for a %d %s \n",selected_index,argv[2]);
			return ret;
		}

		sm_hw_tag.ver_minor = selected_index;
		printf( "Entered value - %d \n",selected_index);
		return ret;
	}

	printf( "=> %s \n",hw_minor_title);
	printf( " Minor - %d \n",sm_hw_tag.ver_minor);

	return ret;
}

static int do_smcfg_h(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	int ret = CMD_RET_SUCCESS;
	u32 selected_index=0;
	char *endptr;

	if (argc == 3) {
		selected_index = simple_strtoul(argv[2], &endptr, 10);
		if (*endptr != '\0') {
			printf( "Invalid input parsing for a %d %s \n",selected_index,argv[2]);
			return ret;
		}

		sm_hw_tag.sm_serial = selected_index;
		printf( "Entered value - %d \n",selected_index);
		return ret;
	}

	printf( "=> %s \n",serial_number_title);
	printf( " Serialnumber - %d \n",sm_hw_tag.sm_serial);

	return ret;
}

static int configure_smcfg()
{
	printf(" Test : %s \n",__func__);

	/* Convert serial number to MAC address (the storage format) */
//	sm_eth_addr.oui = htonl(SILMATES_OUI << 8);
//	sm_eth_addr.nic = htonl(sm_serial << 8);

}


/**
 * mac_read_from_eeprom - read the MAC addresses from EEPROM
 *
 * This function reads the MAC addresses from EEPROM and sets the
 * appropriate environment variables for each one read.
 *
 * The environment variables are only set if they haven't been set already.
 * This ensures that any user-saved variables are never overwritten.
 *
 * This function must be called after relocation.
 *
 */
int mac_read_from_eeprom(void)
{
	printf(" Test : %s \n",__func__);
	unsigned int i;
	int ret;

	puts("EEPROM: ");

	ret = read_sm_eeprom_data(0,0xFA,sm_hw_tag.eth_addr,6);
	if (ret) {
		printf("Read failed.\n");
		return 0;
	}

	if (memcmp(sm_hw_tag.eth_addr, "\0\0\0\0\0\0", 6) &&
	    memcmp(sm_hw_tag.eth_addr, "\xFF\xFF\xFF\xFF\xFF\xFF", 6)) {
		char ethaddr[18];

		sprintf(ethaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
			sm_hw_tag.eth_addr[0],
			sm_hw_tag.eth_addr[1],
			sm_hw_tag.eth_addr[2],
			sm_hw_tag.eth_addr[3],
			sm_hw_tag.eth_addr[4],
			sm_hw_tag.eth_addr[5]);
		/* Only initialize environment variables that are blank
		 * (i.e. have not yet been set)
		 */
		if (!env_get("ethaddr"))
			env_set("ethaddr", ethaddr);

		printf("MAC ID - %s\n",
		       ethaddr);
	} else {
		printf("Invalid ID (%02x %02x %02x %02x)\n",
		       sm_hw_tag.eth_addr[0], sm_hw_tag.eth_addr[1], 
		       sm_hw_tag.eth_addr[2], sm_hw_tag.eth_addr[3]);
		return 0;
	}

	return 0;
}

static int do_smcfg_save(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	u8 *config_block;
	size_t size = SM_CFG_BLOCK_MAX_SIZE;
	int offset = 0;
	int ret = CMD_RET_SUCCESS;
	int err;

	if (argc >= 3) {
		printf("Invalid save options \n");
		return CMD_RET_USAGE;
	}

	/* Allocate RAM area for config block */
	config_block = memalign(ARCH_DMA_MINALIGN, size);
	if (!config_block) {
		printf("Not enough malloc space available!\n");
		return CMD_RET_FAILURE;
	}

	memset(config_block, 0xff, size);

	/* update config block variables */
	configure_smcfg();

	/* write config block to their respective location */
	ret = write_smcfg(config_block);

out:
	free(config_block);
	return ret;
}

static int do_smcfg_print(struct cmd_tbl *cmdtp, int flag, int argc,
			      char * const argv[])
{
	printf(" Test : %s \n",__func__);
	int ret = CMD_RET_SUCCESS;

	if (argc >= 3) {
		printf("Invalid save options \n");
		return CMD_RET_USAGE;
	}

//	sm_eth_addr.oui = htonl(SILMATES_OUI << 8);
//	sm_eth_addr.nic = htonl(sm_serial << 8);
	
	smcfg_print();
	return ret;
}

static int do_smcfg(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	printf(" Test : %s \n",__func__);
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "create")) {
		return do_smcfg_create(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "reload")) {
		ret = read_sm_cfg_block();
		if (ret) {
			printf("Failed to reload Silmates config block: %d\n",
			       ret);
			return CMD_RET_FAILURE;
		}
		return CMD_RET_SUCCESS;
	} else if (!strcmp(argv[1], "print")) {
		return do_smcfg_print(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "a")) {
		return do_smcfg_a(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "b")) {
		return do_smcfg_b(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "c")) {
		return do_smcfg_c(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "d")) {
		return do_smcfg_d(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "e")) {
		return do_smcfg_e(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "f")) {
		return do_smcfg_f(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "g")) {
		return do_smcfg_g(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "h")) {
		return do_smcfg_h(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "save")) {
		return do_smcfg_save(cmdtp, flag, argc, argv);
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	smcfg, 5, 0, do_smcfg,
	"Silmates config block handling commands",
	"create [-y] [barcode] - (Re-)create Silmates config block\n"
	"smcfg reload - Reload Silmates config block from flash\n"
	"smcfg print - Print Silmates config block available in RAM\n"
	"                                     \n"
	"   Below commands will print available options \n"
	"                  if value of x is not present\n"
	"smcfg a [x] - Module\n"
	"smcfg b [x] - Wifi\n"
	"smcfg c [x] - Storage\n"
	"smcfg d [x] - RAM size\n"
	"smcfg e [x] - Storage size\n"
	"smcfg f [x] - HW version major\n"
	"smcfg g [x] - HW version minor\n"
	"smcfg h [x] - Serial number\n"
	"smcfg save  - Save config block info to storage"
);
