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

const char *olimex_get_board_name(uint32_t id);
const char *olimex_get_board_fdt(uint32_t id);

#endif /* __BOARDS_H */
