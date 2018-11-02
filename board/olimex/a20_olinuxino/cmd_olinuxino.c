/*
 *
 * Copyright (C) 2018 Olimex Ltd.
 *   Author: Stefan Mavrodiev <stefan@olimex.com>
 *
 * SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 */
#include <common.h>
#include <linux/ctype.h>

#include "board_detect.h"
#include "boards.h"

static int do_config_info(cmd_tbl_t *cmdtp, int flag,
			  int argc, char *const argv[])
{
	char *mac = eeprom->mac;
	const char *name;
	uint8_t i;

	if (olimex_i2c_eeprom_read()) {
		printf("Failed to read the current EEPROM configuration!\n");
		return CMD_RET_FAILURE;
	}
	if (!olimex_eeprom_is_valid()) {
		printf("Current configuration in the EEPROM is not valid!\n"
		       "Run \"olimex config write\" to restore it.\n");
		return CMD_RET_SUCCESS;
	}

	/* Get board info */
	name = olimex_get_board_name(eeprom->id);

	printf("Model: %s Rev.%c%c", name,
	       (eeprom->revision.major < 'A' || eeprom->revision.major > 'Z') ?
	       0 : eeprom->revision.major,
	       (eeprom->revision.minor < '1' || eeprom->revision.minor > '9') ?
	       0 : eeprom->revision.minor);

	printf("\nSerial:%08X\n", eeprom->serial);
	printf("MAC:   ");
	for (i = 0; i < 12; i += 2 ) {
		if (i < 10)
			printf("%c%c:",
				(mac[i] == 0xFF) ? 'F' : mac[i],
				(mac[i+1] == 0xFF) ? 'F' : mac[i+1]);
		else
			printf("%c%c\n",
				(mac[i] == 0xFF) ? 'F' : mac[i],
				(mac[i+1] == 0xFF) ? 'F' : mac[i+1]);
	}

	return CMD_RET_SUCCESS;
}

static int do_config_list(cmd_tbl_t *cmdtp, int flag,
			  int argc, char *const argv[])
{
	struct olinuxino_boards *board;

	printf("\nSupported boards:\n");
	printf("----------------------------------------\n");

	for (board = olinuxino_boards; board->id != 0; board++)
		printf("%-30s - %-10d\n", board->name, board->id);
	return CMD_RET_SUCCESS;
}

static int do_config_write(cmd_tbl_t *cmdtp, int flag,
			   int argc, char *const argv[])
{
	struct olinuxino_boards *board = olinuxino_boards;
	struct olimex_eeprom info;
	uint32_t sid[4];
	char mac[13];
	uint32_t id;
	uint8_t i = 0;
	char *p;

	if (argc < 3 || argc > 5)
		return CMD_RET_USAGE;

	memset(&info, 0xFF, 256);

	id = simple_strtoul(argv[1], NULL, 10);
	do {
		if (board->id == id)
			break;

		board++;
		if (board->id == 0) {
			printf("%d is not valid ID!\n"
			       "Run olimex config list to get supported IDs.\n", id);
			return CMD_RET_FAILURE;
		}
	} while (board->id != 0);

	info.id = id;
	memcpy(&info.config, board->config, 4);

	info.revision.major = argv[2][0];
	info.revision.minor = '\0';

	/* Make uppercase */
	info.revision.major = toupper(info.revision.major);

	if (info.revision.major < 'A' || info.revision.major > 'Z') {
		printf("%c in not valid revision!\n"
		       "Revision should be one character: A, C, J, etc...\n", info.revision.major);
		return CMD_RET_FAILURE;
	}

	if (argc > 3)
		info.serial = simple_strtoul(argv[3], NULL, 16);


	if (argc > 4) {
		i = 0;
		p = argv[4];
		while (*p) {
			if ((*p < '0' || *p > '9') && (*p < 'a' || *p > 'f') && (*p < 'A' || *p > 'F') && (*p != ':')) {
				printf("Invalid character: %d(%c)!\n", *p, *p);
				return CMD_RET_FAILURE;
			}

			if (*p != ':')
				info.mac[i++] = toupper(*p);
			p++;
		};

		if (i != 12) {
			printf("Invalid MAC address lenght: %d!\n", i);
			return CMD_RET_FAILURE;
		}
	} else {
		sunxi_get_sid(sid);
		if (sid[0] != 0) {

			/* Ensure the NIC specific bytes of the mac are not all 0 */
			if ((sid[3] & 0xffffff) == 0)
				sid[3] |= 0x800000;

			/* Non OUI / registered MAC address */
			snprintf(&mac[0], 3, "%02X", 0x02);
			snprintf(&mac[2], 3, "%02X", (sid[0] >>  0) & 0xff);
			snprintf(&mac[4], 3, "%02X", (sid[3] >> 24) & 0xff);
			snprintf(&mac[6], 3, "%02X", (sid[3] >> 16) & 0xff);
			snprintf(&mac[8], 3, "%02X", (sid[3] >>  8) & 0xff);
			snprintf(&mac[10], 3, "%02X", (sid[3] >>  0) & 0xff);

			memcpy(&info.mac, &mac, 12);

		}
	}

	printf("Erasing previous configuration...\n");
	if (olimex_i2c_eeprom_erase())
		return CMD_RET_FAILURE;

	printf("Writting configuration EEPROM...\n");
	memcpy(eeprom, &info, 256);

	olimex_i2c_eeprom_write();
	olimex_i2c_eeprom_read();
	return CMD_RET_SUCCESS;
}

