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
#include <linux/err.h>
#include <linux/delay.h>

#define CMD_TABLE_LEN 2
typedef u8 cmd_set_table[CMD_TABLE_LEN];

/* Write Manufacture Command Set Control */
#define WRMAUCCTR 0xFE

struct rm67191_panel_priv {
	struct gpio_desc reset;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
};

struct rad_platform_data {
	int (*enable)(struct udevice *dev);
};

/* Manufacturer Command Set pages (CMD2) */
static const cmd_set_table mcs_rm67191[] = {
	{0xFE, 0x0B},
	{0x28, 0x40},
	{0x29, 0x4F},
	{0xFE, 0x0E},
	{0x4B, 0x00},
	{0x4C, 0x0F},
	{0x4D, 0x20},
	{0x4E, 0x40},
	{0x4F, 0x60},
	{0x50, 0xA0},
	{0x51, 0xC0},
	{0x52, 0xE0},
	{0x53, 0xFF},
	{0xFE, 0x0D},
	{0x18, 0x08},
	{0x42, 0x00},
	{0x08, 0x41},
	{0x46, 0x02},
	{0x72, 0x09},
	{0xFE, 0x0A},
	{0x24, 0x17},
	{0x04, 0x07},
	{0x1A, 0x0C},
	{0x0F, 0x44},
	{0xFE, 0x04},
	{0x00, 0x0C},
	{0x05, 0x08},
	{0x06, 0x08},
	{0x08, 0x08},
	{0x09, 0x08},
	{0x0A, 0xE6},
	{0x0B, 0x8C},
	{0x1A, 0x12},
	{0x1E, 0xE0},
	{0x29, 0x93},
	{0x2A, 0x93},
	{0x2F, 0x02},
	{0x31, 0x02},
	{0x33, 0x05},
	{0x37, 0x2D},
	{0x38, 0x2D},
	{0x3A, 0x1E},
	{0x3B, 0x1E},
	{0x3D, 0x27},
	{0x3F, 0x80},
	{0x40, 0x40},
	{0x41, 0xE0},
	{0x4F, 0x2F},
	{0x50, 0x1E},
	{0xFE, 0x06},
	{0x00, 0xCC},
	{0x05, 0x05},
	{0x07, 0xA2},
	{0x08, 0xCC},
	{0x0D, 0x03},
	{0x0F, 0xA2},
	{0x32, 0xCC},
	{0x37, 0x05},
	{0x39, 0x83},
	{0x3A, 0xCC},
	{0x41, 0x04},
	{0x43, 0x83},
	{0x44, 0xCC},
	{0x49, 0x05},
	{0x4B, 0xA2},
	{0x4C, 0xCC},
	{0x51, 0x03},
	{0x53, 0xA2},
	{0x75, 0xCC},
	{0x7A, 0x03},
	{0x7C, 0x83},
	{0x7D, 0xCC},
	{0x82, 0x02},
	{0x84, 0x83},
	{0x85, 0xEC},
	{0x86, 0x0F},
	{0x87, 0xFF},
	{0x88, 0x00},
	{0x8A, 0x02},
	{0x8C, 0xA2},
	{0x8D, 0xEA},
	{0x8E, 0x01},
	{0x8F, 0xE8},
	{0xFE, 0x06},
	{0x90, 0x0A},
	{0x92, 0x06},
	{0x93, 0xA0},
	{0x94, 0xA8},
	{0x95, 0xEC},
	{0x96, 0x0F},
	{0x97, 0xFF},
	{0x98, 0x00},
	{0x9A, 0x02},
	{0x9C, 0xA2},
	{0xAC, 0x04},
	{0xFE, 0x06},
	{0xB1, 0x12},
	{0xB2, 0x17},
	{0xB3, 0x17},
	{0xB4, 0x17},
	{0xB5, 0x17},
	{0xB6, 0x11},
	{0xB7, 0x08},
	{0xB8, 0x09},
	{0xB9, 0x06},
	{0xBA, 0x07},
	{0xBB, 0x17},
	{0xBC, 0x17},
	{0xBD, 0x17},
	{0xBE, 0x17},
	{0xBF, 0x17},
	{0xC0, 0x17},
	{0xC1, 0x17},
	{0xC2, 0x17},
	{0xC3, 0x17},
	{0xC4, 0x0F},
	{0xC5, 0x0E},
	{0xC6, 0x00},
	{0xC7, 0x01},
	{0xC8, 0x10},
	{0xFE, 0x06},
	{0x95, 0xEC},
	{0x8D, 0xEE},
	{0x44, 0xEC},
	{0x4C, 0xEC},
	{0x32, 0xEC},
	{0x3A, 0xEC},
	{0x7D, 0xEC},
	{0x75, 0xEC},
	{0x00, 0xEC},
	{0x08, 0xEC},
	{0x85, 0xEC},
	{0xA6, 0x21},
	{0xA7, 0x05},
	{0xA9, 0x06},
	{0x82, 0x06},
	{0x41, 0x06},
	{0x7A, 0x07},
	{0x37, 0x07},
	{0x05, 0x06},
	{0x49, 0x06},
	{0x0D, 0x04},
	{0x51, 0x04},
};

