/*
 * Copyright (C) 2018 Olimex Ltd.
 *   Author: Stefan Mavrodiev <stefan@olimex.com>
 *
 * SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 */
#include <common.h>
#include "boards.h"

#ifndef CONFIG_SPLBUILD
struct olinuxino_boards {
	uint32_t id;
	const char name[64];
	const char fdt[64];
	const char overlays[64];
};

static struct olinuxino_boards olinuxino_boards[] = {
	/* A20-OLinuXino-Lime Boards */
	{
		.id = 7739,
		.name = "A20-OLinuXino-LIME",
		.fdt = "sun7i-a20-olinuxino-lime",
		// .config = {
		// 	.ram = MBYTES(SIZE_512),
		// },
	},
	{
		.id = 7743,
		.name = "A20-OLinuXino-LIME-n4GB",
		.fdt = "sun7i-a20-olinuxino-lime",
		// .config = {
		// 	.storage = STORAGE_NAND,
		// 	.size = GBYTES(SIZE_4),
		// 	.ram = MBYTES(SIZE_512),
		// },
	},
	{
		.id = 8934,
		.name = "A20-OLinuXino-LIME-n8GB",
		.fdt = "sun7i-a20-olinuxino-lime",
		// .config = {
		// 	.storage = STORAGE_NAND,
		// 	.size = GBYTES(SIZE_8),
		// 	.ram = MBYTES(SIZE_512)
		// },
	},

	/* A20-OLinuXino-Lime2 */
	{
		.id = 7701,
		.name = "A20-OLinuXIno-LIME2",
		.fdt = "sun7i-a20-olinuxino-lime2",
		// .config = {
		// 	.size = GBYTES(SIZE_1),
		// },
	},
	{
		.id = 8340,
		.name = "A20-OLinuXino-LIME2-e4GB",
		.fdt = "sun7i-a20-olinuxino-lime2-emmc",
		// .config = {
		// 	.storage = STORAGE_EMMC,
		// 	.size = GBYTES(SIZE_4),
		// 	.ram = GBYTES(SIZE_1),
		// },
	},
	{
		.id = 7624,
		.name = "A20-OLinuXIno-LIME2-n4GB",
		.fdt = "sun7i-a20-olinuxino-lime2",
		// .config = {
		// 	.storage = STORAGE_NAND,
		// 	.size = GBYTES(SIZE_4),
		// 	.ram = GBYTES(SIZE_1),
		// },
	},
	{
		.id = 8910,
		.name = "A20-OLinuXIno-LIME2-n8GB",
		.fdt = "sun7i-a20-olinuxino-lime2",
		// .config = {
		// 	.storage = STORAGE_NAND,
		// 	.size = GBYTES(SIZE_4),
		// 	.ram = GBYTES(SIZE_1),
		// },
	},
	{
		.id = 8946,
		.name = "A20-OLinuXIno-LIME2-s16MB",
		.fdt = "sun7i-a20-olinuxino-lime2",
		.overlays = "spi-flash",
		// .config = {
		// 	.storage = STORAGE_FLASH,
		// 	.size = MBYTES(SIZE_16),
		// 	.ram = GBYTES(SIZE_1),
		// },
	},

	/* A20-OLinuXino-MICRO */
	{
		.id = 4614,
		.name = "A20-OLinuXino-MICRO",
		.fdt = "sun7i-a20-olinuxino-micro",
		// .config = {
		// 	.ram = GBYTES(SIZE_1),
		// },
	},
	{
		.id = 8832,
		.name = "A20-OLinuXino-MICRO-e4GB",
		.fdt = "sun7i-a20-olinuxino-micro-emmc",
		// .config = {
		// 	.storage = STORAGE_EMMC,
		// 	.size = GBYTES(SIZE_4),
		// 	.ram = GBYTES(SIZE_1),
		// },
	},
	{
		.id = 9042,
		.name = "A20-OLinuXino-MICRO-e16GB",
		.fdt = "sun7i-a20-olinuxino-micro-emmc",
	},
	{
		.id = 8661,
		.name = "A20-OLinuXino-MICRO-e4GB-IND",
		.fdt = "sun7i-a20-olinuxino-micro-emmc",
		// .config = {
		// 	.storage = STORAGE_EMMC,
		// 	.size = GBYTES(SIZE_4),
		// 	.ram = GBYTES(SIZE_1),
		// 	.grade = INDUSTRIAL_GRADE,
		// },
	},
	{
		.id = 8828,
		.name = "A20-OLinuXino-MICRO-IND",
		.fdt = "sun7i-a20-olinuxino-micro",
		// .config = {
		// 	.ram = GBYTES(SIZE_1),
		// 	.grade = INDUSTRIAL_GRADE,
		// },
	},
	{
		.id = 4615,
		.name = "A20-OLinuXino-MICRO-n4GB",
		.fdt = "sun7i-a20-olinuxino-micro",
		// .config = {
		// 	.storage = STORAGE_NAND,
		// 	.size = GBYTES(SIZE_4),
		// 	.ram = GBYTES(SIZE_1),
		// },
	},
	{
		.id = 8918,
		.name = "A20-OLinuXino-MICRO-n8GB",
		.fdt = "sun7i-a20-olinuxino-micro",
		// .config = {
		// 	.storage = STORAGE_NAND,
		// 	.size = GBYTES(SIZE_8),
		// 	.ram = GBYTES(SIZE_1),
		// },
	},

	/* A20-SOM204 */
	{
		.id = 8991,
		.name = "A20-SOM204",
		.fdt = "sun7i-a20-olimex-som204-evb",
		// .config = {
		// 	.ram = GBYTES(SIZE_1)
		// },
	},
	{
		.id = 8958,
		.name = "A20-SOM204-1Gs16Me16G-MC",
		.fdt = "sun7i-a20-olimex-som204-evb-emmc",
		.overlays = "atecc508a ir0 spi-flash",
		// .config = {
		// 	.storage = STORAGE_EMMC,
		// 	.size = GBYTES(SIZE_16),
		// 	.ram = GBYTES(SIZE_1)
		// },
	},

	/* END */
	{
		.id = 0
	},
};


const char *olimex_get_board_name(uint32_t id)
{
	struct olinuxino_boards *board = olinuxino_boards;

	while (board->id) {
		if (id == board->id)
			return board->name;
		board++;
	}
	return "";
}

const char *olimex_get_board_fdt(uint32_t id)
{
	struct olinuxino_boards *board = olinuxino_boards;

	while (board->id) {
		if (id == board->id)
			return board->fdt;
		board++;
	}
	return "";
}

#endif

bool olimex_board_is_micro(uint32_t id)
{
	switch (id) {
		case 4614:
		case 8832:
		case 8661:
		case 8828:
		case 4615:
		case 8918:
			return true;

		default:
			return false;
	}
}

bool olimex_board_is_lime(uint32_t id)
{
	switch (id) {
		case 7739:
		case 7743:
		case 8934:
			return true;

		default:
			return false;
	}
}

bool olimex_board_is_lime2(uint32_t id)
{
	switch (id) {
		case 7701:
		case 8340:
		case 7624:
		case 8910:
		case 8946:
			return true;

		default:
			return false;
	}
}

bool olimex_board_is_som204_evb(uint32_t id)
{
	switch (id) {
		case 8991:
		case 8958:
			return true;

		default:
			return false;
	}
}

bool olimex_board_is_som_evb(uint32_t id)
{
	return false;
}
