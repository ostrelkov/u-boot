/*
 * List of all supported devices
 *
 * Copyright (C) 2018 Olimex Ltd.
 *   Author: Stefan Mavrodiev <stefan@olimex.com>
 *
 * SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 */

#ifndef __BOARDS_H
#define __BOARDS_H

enum olinuxino_storage {
	STORAGE_NONE = 0,
	STORAGE_EMMC = 'e',
	STORAGE_NAND = 'n',
	STORAGE_SPI = 's'
};

enum olinuxino_grade {
	GRADE_COM = 0,
	GRADE_IND = 1,
};

enum olinuxino_size {
	S_1 = 0, S_2, S_4, S_8, S_16, S_32, S_64, S_128, S_256, S_512
};

#define BYTES(a)	( S_##a )
#define KBYTES(a)	( S_##a + 10)
#define MBYTES(a)	( S_##a + 20)
#define GBYTES(a)	( S_##a + 30)

#define OLINUXINO_CONFIG(__storage, __size, __ram, __grade) \
	.config = { STORAGE_##__storage, __size, __ram, GRADE_##__grade },

#define OLINUXINO_BOARD(__id, __name, __fdt) \
	.id = __id, \
	.name = __name, \
	.fdt = __fdt,

#ifndef CONFIG_SPL_BUILD
struct olinuxino_boards {
	uint32_t id;
	const char name[32];
	const char fdt[40];
	uint8_t config[4];
};

extern struct olinuxino_boards olinuxino_boards[];
#endif

const char *olimex_get_board_name(uint32_t id);
const char *olimex_get_board_fdt(uint32_t id);


bool olimex_board_is_lime(uint32_t id);
bool olimex_board_is_lime2(uint32_t id);
bool olimex_board_is_micro(uint32_t id);
bool olimex_board_is_som_evb(uint32_t id);
bool olimex_board_is_som204_evb(uint32_t id);

#endif /* __BOARDS_H */