static const cmd_set_table mcs_rm67199[] = {
	{0xFE, 0xA0}, {0x2B, 0x18}, {0xFE, 0x70}, {0x7D, 0x05},
	{0x5D, 0x0A}, {0x5A, 0x79}, {0x5C, 0x00}, {0x52, 0x00},
	{0xFE, 0xD0}, {0x40, 0x02}, {0x13, 0x40}, {0xFE, 0x40},
	{0x05, 0x08}, {0x06, 0x08}, {0x08, 0x08}, {0x09, 0x08},
	{0x0A, 0xCA}, {0x0B, 0x88}, {0x20, 0x93}, {0x21, 0x93},
	{0x24, 0x02}, {0x26, 0x02}, {0x28, 0x05}, {0x2A, 0x05},
	{0x74, 0x2F}, {0x75, 0x1E}, {0xAD, 0x00}, {0xFE, 0x60},
	{0x00, 0xCC}, {0x01, 0x00}, {0x02, 0x04}, {0x03, 0x00},
	{0x04, 0x00}, {0x05, 0x07}, {0x06, 0x00}, {0x07, 0x88},
	{0x08, 0x00}, {0x09, 0xCC}, {0x0A, 0x00}, {0x0B, 0x04},
	{0x0C, 0x00}, {0x0D, 0x00}, {0x0E, 0x05}, {0x0F, 0x00},
	{0x10, 0x88}, {0x11, 0x00}, {0x12, 0xCC}, {0x13, 0x0F},
	{0x14, 0xFF}, {0x15, 0x04}, {0x16, 0x00}, {0x17, 0x06},
	{0x18, 0x00}, {0x19, 0x96}, {0x1A, 0x00}, {0x24, 0xCC},
	{0x25, 0x00}, {0x26, 0x02}, {0x27, 0x00}, {0x28, 0x00},
	{0x29, 0x06}, {0x2A, 0x06}, {0x2B, 0x82}, {0x2D, 0x00},
	{0x2F, 0xCC}, {0x30, 0x00}, {0x31, 0x02}, {0x32, 0x00},
	{0x33, 0x00}, {0x34, 0x07}, {0x35, 0x06}, {0x36, 0x82},
	{0x37, 0x00}, {0x38, 0xCC}, {0x39, 0x00}, {0x3A, 0x02},
	{0x3B, 0x00}, {0x3D, 0x00}, {0x3F, 0x07}, {0x40, 0x00},
	{0x41, 0x88}, {0x42, 0x00}, {0x43, 0xCC}, {0x44, 0x00},
	{0x45, 0x02}, {0x46, 0x00}, {0x47, 0x00}, {0x48, 0x06},
	{0x49, 0x02}, {0x4A, 0x8A}, {0x4B, 0x00}, {0x5F, 0xCA},
	{0x60, 0x01}, {0x61, 0xE8}, {0x62, 0x09}, {0x63, 0x00},
	{0x64, 0x07}, {0x65, 0x00}, {0x66, 0x30}, {0x67, 0x00},
	{0x9B, 0x03}, {0xA9, 0x07}, {0xAA, 0x06}, {0xAB, 0x02},
	{0xAC, 0x10}, {0xAD, 0x11}, {0xAE, 0x05}, {0xAF, 0x04},
	{0xB0, 0x10}, {0xB1, 0x10}, {0xB2, 0x10}, {0xB3, 0x10},
	{0xB4, 0x10}, {0xB5, 0x10}, {0xB6, 0x10}, {0xB7, 0x10},
	{0xB8, 0x10}, {0xB9, 0x10}, {0xBA, 0x04}, {0xBB, 0x05},
	{0xBC, 0x00}, {0xBD, 0x01}, {0xBE, 0x0A}, {0xBF, 0x10},
	{0xC0, 0x11}, {0xFE, 0xA0}, {0x22, 0x00},
};


enum ili9881c_op {
	ILI9881C_SWITCH_PAGE,
	ILI9881C_COMMAND,
};

struct ili9881c_instr {
	enum ili9881c_op	op;

	union arg {
		struct cmd {
			u8	cmd;
			u8	data;
		} cmd;
		u8	page;
	} arg;
};

#define ILI9881C_SWITCH_PAGE_INSTR(_page)	\
	{					\
		.op = ILI9881C_SWITCH_PAGE,	\
		.arg = {			\
			.page = (_page),	\
		},				\
	}

#define ILI9881C_COMMAND_INSTR(_cmd, _data)		\
	{						\
		.op = ILI9881C_COMMAND,		\
		.arg = {				\
			.cmd = {			\
				.cmd = (_cmd),		\
				.data = (_data),	\
			},				\
		},					\
	}

