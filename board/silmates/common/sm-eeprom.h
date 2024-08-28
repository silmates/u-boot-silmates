/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2020 Silmates
 */

#ifndef _SM_EEPROM_H
#define _SM_EEPROM_H

#include <i2c_eeprom.h>

int read_sm_eeprom_data(u32 eeprom_id, int offset, uint8_t *buf, int size);
int write_sm_eeprom_data(u32 eeprom_id, int offset, uint8_t *buf, int size);

#endif /* _SM_EEPROM_H */