static int do_config_erase(cmd_tbl_t *cmdtp, int flag,
			   int argc, char *const argv[])
{
	printf("Erasing configuration EEPROM...\n");
	return olimex_i2c_eeprom_erase();
}

static cmd_tbl_t cmd_config[] = {
	U_BOOT_CMD_MKENT(info,	1, 0, do_config_info,  "", ""),
	U_BOOT_CMD_MKENT(list,	1, 0, do_config_list,  "", ""),
	U_BOOT_CMD_MKENT(write, 5, 0, do_config_write, "", ""),
	U_BOOT_CMD_MKENT(erase, 1, 0, do_config_erase, "", ""),
};

static int do_monitor_list(cmd_tbl_t *cmdtp, int flag,
			   int argc, char *const argv[])
{
	printf("\nSupported video outputs:\n");
	printf("----------------------------------------\n");
	printf("%-30s - %-10s\n", "LCD-OLinuXino", "Video outout to the LCD connector with AUTO detect function");
	printf("%-30s - %-10s\n", "LCD-OLinuXino-4.3TS", "Video output to LCD-OLinuXino-4.3TS");
	printf("%-30s - %-10s\n", "LCD-OLinuXino-7", "Video output to LCD-OLinuXino-7");
	printf("%-30s - %-10s\n", "LCD-OLinuXino-5", "Video output to LCD-OLinuXino-5");
	printf("%-30s - %-10s\n", "LCD-OLinuXino-10", "Video output to LCD-OLinuXino-10");

	return CMD_RET_SUCCESS;
}

static int do_monitor_set(cmd_tbl_t *cmdtp, int flag,
			  int argc, char *const argv[])
{
	char *p;
	int i;

	if (argc < 2)
		return CMD_RET_USAGE;

	/* Convert to lowercase */
	p = argv[1];
	for (i = 0; i < strlen(p); i++) {
		p[i] = tolower(p[i]);
	}

	if (!strcmp(p, "lcd-olinuxino"))
		env_set("lcd_olinuxino", "");
	else if (!strcmp(p, "lcd-olinuxino-4.3ts"))
		env_set("lcd_olinuxino", "LCD-OLinuXino-4.3TS");
	else if (!strcmp(p, "lcd-olinuxino-5"))
		env_set("lcd_olinuxino", "LCD-OLinuXino-5");
	else if (!strcmp(p, "lcd-olinuxino-7"))
		env_set("lcd_olinuxino", "LCD-OLinuXino-7");
	else if (!strcmp(p, "lcd-olinuxino-10"))
		env_set("lcd_olinuxino", "LCD-OLinuXino-10");
	else
		printf("Invalid LCD! Run \"olinuxino monitor list\" for supported devices.\n");

	return CMD_RET_SUCCESS;
}

static cmd_tbl_t cmd_monitor[] = {
	U_BOOT_CMD_MKENT(list, 1, 0, do_monitor_list, "", ""),
	U_BOOT_CMD_MKENT(set, 2, 0, do_monitor_set, "", ""),
};


static int do_olinuxino_opts(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *cp;

	if (!strcmp(argv[0], "config"))
		cp = find_cmd_tbl(argv[1], cmd_config, ARRAY_SIZE(cmd_config));
	else if (!strcmp(argv[0], "monitor"))
		cp = find_cmd_tbl(argv[1], cmd_monitor, ARRAY_SIZE(cmd_monitor));
	else
		return CMD_RET_USAGE;

	argc--;
	argv++;

	if (cp == NULL || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cp->repeatable)
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}

static cmd_tbl_t cmd_olinuxino[] = {
	U_BOOT_CMD_MKENT(config, CONFIG_SYS_MAXARGS, 0, do_olinuxino_opts, "", ""),
	U_BOOT_CMD_MKENT(monitor, CONFIG_SYS_MAXARGS, 0, do_olinuxino_opts, "", ""),
};

static int do_olinuxino_ops(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *cp;

	cp = find_cmd_tbl(argv[1], cmd_olinuxino, ARRAY_SIZE(cmd_olinuxino));

	argc--;
	argv++;

	if (cp == NULL || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cp->repeatable)
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(
	olinuxino, 7, 0, do_olinuxino_ops,
	"OLinuXino board configurator",
	"config info		- Print current configuration: ID, serial, ram, storage, grade...\n"
	"olinuxino config list		- Print supported boards and their IDs\n"
	"olinuxino config erase		- Erase currently stored configuration\n"
	"olinuxino config write [id] [revision] [serial] [mac]\n"
	"  arguments:\n"
	"    [id]			- Specific board ID\n"
	"    [revision]			- Board revision: C, D1, etc...\n"
	"    [serial]			- New serial number for the board\n"
	"    [mac]			- New MAC address for the board\n"
	"				  Format can be:\n"
	"					aa:bb:cc:dd:ee:ff\n"
	"					FF:FF:FF:FF:FF:FF\n"
	"					aabbccddeeff\n"
	"olinuxino monitor list		- Print supported video outputs\n"
	"olinuxino monitor set		- Set specific LCD\n"
	);