static const struct ili9881c_instr ts101wxu_nwo_init[] = {
	ILI9881C_SWITCH_PAGE_INSTR(3),
	ILI9881C_COMMAND_INSTR(0x01, 0x00),
	ILI9881C_COMMAND_INSTR(0x02, 0x00),
	ILI9881C_COMMAND_INSTR(0x03, 0x53),
	ILI9881C_COMMAND_INSTR(0x04, 0xD3),
	ILI9881C_COMMAND_INSTR(0x05, 0x00),
	ILI9881C_COMMAND_INSTR(0x06, 0x0D),
	ILI9881C_COMMAND_INSTR(0x07, 0x08),
	ILI9881C_COMMAND_INSTR(0x08, 0x00),
	ILI9881C_COMMAND_INSTR(0x09, 0x00),
	ILI9881C_COMMAND_INSTR(0x0A, 0x00),
	ILI9881C_COMMAND_INSTR(0x0B, 0x00),
	ILI9881C_COMMAND_INSTR(0x0C, 0x00),
	ILI9881C_COMMAND_INSTR(0x0D, 0x00),
	ILI9881C_COMMAND_INSTR(0x0E, 0x00),
	ILI9881C_COMMAND_INSTR(0x0F, 0x28),
	ILI9881C_COMMAND_INSTR(0x10, 0x28),
	ILI9881C_COMMAND_INSTR(0x11, 0x00),
	ILI9881C_COMMAND_INSTR(0x12, 0x00),
	ILI9881C_COMMAND_INSTR(0x13, 0x00),
	ILI9881C_COMMAND_INSTR(0x14, 0x00),
	ILI9881C_COMMAND_INSTR(0x15, 0x00),
	ILI9881C_COMMAND_INSTR(0x16, 0x00),
	ILI9881C_COMMAND_INSTR(0x17, 0x00),
	ILI9881C_COMMAND_INSTR(0x18, 0x00),
	ILI9881C_COMMAND_INSTR(0x19, 0x00),
	ILI9881C_COMMAND_INSTR(0x1A, 0x00),
	ILI9881C_COMMAND_INSTR(0x1B, 0x00),
	ILI9881C_COMMAND_INSTR(0x1C, 0x00),
	ILI9881C_COMMAND_INSTR(0x1D, 0x00),
	ILI9881C_COMMAND_INSTR(0x1E, 0x40),
	ILI9881C_COMMAND_INSTR(0x1F, 0x80),
	ILI9881C_COMMAND_INSTR(0x20, 0x06),
	ILI9881C_COMMAND_INSTR(0x21, 0x01),
	ILI9881C_COMMAND_INSTR(0x22, 0x00),
	ILI9881C_COMMAND_INSTR(0x23, 0x00),
	ILI9881C_COMMAND_INSTR(0x24, 0x00),
	ILI9881C_COMMAND_INSTR(0x25, 0x00),
	ILI9881C_COMMAND_INSTR(0x26, 0x00),
	ILI9881C_COMMAND_INSTR(0x27, 0x00),
	ILI9881C_COMMAND_INSTR(0x28, 0x33), /* CLK Phase */
	ILI9881C_COMMAND_INSTR(0x29, 0x33), /* CLK overlap */
	ILI9881C_COMMAND_INSTR(0x2A, 0x00),
	ILI9881C_COMMAND_INSTR(0x2B, 0x00),
	ILI9881C_COMMAND_INSTR(0x2C, 0x00),
	ILI9881C_COMMAND_INSTR(0x2D, 0x00),
	ILI9881C_COMMAND_INSTR(0x2E, 0x00),
	ILI9881C_COMMAND_INSTR(0x2F, 0x00),
	ILI9881C_COMMAND_INSTR(0x30, 0x00),
	ILI9881C_COMMAND_INSTR(0x31, 0x00),
	ILI9881C_COMMAND_INSTR(0x32, 0x00),
	ILI9881C_COMMAND_INSTR(0x33, 0x00),
	ILI9881C_COMMAND_INSTR(0x34, 0x03), /* VDD1&2 overlap 0us  */
	ILI9881C_COMMAND_INSTR(0x35, 0x00),
	ILI9881C_COMMAND_INSTR(0x36, 0x00),
	ILI9881C_COMMAND_INSTR(0x37, 0x00),
	ILI9881C_COMMAND_INSTR(0x38, 0x96), /* VDD1&2 toggle 2.5sec */
	ILI9881C_COMMAND_INSTR(0x39, 0x00),
	ILI9881C_COMMAND_INSTR(0x3A, 0x00),
	ILI9881C_COMMAND_INSTR(0x3B, 0x00),
	ILI9881C_COMMAND_INSTR(0x3C, 0x00),
	ILI9881C_COMMAND_INSTR(0x3D, 0x00),
	ILI9881C_COMMAND_INSTR(0x3E, 0x00),
	ILI9881C_COMMAND_INSTR(0x3F, 0x00),
	ILI9881C_COMMAND_INSTR(0x40, 0x00),
	ILI9881C_COMMAND_INSTR(0x41, 0x00),
	ILI9881C_COMMAND_INSTR(0x42, 0x00),
	ILI9881C_COMMAND_INSTR(0x43, 0x00),
	ILI9881C_COMMAND_INSTR(0x44, 0x00),
	ILI9881C_COMMAND_INSTR(0x50, 0x00),
	ILI9881C_COMMAND_INSTR(0x51, 0x23),
	ILI9881C_COMMAND_INSTR(0x52, 0x45),
	ILI9881C_COMMAND_INSTR(0x53, 0x67),
	ILI9881C_COMMAND_INSTR(0x54, 0x89),
	ILI9881C_COMMAND_INSTR(0x55, 0xAB),
	ILI9881C_COMMAND_INSTR(0x56, 0x01),
	ILI9881C_COMMAND_INSTR(0x57, 0x23),
	ILI9881C_COMMAND_INSTR(0x58, 0x45),
	ILI9881C_COMMAND_INSTR(0x59, 0x67),
	ILI9881C_COMMAND_INSTR(0x5A, 0x89),
	ILI9881C_COMMAND_INSTR(0x5B, 0xAB),
	ILI9881C_COMMAND_INSTR(0x5C, 0xCD),
	ILI9881C_COMMAND_INSTR(0x5D, 0xEF),
	ILI9881C_COMMAND_INSTR(0x5E, 0x00),
	ILI9881C_COMMAND_INSTR(0x5F, 0x08),
	ILI9881C_COMMAND_INSTR(0x60, 0x08),
	ILI9881C_COMMAND_INSTR(0x61, 0x06),
	ILI9881C_COMMAND_INSTR(0x62, 0x06),
	ILI9881C_COMMAND_INSTR(0x63, 0x01),
	ILI9881C_COMMAND_INSTR(0x64, 0x01),
	ILI9881C_COMMAND_INSTR(0x65, 0x00),
	ILI9881C_COMMAND_INSTR(0x66, 0x00),
	ILI9881C_COMMAND_INSTR(0x67, 0x02),
	ILI9881C_COMMAND_INSTR(0x68, 0x15),
	ILI9881C_COMMAND_INSTR(0x69, 0x15),
	ILI9881C_COMMAND_INSTR(0x6A, 0x14),
	ILI9881C_COMMAND_INSTR(0x6B, 0x14),
	ILI9881C_COMMAND_INSTR(0x6C, 0x0D),
	ILI9881C_COMMAND_INSTR(0x6D, 0x0D),
	ILI9881C_COMMAND_INSTR(0x6E, 0x0C),
	ILI9881C_COMMAND_INSTR(0x6F, 0x0C),
	ILI9881C_COMMAND_INSTR(0x70, 0x0F),
	ILI9881C_COMMAND_INSTR(0x71, 0x0F),
	ILI9881C_COMMAND_INSTR(0x72, 0x0E),
	ILI9881C_COMMAND_INSTR(0x73, 0x0E),
	ILI9881C_COMMAND_INSTR(0x74, 0x02),
	ILI9881C_COMMAND_INSTR(0x75, 0x08),
	ILI9881C_COMMAND_INSTR(0x76, 0x08),
	ILI9881C_COMMAND_INSTR(0x77, 0x06),
	ILI9881C_COMMAND_INSTR(0x78, 0x06),
	ILI9881C_COMMAND_INSTR(0x79, 0x01),
	ILI9881C_COMMAND_INSTR(0x7A, 0x01),
	ILI9881C_COMMAND_INSTR(0x7B, 0x00),
	ILI9881C_COMMAND_INSTR(0x7C, 0x00),
	ILI9881C_COMMAND_INSTR(0x7D, 0x02),
	ILI9881C_COMMAND_INSTR(0x7E, 0x15),
	ILI9881C_COMMAND_INSTR(0x7F, 0x15),
	ILI9881C_COMMAND_INSTR(0x80, 0x14),
	ILI9881C_COMMAND_INSTR(0x81, 0x14),
	ILI9881C_COMMAND_INSTR(0x82, 0x0D),
	ILI9881C_COMMAND_INSTR(0x83, 0x0D),
	ILI9881C_COMMAND_INSTR(0x84, 0x0C),
	ILI9881C_COMMAND_INSTR(0x85, 0x0C),
	ILI9881C_COMMAND_INSTR(0x86, 0x0F),
	ILI9881C_COMMAND_INSTR(0x87, 0x0F),
	ILI9881C_COMMAND_INSTR(0x88, 0x0E),
	ILI9881C_COMMAND_INSTR(0x89, 0x0E),
	ILI9881C_COMMAND_INSTR(0x8A, 0x02),
	ILI9881C_SWITCH_PAGE_INSTR(4),
	ILI9881C_COMMAND_INSTR(0x6E, 0x2B), /* VGH clamp 15V */
	ILI9881C_COMMAND_INSTR(0x6F, 0x37), /* reg vcl + pumping ratio VGH=3x VGL=-3x */
	ILI9881C_COMMAND_INSTR(0x3A, 0xA4), /* POWER SAVING */
	ILI9881C_COMMAND_INSTR(0x8D, 0x1A), /* VGL clamp -11V */
	ILI9881C_COMMAND_INSTR(0x87, 0xBA), /* ESD */
	ILI9881C_COMMAND_INSTR(0xB2, 0xD1),
	ILI9881C_COMMAND_INSTR(0x88, 0x0B),
	ILI9881C_COMMAND_INSTR(0x38, 0x01),
	ILI9881C_COMMAND_INSTR(0x39, 0x00),
	ILI9881C_COMMAND_INSTR(0xB5, 0x07),
	ILI9881C_COMMAND_INSTR(0x31, 0x75),
	ILI9881C_COMMAND_INSTR(0x3B, 0x98),
	ILI9881C_SWITCH_PAGE_INSTR(1),
	ILI9881C_COMMAND_INSTR(0x22, 0x0A), /* BGR, SS */
	ILI9881C_COMMAND_INSTR(0x31, 0x00), /* Zigzag type3 inversion */
	ILI9881C_COMMAND_INSTR(0x53, 0x40), /* VCOM1 */
	ILI9881C_COMMAND_INSTR(0x55, 0x40), /* VCOM2 */
	ILI9881C_COMMAND_INSTR(0x50, 0x99), /* VREG1OUT 4.5V */
	ILI9881C_COMMAND_INSTR(0x51, 0x94), /* VREG2OUT -4.5V */
	ILI9881C_COMMAND_INSTR(0x60, 0x10), /* SDT 3.5us */
	ILI9881C_COMMAND_INSTR(0x62, 0x20),
	ILI9881C_COMMAND_INSTR(0xA0, 0x00),
	ILI9881C_COMMAND_INSTR(0xA1, 0x00), /* VP251 */
	ILI9881C_COMMAND_INSTR(0xA2, 0x15), /* VP247 */
	ILI9881C_COMMAND_INSTR(0xA3, 0x14), /* VP243 */
	ILI9881C_COMMAND_INSTR(0xA4, 0x1B), /* VP239 */
	ILI9881C_COMMAND_INSTR(0xA5, 0x2F), /* VP231 */
	ILI9881C_COMMAND_INSTR(0xA6, 0x25), /* VP219 */
	ILI9881C_COMMAND_INSTR(0xA7, 0x24), /* VP203 */
	ILI9881C_COMMAND_INSTR(0xA8, 0x80), /* VP175 */
	ILI9881C_COMMAND_INSTR(0xA9, 0x1F), /* VP144 */
	ILI9881C_COMMAND_INSTR(0xAA, 0x2C), /* VP111 */
	ILI9881C_COMMAND_INSTR(0xAB, 0x6C), /* VP80 */
	ILI9881C_COMMAND_INSTR(0xAC, 0x16), /* VP52 */
	ILI9881C_COMMAND_INSTR(0xAD, 0x14), /* VP36 */
	ILI9881C_COMMAND_INSTR(0xAE, 0x4D), /* VP24 */
	ILI9881C_COMMAND_INSTR(0xAF, 0x20), /* VP16 */
	ILI9881C_COMMAND_INSTR(0xB0, 0x29), /* VP12 */
	ILI9881C_COMMAND_INSTR(0xB1, 0x4F), /* VP8 */
	ILI9881C_COMMAND_INSTR(0xB2, 0x5F), /* VP4 */
	ILI9881C_COMMAND_INSTR(0xB3, 0x23), /* VP0 */
	ILI9881C_COMMAND_INSTR(0xC0, 0x00), /* VN255 GAMMA N */
	ILI9881C_COMMAND_INSTR(0xC1, 0x2E), /* VN251 */
	ILI9881C_COMMAND_INSTR(0xC2, 0x3B), /* VN247 */
	ILI9881C_COMMAND_INSTR(0xC3, 0x15), /* VN243 */
	ILI9881C_COMMAND_INSTR(0xC4, 0x16), /* VN239 */
	ILI9881C_COMMAND_INSTR(0xC5, 0x28), /* VN231 */
	ILI9881C_COMMAND_INSTR(0xC6, 0x1A), /* VN219 */
	ILI9881C_COMMAND_INSTR(0xC7, 0x1C), /* VN203 */
	ILI9881C_COMMAND_INSTR(0xC8, 0xA7), /* VN175 */
	ILI9881C_COMMAND_INSTR(0xC9, 0x1B), /* VN144 */
	ILI9881C_COMMAND_INSTR(0xCA, 0x28), /* VN111 */
	ILI9881C_COMMAND_INSTR(0xCB, 0x92), /* VN80 */
	ILI9881C_COMMAND_INSTR(0xCC, 0x1F), /* VN52 */
	ILI9881C_COMMAND_INSTR(0xCD, 0x1C), /* VN36 */
	ILI9881C_COMMAND_INSTR(0xCE, 0x4B), /* VN24 */
	ILI9881C_COMMAND_INSTR(0xCF, 0x1F), /* VN16 */
	ILI9881C_COMMAND_INSTR(0xD0, 0x28), /* VN12 */
	ILI9881C_COMMAND_INSTR(0xD1, 0x4E), /* VN8 */
	ILI9881C_COMMAND_INSTR(0xD2, 0x5C), /* VN4 */
	ILI9881C_COMMAND_INSTR(0xD3, 0x23), /* VN0 */

	ILI9881C_SWITCH_PAGE_INSTR(0),
};

