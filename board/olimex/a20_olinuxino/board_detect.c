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
		printf("%s(): Failed to set bus!\n", __func__);
		return ret;
	}

	if ((ret = i2c_probe(OLIMEX_EEPROM_ADDRESS))) {
		printf("%s(): Failed to probe!\n", __func__);
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
		printf("Error: EEPROM magic header is not valid!\n"
		"Expected:   %08x\n"
		"Got:        %08x\n",
		OLIMEX_EEPROM_MAGIC_HEADER, eeprom->header);
		memset(eeprom, 0xFF, 256);
		return 1;
	}

	crc = crc32(0L, (uint8_t *)eeprom, 252);
	if (eeprom->crc != crc) {
		printf("Error: CRC checksum is not valid!\n"
		"Expected:   %08x\n"
		"Got:        %08x\n",
		crc, eeprom->crc);
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

#if 0
uint32_t olimex_get_eeprom_id(void)
{
	struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;
	return eeprom->id;
}

struct olimex_revision *olimex_get_eeprom_revision(void)
{
	struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;
	return &eeprom->revision;
}

uint32_t olimex_get_eeprom_serial(void)
{
	struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;
	return eeprom->serial;
}

struct olimex_config *olimex_get_eeprom_config(void)
{
	struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;
	return &eeprom->config;
}

char *olimex_get_eeprom_mac(void)
{
	struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;
	return eeprom->mac;
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

bool olimex_board_is_micro(void)
{
	switch (eeprom->id) {
		case 4614:
		case 8832:
		case 8661:
		case 8828:
		case 4615:
		case 8918:
			return true;

		default:
			break;
	}

	return false;
}

bool olimex_board_is_lime(void)
{
	switch (eeprom->id) {
		case 7739:
		case 7743:
		case 8934:
			return true;

		default:
			break;
	}

	return false;
}

bool olimex_board_is_lime2(void)
{
	switch (eeprom->id) {
		case 7701:
		case 8340:
		case 7624:
		case 8910:
		case 8946:
			return true;

		default:
			break;
	}

	return false;
}

bool olimex_board_is_som204(void)
{
	switch (eeprom->id) {
		case 8991:
		case 8958:
			return true;

		default:
			break;
	}

	return false;
}

#ifndef CONFIG_SPL_BUILD



#if 0
struct olimex_config *olimex_get_board_config(void)
{
	struct board_table *board = olinuxino_ids;
	uint32_t id = olimex_get_eeprom_id();

	while(board->id != 0) {
		if (id == board->id )
			return &board->config;
		board++;
	}
	return NULL;
}
#endif


#if 0
const char * olimex_get_board_fdt(void)
{
	struct board_table *board = olinuxino_ids;
	uint32_t id = olimex_get_eeprom_id();

	while(board->id != 0) {
		if (id == board->id)
			return board->dtb;
		board++;
	}
	return "";
}

const char *olimex_get_board_overlays(void)
{
	struct board_table *board = olinuxino_ids;
	uint32_t id = olimex_get_eeprom_id();

	while(board->id != 0) {
		if (id == board->id)
			return board->overlays;
		board++;
	}
	return "";
}

struct board_table * olimex_get_board_list(void)
{
	return olinuxino_ids;
}

void olimex_set_eeprom_id(uint16_t id)
{
	struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;
	eeprom->id = id;
}

void olimex_set_eeprom_serial(uint32_t serial)
{
	struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;
	eeprom->serial = serial;
}

void olimex_set_eeprom_revision(struct olimex_revision *revision)
{
	struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;
	eeprom->revision.major = revision->major;
	eeprom->revision.minor = revision->minor;
}

void olimex_set_eeprom_config(struct olimex_config *config)
{
	struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;
	memcpy(&eeprom->config, config, OLIMEX_EEPROM_CONFIG_LENGTH);
}

void olimex_set_eeprom_mac(char *mac)
{
	struct olimex_eeprom *eeprom = OLIMEX_EEPROM_DATA;
	memcpy(eeprom->mac, mac, OLIMEX_EEPROM_MAC_LENGTH);
}
#endif

#endif /* CONFIG_SPL_BUILD */
