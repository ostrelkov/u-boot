/*
 * Copyright (C) 2018 Olimex Ltd.
 *   Author: Stefan Mavrodiev <stefan@olimex.com>
 *
 * SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 */
#include <asm/arch/dram.h>
#include <asm/arch/spl.h>
#include <common.h>
#include <i2c.h>

#include "board_detect.h"

struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;

static int olimex_i2c_eeprom_init(void)
{
	int ret;

	if ((ret = i2c_set_bus_num(OLIMEX_EEPROM_BUS))) {
		debug("%s(): Failed to set bus!\n", __func__);
		return ret;
	}

	if ((ret = i2c_probe(OLIMEX_EEPROM_ADDRESS))) {
		debug("%s(): Failed to probe!\n", __func__);
		return ret;
	}

	return 0;
}

int olimex_i2c_eeprom_read(void)
{
	uint32_t crc;
	int ret;

	if ((ret = olimex_i2c_eeprom_init())) {
		printf("Error: Failed to init EEPROM!\n");
		return ret;
	}

	if ((ret = i2c_read(OLIMEX_EEPROM_ADDRESS, 0, 1, (uint8_t *)eeprom, 256))) {
		printf("Error: Failed to read EEPROM!\n");
		return ret;
	}

	if (eeprom->header != OLIMEX_EEPROM_MAGIC_HEADER) {
		printf("Error: EEPROM magic header is not valid!\n");
		memset(eeprom, 0xFF, 256);
		return 1;
	}

	crc = crc32(0L, (uint8_t *)eeprom, 252);
	if (eeprom->crc != crc) {
		printf("Error: CRC checksum is not valid!\n");
		memset(eeprom, 0xFF, 256);
		return 1;
	}

	return 0;
}

#ifndef CONFIG_SPL_BUILD
int olimex_i2c_eeprom_write(void)
{
	uint8_t *data = (uint8_t *)eeprom;
	uint16_t i;
	int ret;

	if ((ret = olimex_i2c_eeprom_init())) {
		printf("ERROR: Failed to init eeprom!\n");
		return ret;
	}

	/* Restore magic header */
	eeprom->header = OLIMEX_EEPROM_MAGIC_HEADER;

	/* Calculate new chechsum */
	eeprom->crc = crc32(0L, data, 252);

	/* Write new values */
	for(i = 0; i < 256; i += 16) {
		if ((ret = i2c_write(OLIMEX_EEPROM_ADDRESS, i, 1, data + i , 16))) {
			printf("ERROR: Failed to write eeprom!\n");
			return ret;
		}
		mdelay(5);
	}

	return 0;
}

int olimex_i2c_eeprom_erase(void)
{
	uint8_t *data = (uint8_t *)eeprom;
	uint16_t i;
	int ret;

	/* Initialize EEPROM */
	if ((ret = olimex_i2c_eeprom_init())) {
		printf("ERROR: Failed to init eeprom!\n");
		return ret;
	}

	/* Erase previous data */
	memset((uint8_t *)eeprom, 0xFF, 256);

	/* Write data */
	for(i = 0; i < 256; i += 16) {
		if ((ret = i2c_write(OLIMEX_EEPROM_ADDRESS, i, 1, data + i, 16))) {
			printf("ERROR: Failed to write eeprom!\n");
			return ret;
		}
		mdelay(5);
	}

	return 0;
}
#endif

bool olimex_eeprom_is_valid(void)
{
	/*
	 * If checksum during EEPROM initalization was wrong,
	 * then the whole memory location should be empty.
	 * Therefore it's enough to check the magic header
	 */
	return (eeprom->header == OLIMEX_EEPROM_MAGIC_HEADER);
}