/*
static const struct display_timing default_timing = {
	.pixelclock.typ		= 121000000,
	.hactive.typ		= 1080,
	.hfront_porch.typ	= 20,
	.hback_porch.typ	= 34,
	.hsync_len.typ		= 2,
	.vactive.typ		= 1920,
	.vfront_porch.typ	= 10,
	.vback_porch.typ	= 4,
	.vsync_len.typ		= 2,
	.flags = DISPLAY_FLAGS_HSYNC_LOW |
		 DISPLAY_FLAGS_VSYNC_LOW |
		 DISPLAY_FLAGS_DE_LOW |
		 DISPLAY_FLAGS_PIXDATA_NEGEDGE,
};
*/

static const struct display_timing default_timing = {
	.pixelclock.typ		= 74000000,//148500000,
	.hactive.typ		= 800,
	.hfront_porch.typ	= 60,
	.hback_porch.typ	= 60,
	.hsync_len.typ		= 16,
	.vactive.typ		= 1280,
	.vfront_porch.typ	= 10,
	.vback_porch.typ	= 10,
	.vsync_len.typ		= 4,
	.flags = DISPLAY_FLAGS_HSYNC_LOW |
		 DISPLAY_FLAGS_VSYNC_LOW |
		 DISPLAY_FLAGS_DE_LOW |
		 DISPLAY_FLAGS_PIXDATA_NEGEDGE,
};


