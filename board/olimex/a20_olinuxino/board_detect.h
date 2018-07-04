/*
 * Copyright (C) 2018 Olimex Ltd.
 *   Author: Stefan Mavrodiev <stefan@olimex.com>
 *
 * SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 */
#ifndef __BOARD_DETECT_H
#define __BOARD_DETECT_H

/**
 * Location in the RAM.
 * Content should be transfered after relocation
 */
#define OLIMEX_EEPROM_DATA ((struct olimex_eeprom *)CONFIG_SYS_SDRAM_BASE)
extern struct olimex_eeprom *eeprom;

/**
 * Define EEPROM bus and address
 */
#define OLIMEX_EEPROM_BUS		I2C_1
#define OLIMEX_EEPROM_ADDRESS		0x50
#define OLIMEX_EEPROM_MAGIC_HEADER	0x4f4caa55

struct olimex_eeprom {
	uint32_t header;
	uint32_t id;
	struct {
		char major;
		char minor;
	} revision;
	uint32_t serial;
	struct {
		uint8_t storage;
		uint8_t size;
		uint8_t ram;
		uint8_t grade;
	} config;
	char mac[12];
	uint8_t reserved[222];
	uint32_t crc;
} __attribute__ ((__packed__));


/* I2C access functions */
int olimex_i2c_eeprom_read(void);
int olimex_i2c_eeprom_write(void);
int olimex_i2c_eeprom_erase(void);

bool olimex_eeprom_is_valid(void);


#if 0
struct olimex_revision *olimex_get_eeprom_revision(void);
struct olimex_config *olimex_get_eeprom_config(void);
uint32_t olimex_get_eeprom_serial(void);
uint32_t olimex_get_eeprom_id(void);
char *olimex_get_eeprom_mac(void);


void olimex_set_eeprom_revision(struct olimex_revision *revision);
void olimex_set_eeprom_config(struct olimex_config *config);
void olimex_set_eeprom_serial(uint32_t serial);
void olimex_set_eeprom_id(uint16_t id);
void olimex_set_eeprom_mac(char *mac);


struct olimex_config *olimex_get_board_config(void);
struct board_table *olimex_get_board_list(void);
const char *olimex_get_board_overlays(void);
#endif

#if 0

#endif

// bool olimex_board_is_micro(void);
// bool olimex_board_is_lime(void);
// bool olimex_board_is_lime2(void);
// bool olimex_board_is_som204(void);

#endif	/* __BOARD_DETECT_H */
