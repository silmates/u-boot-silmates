/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2016 Silmates, Inc.
 */

#ifndef _SM_COMMON_H
#define _SM_COMMON_H

#define SILMATES_USB_PRODUCT_NUM_OFFSET	0x4000
#define SM_USB_VID			0x1B67
//#define SILMATES_OUI 			0x00142dUL

int ft_common_board_setup(void *blob, struct bd_info *bd);
u32 get_board_revision(void);

#if defined(CONFIG_DM_VIDEO)
int show_boot_logo(void);
#endif

#endif /* _SM_COMMON_H */