/*
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
	.flags = DISPLAY_FLAGS_HSYNC_LOW |
		 DISPLAY_FLAGS_VSYNC_LOW |
		 DISPLAY_FLAGS_DE_LOW |
		 DISPLAY_FLAGS_PIXDATA_NEGEDGE,
};
*/


static u8 color_format_from_dsi_format(enum mipi_dsi_pixel_format format)
{
	switch (format) {
	case MIPI_DSI_FMT_RGB565:
		return 0x55;
	case MIPI_DSI_FMT_RGB666:
	case MIPI_DSI_FMT_RGB666_PACKED:
		return 0x66;
	case MIPI_DSI_FMT_RGB888:
		return 0x77;
	default:
		return 0x77; /* for backward compatibility */
	}
};

static int rad_panel_push_cmd_list(struct mipi_dsi_device *device,
				   const cmd_set_table *cmd_set,
				   size_t count)
{
	size_t i;
	const cmd_set_table *cmd;
	int ret = 0;

	for (i = 0; i < count; i++) {
		cmd = cmd_set++;
		ret = mipi_dsi_generic_write(device, cmd, CMD_TABLE_LEN);
		if (ret < 0)
			return ret;
	}

	return ret;
};

static int rm67191_enable(struct udevice *dev)
{
	struct rm67191_panel_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *dsi = plat->device;
	u8 color_format = color_format_from_dsi_format(priv->format);
	u16 brightness;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = rad_panel_push_cmd_list(dsi, &mcs_rm67191[0],
		sizeof(mcs_rm67191) / CMD_TABLE_LEN);
	if (ret < 0) {
		printf("Failed to send MCS (%d)\n", ret);
		return -EIO;
	}

	/* Select User Command Set table (CMD1) */
	ret = mipi_dsi_generic_write(dsi, (u8[]){ WRMAUCCTR, 0x00 }, 2);
	if (ret < 0)
		return -EIO;

	/* Software reset */
	ret = mipi_dsi_dcs_soft_reset(dsi);
	if (ret < 0) {
		printf("Failed to do Software Reset (%d)\n", ret);
		return -EIO;
	}

	/* Wait 80ms for panel out of reset */
	mdelay(80);

	/* Set DSI mode */
	ret = mipi_dsi_generic_write(dsi, (u8[]){ 0xC2, 0x0B }, 2);
	if (ret < 0) {
		printf("Failed to set DSI mode (%d)\n", ret);
		return -EIO;
	}

	/* Set tear ON */
	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		printf("Failed to set tear ON (%d)\n", ret);
		return -EIO;
	}

	/* Set tear scanline */
	ret = mipi_dsi_dcs_set_tear_scanline(dsi, 0x380);
	if (ret < 0) {
		printf("Failed to set tear scanline (%d)\n", ret);
		return -EIO;
	}

	/* Set pixel format */
	ret = mipi_dsi_dcs_set_pixel_format(dsi, color_format);
	if (ret < 0) {
		printf("Failed to set pixel format (%d)\n", ret);
		return -EIO;
	}


	/* Set display brightness */
	brightness = 255; /* Max brightness */
	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 2);
	if (ret < 0) {
		printf("Failed to set display brightness (%d)\n",
				  ret);
		return -EIO;
	}

	/* Exit sleep mode */
	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		printf("Failed to exit sleep mode (%d)\n", ret);
		return -EIO;
	}

	mdelay(5);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		printf("Failed to set display ON (%d)\n", ret);
		return -EIO;
	}

	return 0;
}

