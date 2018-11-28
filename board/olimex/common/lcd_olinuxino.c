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

struct lcd_olinuxino_board lcd_olinuxino_boards[] = {
	{
		{
			.name = "LCD-OLinuXino-4.3TS",
			.bus_format = MEDIA_BUS_FMT_RGB888_1X24,
		},
		{
			.pixelclock = 12000,
			.hactive = 480,
			.hfp = 8,
			.hbp = 23,
			.hpw = 20,
			.vactive = 272,
			.vfp = 4,
			.vbp = 13,
			.vpw = 10,
			.refresh = 60,
			.flags = 0
		}

	},
	{
		{
			.name = "LCD-OLinuXino-5",
			.bus_format = MEDIA_BUS_FMT_RGB888_1X24,
		},
		{
			.pixelclock = 33000,
			.hactive = 800,
			.hfp = 210,
			.hbp = 26,
			.hpw = 20,
			.vactive = 480,
			.vfp = 22,
			.vbp = 13,
			.vpw = 10,
			.refresh = 60,
			.flags = 0
		}

	},
	{
		{
			.name = "LCD-OLinuXino-7",
			.bus_format = MEDIA_BUS_FMT_RGB888_1X24,
		},
		{
			.pixelclock = 33000,
			.hactive = 800,
			.hfp = 210,
			.hbp = 26,
			.hpw = 20,
			.vactive = 480,
			.vfp = 22,
			.vbp = 13,
			.vpw = 10,
			.refresh = 60,
			.flags = 0
		}

	},
	{
		{
			.name = "LCD-OLinuXino-7CTS",
			.bus_format = MEDIA_BUS_FMT_RGB888_1X24,
		},
		{
			.pixelclock = 51000,
			.hactive = 1024,
			.hfp = 154,
			.hbp = 150,
			.hpw = 10,
			.vactive = 600,
			.vfp = 12,
			.vbp = 21,
			.vpw = 2,
			.refresh = 60,
			.flags = 0
		}

	},
	{
		{
			.name = "LCD-OLinuXino-10",
			.bus_format = MEDIA_BUS_FMT_RGB888_1X24,
		},
		{
			.pixelclock = 51000,
			.hactive = 1024,
			.hfp = 154,
			.hbp = 150,
			.hpw = 10,
			.vactive = 600,
			.vfp = 12,
			.vbp = 21,
			.vpw = 2,
			.refresh = 60,
			.flags = 0
		}

	},
	{
		{
			.name = "LCD-OLinuXino-15.6",
		},
		{
			.pixelclock = 70000,
			.hactive = 1366,
			.hfp = 20,
			.hbp = 54,
			.hpw = 0,
			.vactive = 768,
			.vfp = 17,
			.vbp = 23,
			.vpw = 0,
			.refresh = 60,
			.flags = 0
		}

	},
	{
		{
			.name = "LCD-OLinuXino-15.6FHD",
		},
		{
			.pixelclock = 152000,
			.hactive = 1920,
			.hfp = 150,
			.hbp = 246,
			.hpw = 60,
			.vactive = 1080,
			.vfp = 15,
			.vbp = 53,
			.vpw = 9,
			.refresh = 60,
			.flags = 0
		}

	},
	{
		{
			.name = "",
		},
	},
};

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
		debug("Error: Failed to init EEPROM!\n");
		return ret;
	}

	if ((ret = i2c_read(LCD_OLINUXINO_EEPROM_ADDRESS, 0, 1, (uint8_t *)&lcd_olinuxino_eeprom, 256))) {
		debug("Error: Failed to read EEPROM!\n");
		return ret;
	}

	if (lcd_olinuxino_eeprom.header != LCD_OLINUXINO_HEADER_MAGIC) {
		debug("Error: EEPROM magic header is not valid!\n");
		memset(&lcd_olinuxino_eeprom, 0xFF, 256);
		return 1;
	}

	crc = crc32(0L, (uint8_t *)&lcd_olinuxino_eeprom, 252);
	if (lcd_olinuxino_eeprom.checksum != crc) {
		debug("Error: CRC checksum is not valid!\n");
		memset(&lcd_olinuxino_eeprom, 0xFF, 256);
		return 1;
	}

	return 0;
}

char * lcd_olinuxino_video_mode()
{
	struct lcd_olinuxino_board *lcd = lcd_olinuxino_boards;
	struct lcd_olinuxino_mode *mode = NULL;
	struct lcd_olinuxino_info *info = NULL;
	char *s = env_get("lcd_olinuxino");
	int ret;

	while (s && strlen(lcd->info.name)) {
		if (!strncmp(lcd->info.name, s, strlen(s))) {
			info = &lcd->info;
			mode = &lcd->mode;
			break;
		}
		lcd++;
	}

	if (mode == NULL || info == NULL) {
		ret = lcd_olinuxino_eeprom_read();
		if (ret)
			return "";

		printf("Detected %s, Rev.%s, Serial:%08x\n",
		       lcd_olinuxino_eeprom.info.name,
		       lcd_olinuxino_eeprom.revision,
		       lcd_olinuxino_eeprom.serial);

		mode = (struct lcd_olinuxino_mode *)&lcd_olinuxino_eeprom.reserved;
		info = &lcd_olinuxino_eeprom.info;
	}

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
	char *s = env_get("lcd_olinuxino");

	if (!s)
		return (lcd_olinuxino_eeprom.header == LCD_OLINUXINO_HEADER_MAGIC);
	else
		return true;
}

char * lcd_olinuxino_compatible()
{
	char *s = env_get("lcd_olinuxino");

	if (!s)
		return "olimex,lcd-olinuxino";

	if (!strncmp(s, "LCD-OLinuXino-4.3TS", strlen(s)))
		return "olimex,lcd-olinuxino-4.3";
	else if (!strncmp(s, "LCD-OLinuXino-5", strlen(s)))
		return "olimex,lcd-olinuxino-5";
	else if (!strncmp(s, "LCD-OLinuXino-7", strlen(s)))
		return "olimex,lcd-olinuxino-7";
	else if (!strncmp(s, "LCD-OLinuXino-7CTS", strlen(s)))
		/* LCD-OlinuXino-7CTS uses 10 inch resolution */
		return "olimex,lcd-olinuxino-10";
	else if (!strncmp(s, "LCD-OLinuXino-10", strlen(s)))
		return "olimex,lcd-olinuxino-10";

	return "olimex,lcd-olinuxino";
}

uint8_t lcd_olinuxino_dclk_phase()
{
	return 0;
}

uint8_t lcd_olinuxino_interface()
{
	char *s = env_get("lcd_olinuxino");

	/* Is not set assume LCD-DRIVER */
	if (!s)
		return LCD_OLINUXINO_IF_PARALLEL;

	/* Check LVDS or PARALLEL */
	if (!strncmp(s, "LCD-OLinuXino-15.6", strlen(s)) ||
	    !strncmp(s, "LCD-OLinuXino-15.6FHD", strlen(s)))
		return LCD_OLINUXINO_IF_LVDS;

	return LCD_OLINUXINO_IF_PARALLEL;
}

struct lcd_olinuxino_board * lcd_olinuxino_get_data()
{
	struct lcd_olinuxino_board *lcd = lcd_olinuxino_boards;
	char *s = env_get("lcd_olinuxino");

	if (!s)
		return NULL;

	while (strlen(lcd->info.name)) {
		if (!strncmp(lcd->info.name, s, strlen(s)))
			return lcd;
		lcd++;
	}

	return NULL;
}
