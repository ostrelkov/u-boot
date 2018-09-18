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

#include "lcd_olinuxino.h"
#include "board_detect.h"

struct lcd_olinuxino_eeprom lcd_olinuxino_eeprom;
char videomode[128];

static int lcd_olinuxino_eeprom_init(void)
{
	int ret;

	if ((ret = i2c_set_bus_num(LCD_OLINUXINO_EEPROM_BUS))) {
		debug("%s(): Failed to set bus!\n", __func__);
		return ret;
	}

	if ((ret = i2c_probe(LCD_OLINUXINO_EEPROM_ADDRESS))) {
		debug("%s(): Failed to probe!\n", __func__);
		return ret;
	}

	return 0;
}

static int lcd_olinuxino_eeprom_read(void)
{
	uint32_t crc;
	int ret;

	if ((ret = lcd_olinuxino_eeprom_init())) {
		printf("Error: Failed to init EEPROM!\n");
		return ret;
	}

	if ((ret = i2c_read(LCD_OLINUXINO_EEPROM_ADDRESS, 0, 1, (uint8_t *)&lcd_olinuxino_eeprom, 256))) {
		printf("Error: Failed to read EEPROM!\n");
		return ret;
	}

	if (lcd_olinuxino_eeprom.header != LCD_OLINUXINO_HEADER_MAGIC) {
		printf("Error: EEPROM magic header is not valid!\n");
		memset(&lcd_olinuxino_eeprom, 0xFF, 256);
		return 1;
	}

	crc = crc32(0L, (uint8_t *)&lcd_olinuxino_eeprom, 252);
	if (lcd_olinuxino_eeprom.checksum != crc) {
		printf("Error: CRC checksum is not valid!\n");
		memset(&lcd_olinuxino_eeprom, 0xFF, 256);
		return 1;
	}

	return 0;
}

char *lcd_olinuxino_video_mode()
{
	struct lcd_olinuxino_mode *mode;
	struct lcd_olinuxino_info *info;
	int ret;

	ret = lcd_olinuxino_eeprom_read();
	if (ret)
		return "";

	mode = (struct lcd_olinuxino_mode *)&lcd_olinuxino_eeprom.reserved;
	info = &lcd_olinuxino_eeprom.info;

	printf("Detected %s, Rev.%s, Serial:%08x\n",
	       lcd_olinuxino_eeprom.info.name,
	       lcd_olinuxino_eeprom.revision,
	       lcd_olinuxino_eeprom.serial);

	sprintf(videomode, "x:%d,y:%d,depth:%d,pclk_khz:%d,le:%d,ri:%d,up:%d,lo:%d,hs:%d,vs:%d,sync:3,vmode:0",
		mode->hactive,
		mode->vactive,
		(info->bus_format == MEDIA_BUS_FMT_RGB888_1X24) ? 24 : 18,
		mode->pixelclock,
		mode->hbp,
		mode->hfp,
		mode->vbp,
		mode->vfp,
		mode->hpw,
		mode->vpw);


	return videomode;
}

bool lcd_olinuxino_is_present()
{
	return (lcd_olinuxino_eeprom.header == LCD_OLINUXINO_HEADER_MAGIC);
}

uint8_t lcd_olinuxino_dclk_phase()
{
	/* For now always return 0 */
	return 0;
}