static int rm67199_enable(struct udevice *dev)
{
	struct rm67191_panel_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *dsi = plat->device;
	u8 color_format = color_format_from_dsi_format(priv->format);
	u16 brightness;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = rad_panel_push_cmd_list(dsi, &mcs_rm67199[0],
		sizeof(mcs_rm67199) / CMD_TABLE_LEN);
	if (ret < 0) {
		printf("Failed to send MCS (%d)\n", ret);
		return -EIO;
	}

	/* Select User Command Set table (CMD1) */
	ret = mipi_dsi_generic_write(dsi, (u8[]){ WRMAUCCTR, 0x00 }, 2);
	if (ret < 0)
		return -EIO;

	/* Set DSI mode */
	ret = mipi_dsi_generic_write(dsi, (u8[]){ 0xC2, 0x08 }, 2);
	if (ret < 0) {
		printf("Failed to set DSI mode (%d)\n", ret);
		return -EIO;
	}

	/* Set tear ON */
	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		printf("Failed to set tear ON (%d)\n", ret);
		return -EIO;
	}

	/* Set tear scanline */
	ret = mipi_dsi_dcs_set_tear_scanline(dsi, 0x00);
	if (ret < 0) {
		printf("Failed to set tear scanline (%d)\n", ret);
		return -EIO;
	}

	/* Set pixel format */
	ret = mipi_dsi_dcs_set_pixel_format(dsi, color_format);
	if (ret < 0) {
		printf("Failed to set pixel format (%d)\n", ret);
		return -EIO;
	}

	/* Set display brightness */
	brightness = 255; /* Max brightness */
	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 2);
	if (ret < 0) {
		printf("Failed to set display brightness (%d)\n",
				  ret);
		return -EIO;
	}

	/* Exit sleep mode */
	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		printf("Failed to exit sleep mode (%d)\n", ret);
		return -EIO;
	}

	mdelay(120);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		printf("Failed to set display ON (%d)\n", ret);
		return -EIO;
	}

	mdelay(100);

	return 0;
}


