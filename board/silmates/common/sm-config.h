/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2016-2020 Silmates
 */

#ifndef _SM_CFG_BLOCK_H
#define _SM_CFG_BLOCK_H

#include "sm-common.h"

#define VALIDATE_PARAM_RANGE(x,min,max)		(((x > max) | (x < 0) ) ? 0 : x)

typedef struct {
	u16 processor;
	u8 wifi;
	u8 storage;
	u8 ram_size;
	u8 storage_size;
} silmates_board_info;


struct silmates_eth_addr {
	u32 oui:24;
	u32 nic:24;
} __attribute__((__packed__));

struct silmates_hw {
	u16 tag_id;
	u16 ver_major;
	u16 ver_minor;
	u16 ver_assembly;
	u32 sm_serial;
	silmates_board_info prodid;
	u8 eth_addr[6];
	u32 crc;
} __attribute__((__packed__));

enum {
	SM_PROCESSOR_NONE = 0,
	SM_PROCESSOR_iMX8MM_MEZ,
	SM_PROCESSOR_iMX8MN_MEZ,
	SM_PROCESSOR_MAX,
};

enum {
	SM_WIFI_NONE = 0,
	SM_WIFI_WITHOUT,
	SM_WIFI_WITH,
	SM_WIFI_MAX,
};

enum {
	SM_STORAGE_NONE = 0,
	SM_STORAGE_EMMC,
	SM_STORAGE_NAND,
	SM_STORAGE_USD,
	SM_STORAGE_NOR,
	SM_STORAGE_MAX,
};

enum {
	SM_RAM_SIZE_NONE = 0,
	SM_RAM_SIZE_128MB,
	SM_RAM_SIZE_256MB,
	SM_RAM_SIZE_512MB,
	SM_RAM_SIZE_1GB,
	SM_RAM_SIZE_2GB,
	SM_RAM_SIZE_4GB,
	SM_RAM_SIZE_8GB,
	SM_RAM_SIZE_MAX,
};

enum {
	SM_STORAGE_SIZE_NONE = 0,
	SM_STORAGE_SIZE_128MB,
	SM_STORAGE_SIZE_256MB,
	SM_STORAGE_SIZE_512MB,
	SM_STORAGE_SIZE_1GB,
	SM_STORAGE_SIZE_2GB,
	SM_STORAGE_SIZE_4GB,
	SM_STORAGE_SIZE_8GB,
	SM_STORAGE_SIZE_MAX,
};

extern const char * silmates_modules[];
extern const char * silmates_wifi_modules[];
extern const char * silmates_storage_modules[];
extern const char * silmates_ram_size_modules[];
extern const char * silmates_storage_size_modules[];

extern const char * processor_title;
extern const char * wifi_title;
extern const char * storage_title;
extern const char * ram_size_title;
extern const char * storage_size_title;

extern const char * hw_major_title;
extern const char * hw_minor_title;
extern const char * serial_number_title;

extern bool valid_smcfg;
extern struct silmates_hw sm_hw_tag;

int read_sm_cfg_block(void);


#endif /* _SM_CFG_BLOCK_H */