/*
 * The panel seems to accept some private DCS commands that map
 * directly to registers.
 *
 * It is organised by page, with each page having its own set of
 * registers, and the first page looks like it's holding the standard
 * DCS commands.
 *
 * So before any attempt at sending a command or data, we have to be
 * sure if we're in the right page or not.
 */
static int ili9881c_switch_page(struct mipi_dsi_device *dsi, u8 page)
{
	u8 buf[4] = { 0xff, 0x98, 0x81, page };
	int ret;

	ret = mipi_dsi_generic_write(dsi, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	return 0;
}

static int ili9881c_send_cmd_data(struct mipi_dsi_device *dsi, u8 cmd, u8 data)
{
	u8 buf[2] = { cmd, data };
	int ret;

	ret = mipi_dsi_generic_write(dsi, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	return 0;
}

static int ili9881c_enable(struct udevice *dev)
{
	struct rm67191_panel_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *dsi = plat->device;
	u8 color_format = color_format_from_dsi_format(priv->format);
	u16 brightness;
	int ret;
	int i;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	for (i = 0; i < ARRAY_SIZE(ts101wxu_nwo_init); i++) {
		const struct ili9881c_instr *instr = &ts101wxu_nwo_init[i];

		if (instr->op == ILI9881C_SWITCH_PAGE)
			ret = ili9881c_switch_page(dsi, instr->arg.page);
		else if (instr->op == ILI9881C_COMMAND)
			ret = ili9881c_send_cmd_data(dsi, instr->arg.cmd.cmd,
						      instr->arg.cmd.data);

		if (ret) {
			printf("Failed to send MCS (%d)\n", ret);
			return ret;
		}
	}

	/* Select User Command Set table (CMD1) */
	ret = mipi_dsi_generic_write(dsi, (u8[]){ WRMAUCCTR, 0x00 }, 2);
	if (ret < 0)
		return -EIO;

	/* Set DSI mode */
	ret = mipi_dsi_generic_write(dsi, (u8[]){ 0xC2, 0x08 }, 2);
	if (ret < 0) {
		printf("Failed to set DSI mode (%d)\n", ret);
		return -EIO;
	}

	/* Set tear ON */
	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		printf("Failed to set tear ON (%d)\n", ret);
		return -EIO;
	}

	/* Set tear scanline */
	ret = mipi_dsi_dcs_set_tear_scanline(dsi, 0x00);
	if (ret < 0) {
		printf("Failed to set tear scanline (%d)\n", ret);
		return -EIO;
	}

	/* Set pixel format */
	ret = mipi_dsi_dcs_set_pixel_format(dsi, color_format);
	if (ret < 0) {
		printf("Failed to set pixel format (%d)\n", ret);
		return -EIO;
	}

	/* Set display brightness */
	brightness = 255; /* Max brightness */
	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 2);
	if (ret < 0) {
		printf("Failed to set display brightness (%d)\n",
				  ret);
		return -EIO;
	}

	/* Exit sleep mode */
	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		printf("Failed to exit sleep mode (%d)\n", ret);
		return -EIO;
	}

	mdelay(120);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		printf("Failed to set display ON (%d)\n", ret);
		return -EIO;
	}

	mdelay(100);

	return 0;
}
static int rm67191_panel_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct rad_platform_data *data = (struct rad_platform_data *)dev_get_driver_data(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
		return ret;

	return data->enable(dev);
}

static int rm67191_panel_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	struct rm67191_panel_priv *priv = dev_get_priv(dev);

	memcpy(timings, &default_timing, sizeof(*timings));

	/* fill characteristics of DSI data link */
	if (device) {
		device->lanes = priv->lanes;
		device->format = priv->format;
		device->mode_flags = priv->mode_flags;
	}

	return 0;
}

static int rm67191_panel_probe(struct udevice *dev)
{
	struct rm67191_panel_priv *priv = dev_get_priv(dev);
	int ret;
	u32 video_mode;

	priv->format = MIPI_DSI_FMT_RGB888;
	priv->mode_flags = MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO;

	ret = dev_read_u32(dev, "video-mode", &video_mode);
	if (!ret) {
		switch (video_mode) {
		case 0:
			/* burst mode */
			priv->mode_flags |= MIPI_DSI_MODE_VIDEO_BURST;
			break;
		case 1:
			/* non-burst mode with sync event */
			break;
		case 2:
			/* non-burst mode with sync pulse */
			priv->mode_flags |= MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
			break;
		default:
			dev_warn(dev, "invalid video mode %d\n", video_mode);
			break;
		}
	}

	ret = dev_read_u32(dev, "dsi-lanes", &priv->lanes);
	if (ret) {
		printf("Failed to get dsi-lanes property (%d)\n", ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "reset-gpio", 0, &priv->reset,
				   GPIOD_IS_OUT);
	if (ret) {
		printf("Warning: cannot get reset GPIO\n");
		if (ret != -ENOENT)
			return ret;
	}

	/* reset panel */
	ret = dm_gpio_set_value(&priv->reset, true);
	if (ret)
		printf("reset gpio fails to set true\n");
	mdelay(100);
	ret = dm_gpio_set_value(&priv->reset, false);
	if (ret)
		printf("reset gpio fails to set true\n");
	mdelay(100);

	return 0;
}

static int rm67191_panel_disable(struct udevice *dev)
{
	struct rm67191_panel_priv *priv = dev_get_priv(dev);

	dm_gpio_set_value(&priv->reset, true);

	return 0;
}

static const struct panel_ops rm67191_panel_ops = {
	.enable_backlight = rm67191_panel_enable_backlight,
	.get_display_timing = rm67191_panel_get_display_timing,
};

static const struct rad_platform_data rad_rm67191 = {
	.enable = &rm67191_enable,
};

static const struct rad_platform_data rad_rm67199 = {
	.enable = &rm67199_enable,
};

static const struct rad_platform_data ili_ili9881c = {
	.enable = &ili9881c_enable,
};

static const struct udevice_id rm67191_panel_ids[] = {
	{ .compatible = "raydium,rm67191", .data = (ulong)&rad_rm67191 },
	{ .compatible = "raydium,rm67199", .data = (ulong)&rad_rm67199 },
	{ .compatible = "ilitek,ili9881c", .data = (ulong)&ili_ili9881c },
	{ }
};

U_BOOT_DRIVER(rm67191_panel) = {
	.name			  = "rm67191_panel",
	.id			  = UCLASS_PANEL,
	.of_match		  = rm67191_panel_ids,
	.ops			  = &rm67191_panel_ops,
	.probe			  = rm67191_panel_probe,
	.remove			  = rm67191_panel_disable,
	.plat_auto = sizeof(struct mipi_dsi_panel_plat),
	.priv_auto = sizeof(struct rm67191_panel_priv),
};
