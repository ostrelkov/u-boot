/*
 * Device Tree fixup for A20-OLinuXino
 *
 * Copyright (C) 2018 Olimex Ltd.
 *   Author: Stefan Mavrodiev <stefan@olimex.com>
 *
 * SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 */

#include <common.h>
#include <linux/libfdt.h>
#include <linux/sizes.h>
#include <fdt_support.h>
#include <mtd_node.h>
#include <jffs2/load_kernel.h>
#include <asm/arch/gpio.h>

#include "../common/lcd_olinuxino.h"
#include "../common/board_detect.h"
#include "../common/boards.h"

#define FDT_PATH_ROOT		"/"
#define FDT_PATH_ALIASES	"/aliases"
#define FDT_PATH_VCC5V0		"/vcc5v0"

#define FDT_COMP_PINCTRL	"allwinner,sun7i-a20-pinctrl"

enum devices {
	PATH_I2C2 = 0,
	PATH_NAND,
	PATH_SPI0,
	PATH_PWM,
	PATH_TCON0,
	PATH_RTP,
};

#define FDT_PATH(__name, __addr) \
{ \
	.name = __name, \
	.addr = __addr, \
}

struct __path {
	char name[16];
	uint32_t addr;
} paths[] = {
	FDT_PATH("i2c",			0x01c2b400),
	FDT_PATH("nand",		0x01c03000),
	FDT_PATH("spi",			0x01c05000),
	FDT_PATH("pwm",			0x01c20e00),
	FDT_PATH("lcd-controller",	0x01c0c000),
	FDT_PATH("rtp",			0x01c25000),
};

#define NAND_PART(__label, __start, __lenght) \
	{ \
		.label = __label, \
		.addr = __start, \
		.lenght = __lenght \
	}

struct __nand_partition {
	char label[32];
	uint32_t addr;
	uint32_t lenght;

} nand_partitions[] = {
	NAND_PART("NAND.rootfs",		0x02C00000,	0xFD400000),	/* TODO: Actually check nand size! */
	NAND_PART("NAND.kernel",		0x01C00000,	SZ_16M),
	NAND_PART("NAND.dtb",			0x01800000,	SZ_4M),
	NAND_PART("NAND.u-boot-env.backup",	0x01400000,	SZ_4M),
	NAND_PART("NAND.u-boot-env",		0x01000000,	SZ_4M),
	NAND_PART("NAND.u-boot.backup",		0x00C00000,	SZ_4M),
	NAND_PART("NAND.u-boot",		0x00800000,	SZ_4M),
	NAND_PART("NAND.SPL.backup",		0x00400000,	SZ_4M),
	NAND_PART("NAND.SPL",			0x00000000,	SZ_4M),
};

static int get_path_offset(void *blob, enum devices dev, char *dpath)
{
	char path[64];
	int offset;

	sprintf(path, "/soc@1c00000/%s@%x", paths[dev].name, paths[dev].addr);
	offset = fdt_path_offset(blob, path);
	if (offset >= 0)
		goto success;

	sprintf(path, "/soc@01c00000/%s@%08x", paths[dev].name, paths[dev].addr);
	offset = fdt_path_offset(blob, path);
	if (offset >= 0)
		goto success;

	printf("Path \"%s\" not found: %s (%d)\n", path, fdt_strerror(offset), offset);
	return offset;

success:
	if (path != NULL)
		strcpy(dpath, path);
	return offset;
}

static int board_fix_atecc508a(void *blob)
{
	int offset;
	int ret;

	/**
	 * Enabled on:
	 *   - A20-SOM204-1Gs16Me16G-MC (8958)
	 */
	if (eeprom->id != 8958)
		return 0;

	/**
	 * Add the following node:
	 * &i2c {
	 *     atecc508a@60 {
	 *         compatible = "atmel,atecc508a";
	 *         reg = <0x60>;
	 * };
	 */
	offset = get_path_offset(blob, PATH_I2C2, NULL);
 	if (offset < 0)
 		return offset;

	offset = fdt_add_subnode(blob, offset, "atecc508a@60");
	if (offset < 0)
		return offset;

	ret = fdt_setprop_u32(blob, offset, "reg", 0x60);
	ret |= fdt_setprop_string(blob, offset, "compatible", "atmel,atecc508a");

	return ret;
}

static int board_fix_spi_flash(void *blob)
{
	uint32_t phandle;
	char path[64];
	int ret = 0;
	int offset;

	/**
	 * Some boards, have both eMMC and SPI flash:
	 *   - A20-SOM204-1Gs16Me16G-MC (8958)
	 */
	if (eeprom->config.storage != 's' && eeprom->id != 8958)
		return 0;

	/*
	 * Find /soc@01c00000/pinctrl@01c20800
	 * Add following properties:
	 *     spi0@1 {
	 *         pins = "PC0", "PC1", "PC2", "PC23";
	 *         function = "spi0";
	 *     };
	 *
	 * Test:
	 * fdt print /soc@01c00000/pinctrl@01c20800/spi0@1
	 */

	offset = fdt_node_offset_by_compatible(blob, -1, FDT_COMP_PINCTRL);
	if (offset < 0)
		return offset;

	offset = fdt_add_subnode(blob, offset, "spi0@1");
	if (offset < 0)
		return offset;

	/* Generate phandle */
	phandle = fdt_create_phandle(blob, offset);
	if (!phandle)
		return -1;

	ret |= fdt_setprop_string(blob, offset, "function" , "spi0");
	ret |= fdt_setprop_string(blob, offset, "pins" , "PC0");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC1");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC2");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC23");
	if (ret < 0)
		return ret;

	/**
	 * Find /soc@01c00000/spi@01c05000
	 *
	 * Change following properties:
	 *   - pinctrl-names = "default";
	 *   - pinctrl-0 = <&spi0@1>;
	 *   - spi-max-frequency = <20000000>;
	 *   - status = "okay";
	 *
	 * Test:
	 * fdt print /soc@01c00000/spi@01c05000
	 */
	offset = get_path_offset(blob, PATH_SPI0, path);
 	if (offset < 0)
 		return offset;

	/* Change status to okay */
	ret |= fdt_set_node_status(blob, offset, FDT_STATUS_OKAY, 0);
	ret |= fdt_setprop_u32(blob, offset, "spi-max-frequency", 20000000);
	ret |= fdt_setprop_u32(blob, offset, "pinctrl-0", phandle);
	ret |= fdt_setprop_string(blob, offset, "pinctrl-names", "default");
	if (ret < 0)
		return ret;

	/**
	 * Add the following node:
	 * spi-nor@0 {
	 *     #address-cells = <1>;
	 *     #size-cells = <1>;
	 *     compatible = "winbond,w25q128", "jedec,spi-nor", "spi-flash";
	 *     reg = <0>;
	 *     spi-max-frequency = <20000000>;
	 *     status = "okay";
	 * }
	 */
	offset = fdt_add_subnode(blob, offset, "spi-nor@0");
	if (offset < 0)
		return offset;

	ret |= fdt_set_node_status(blob, offset, FDT_STATUS_OKAY, 0);
	ret |= fdt_setprop_u32(blob, offset, "spi-max-frequency", 20000000);
	ret |= fdt_setprop_u32(blob, offset, "reg", 0);
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", 1);
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", 1);
	ret |= fdt_setprop_string(blob, offset, "compatible", "winbond,w25q128");
	ret |= fdt_appendprop_string(blob, offset, "compatible", "jedec,spi-nor");
	ret |= fdt_appendprop_string(blob, offset, "compatible", "spi-flash");
	if (ret < 0)
		return ret;

	offset = fdt_add_subnode(blob, offset, "partitions");
	if (offset < 0)
		return offset;
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", 1);
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", 1);
	ret |= fdt_setprop_string(blob, offset, "compatible", "fixed-partitions");


	/*
	 * Add alias property
	 *
	 * fdt print /aliases
	 *     spi0 = "/soc@01c00000/spi@01c05000"
	 */
	offset = fdt_path_offset(blob, FDT_PATH_ALIASES);
	if (offset < 0)
		return offset;

	ret = fdt_setprop_string(blob, offset, "spi0", path);
	if (ret < 0)
		return ret;

	return 0;
}

static int board_fix_nand(void *blob)
{
	char partition_name[64];
	int offset, parent;
	uint32_t phandle;
	int ret = 0;
	uint8_t i;

	/* Modify only boards with nand storage */
	if (eeprom->config.storage != 'n')
		return 0;

	/*
	 * Find /soc@01c00000/pinctrl@01c20800
	 * Add following properties:
	 *     nand0@0 {
	 *         pins = "PC0", "PC1", "PC2", PC4, "PC5", PC6, "PC8",
	 *		"PC9", "PC10", "PC11", "PC12", "PC13",
	 *		"PC14", "PC15", "PC16";
	 *         function = "nand0";
	 *     };
	 *
	 * Test:
	 * fdt print /soc@01c00000/pinctrl@01c20800/nand0@0
	 */

	offset = fdt_node_offset_by_compatible(blob, -1, FDT_COMP_PINCTRL);
	if (offset < 0)
		return offset;

 	offset = fdt_add_subnode(blob, offset, "nand0@0");
 	if (offset < 0)
 		return offset;

	phandle = fdt_create_phandle(blob, offset);
	if (!phandle)
		return -1;

 	ret |= fdt_setprop_string(blob, offset, "function" , "nand0");

 	ret |= fdt_setprop_string(blob, offset, "pins" , "PC0");
 	ret |= fdt_appendprop_string(blob, offset, "pins", "PC1");
 	ret |= fdt_appendprop_string(blob, offset, "pins", "PC2");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC4");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC5");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC6");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC8");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC9");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC10");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC11");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC12");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC13");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC14");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC15");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC16");
 	if (ret < 0)
 		return ret;
#
	/**
	 * Find /soc@01c00000/nand@01c03000
	 *
	 * Change following properties:
	 *   - pinctrl-names = "default";
	 *   - pinctrl-0 = <&nand0@0>;
	 *   - #address-cells = <1>;
	 *   - #size-cells = <0>;
	 *   - status = "okay";
	 *
	 * Test:
	 * fdt print /soc@01c00000/nand@01c03000
	 */

	offset = get_path_offset(blob, PATH_NAND, NULL);
 	if (offset < 0)
 		return offset;

	/* Change status to okay */
	ret |= fdt_set_node_status(blob, offset, FDT_STATUS_OKAY, 0);
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", 0);
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", 1);
	ret |= fdt_setprop_u32(blob, offset, "pinctrl-0", phandle);
	ret |= fdt_setprop_string(blob, offset, "pinctrl-names", "default");
	if (ret < 0)
		return ret;

	/**
	 * Add the following node:
	 * nand@0 {
	 *     reg = <0>;
	 *     allwinner,rb = <0>;
	 *     nand-ecc-mode = "hw";
	 *     nand-on-flash-bbt;
	 *     partitions {
	 *         compatible = "fixed-partitions";
	 *         #address-cells = <2>;
	 *         #size-cells = <2>;
	 *
	 *         partition@0 {
	 *             label = "NAND.SPL";
	 *             reg = <0x0 0x0 0x0 0x400000>;
	 *         };
	 *
	 *         partition@400000 {
	 *             label = "SPL.backup";
	 *             reg = <0x0 0x400000 0x0 0x400000>;
	 *         };
	 *
	 *         partition@800000 {
	 *             label = "NAND.u-boot";
	 *             reg = <0x0 0x800000 0x0 0x400000>;
	 *         };
	 *
	 *         partition@c00000 {
	 *             label = "NAND.u-boot.backup";
	 *             reg = <0x0 0xc00000 0x0 0x400000>;
	 *         };
	 *
	 *         partition@1000000 {
	 *             label = "NAND.u-boot-env";
	 *             reg = <0x0 0x1000000 0x0 0x400000>;
	 *         };
	 *
	 *         partition@1400000 {
	 *             label = "NAND.u-boot-env.backup";
	 *             reg = <0x0 0x1400000 0x0 0x400000>;
	 *         };
	 *
	 *         partition@1800000 {
	 *             label = "NAND.dtb";
	 *             reg = <0x0 0x1800000 0x0 0x400000>;
	 *         };
	 *
	 *         partition@1c00000 {
	 *             label = "NAND.kernel";
	 *             reg = <0x0 0x1c00000 0x0 0x1000000>;
	 *         };

	 *         partition@2c00000 {
	 *             label = "NAND.rootfs";
	 *             reg = <0x0 0x2c00000 0x0 0xfd400000>;
	 *         };
	 *    }
	 */
	offset = fdt_add_subnode(blob, offset, "nand@0");
	if (offset < 0)
		return offset;

	ret |= fdt_setprop_empty(blob, offset, "nand-on-flash-bbt");
	ret |= fdt_setprop_string(blob, offset, "nand-ecc-mode", "hw");
	ret |= fdt_setprop_u32(blob, offset, "allwinner,rb", 0);
	ret |= fdt_setprop_u32(blob, offset, "reg", 0);
	if (ret < 0)
		return ret;

	offset = fdt_add_subnode(blob, offset, "partitions");
	if (offset < 0)
		return offset;

	ret |= fdt_setprop_string(blob, offset, "compatible" , "fixed-partitions");
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", 2);
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", 2);
	if (ret < 0)
		return ret;

	parent = offset;

	/* Add partitions */
	for (i = 0; i < ARRAY_SIZE(nand_partitions); i++) {

		sprintf(partition_name, "partition@%x", nand_partitions[i].addr);
		offset = fdt_add_subnode(blob, parent, partition_name);
		if (offset < 0) {
			printf("Failed to add %s: %s (%d)\n", partition_name, fdt_strerror(offset), offset);
			return offset;
		}

		fdt64_t path[2];
		path[0] = cpu_to_fdt64(nand_partitions[i].addr);
		path[1] = cpu_to_fdt64(nand_partitions[i].lenght);

		ret |= fdt_setprop_string(blob, offset, "label" , nand_partitions[i].label);
		ret |= fdt_setprop(blob, offset, "reg", path, sizeof(path));
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int (*olinuxino_fixes[]) (void *blob) = {
	board_fix_spi_flash,
	board_fix_atecc508a,
	board_fix_nand,
};

int olinuxino_fdt_fixup(void *blob)
{
	uint8_t i;
	int ret;

	debug("Address of FDT blob: %p\n", (uint32_t *)blob);

	ret = fdt_increase_size(blob, 65535);
	if (ret < 0)
		return ret;

	/* Apply fixes */
	for (i = 0; i < ARRAY_SIZE(olinuxino_fixes); i++) {
		ret = olinuxino_fixes[i](blob);
		if (ret < 0)
			return ret;
	}

	return ret;
}

#ifdef CONFIG_VIDEO_LCD_PANEL_OLINUXINO
static int board_fix_lcd_olinuxino(void *blob)
{
	uint32_t power_supply_phandle;
	uint32_t backlight_phandle;
	uint32_t pinctrl_phandle;
	uint32_t pins_phandle;
	uint32_t pwm_phandle;
	uint32_t panel_endpoint_phandle;
	uint32_t tcon0_endpoint_phandle;

	fdt32_t gpios[4];
	fdt32_t irq[3];
	fdt32_t levels[11];
	char path[64];
	char pin[5];
	int offset;
	int ret = 0;
	int gpio;
	char *s = env_get("lcd_olinuxino");
	int i;

	debug("Enabling LCD-OLinuXino...\n");


	offset = fdt_path_offset(blob, FDT_PATH_VCC5V0);
 	if (offset < 0)
 		return offset;

	power_supply_phandle = fdt_get_phandle(blob, offset);
	if (power_supply_phandle < 0)
 		return offset;


	/**
	 * &pwm {
	 * 	pinctrl-names = "default";
	 *	pinctrl-0 = <&pwm0_pins_a>;
	 *	status = "okay";
	 * };
	 */

	offset = fdt_node_offset_by_compatible(blob, -1, FDT_COMP_PINCTRL);
 	if (offset < 0)
 		return offset;

	pinctrl_phandle = fdt_get_phandle(blob, offset);
	if (pinctrl_phandle < 0)
 		return offset;

	offset = fdt_subnode_offset(blob, offset, "pwm0@0");
	if (offset < 0)
		return offset;

	pins_phandle = fdt_create_phandle(blob, offset);
	if (!pins_phandle)
		return -1;


	offset = get_path_offset(blob, PATH_PWM, NULL);
  	if (offset < 0)
  		return offset;

	ret |= fdt_set_node_status(blob, offset, FDT_STATUS_OKAY, 0);
	ret |= fdt_setprop_u32(blob, offset, "pinctrl-0", pins_phandle);
	ret |= fdt_setprop_string(blob, offset, "pinctrl-names", "default");
	if (ret < 0)
		return ret;

	pwm_phandle = fdt_create_phandle(blob, offset);
	if (!pwm_phandle)
		return -1;

	/**
	 * backlight: backlight {
	 * 	compatible = "pwm-backlight";
	 * 	power-supply = <&reg_vcc5v0>;
	 * 	pwms = <&pwm 0 50000 1>;
	 * 	brightness-levels = <0 10 20 30 40 50 60 70 80 90 100>;
	 *	default-brightness-level = <10>;
	 * };
	 */

	offset = fdt_path_offset(blob, FDT_PATH_ROOT);
 	if (offset < 0)
 		return offset;

	offset = fdt_add_subnode(blob, offset, "backlight");
	if (offset < 0)
		return offset;

	gpios[0] = cpu_to_fdt32(pwm_phandle);
	gpios[1] = cpu_to_fdt32(0);
	gpios[2] = cpu_to_fdt32(50000);
	gpios[3] = cpu_to_fdt32(1);
	ret = fdt_setprop(blob, offset, "pwms", gpios, sizeof(gpios));

	for (i = 0; i < 11; i++)
		levels[i] = cpu_to_fdt32(i * 10);
	ret |= fdt_setprop(blob, offset, "brightness-levels", levels, sizeof(levels));
	ret |= fdt_setprop_u32(blob, offset, "default-brightness-level", 10);
	ret |= fdt_setprop_u32(blob, offset, "power-supply", power_supply_phandle);
	ret |= fdt_setprop_string(blob, offset, "compatible", "pwm-backlight");
	if (ret < 0)
		return ret;

	backlight_phandle = fdt_create_phandle(blob, offset);
	if (!backlight_phandle)
		return -1;


	/**
	 * lcd0_rgb888_pins: lcd0_rgb888_pins@0 {
	 * 	pins = "PD0", "PD1", "PD2", "PD3", "PD4", "PD5",
	 * 		"PD6", "PD7", "PD8", "PD9", "PD10",
	 * 		"PD11", "PD12", "PD13", "PD14", "PD15",
	 * 		"PD16", "PD17", "PD18", "PD19", "PD20",
	 * 		"PD21", "PD22", "PD23", "PD24", "PD25",
	 * 		"PD26", "PD27";
	 * 	function = "lcd0";
	 * };
	 */

	offset = fdt_node_offset_by_compatible(blob, -1, FDT_COMP_PINCTRL);
 	if (offset < 0)
 		return offset;

	offset = fdt_add_subnode(blob, offset, "lcd0_rgb888_pins@0");
	if (offset < 0)
		return offset;

	pins_phandle = fdt_create_phandle(blob, offset);
	if (!pins_phandle)
		return -1;

 	ret = fdt_setprop_string(blob, offset, "function" , "lcd0");

 	ret |= fdt_setprop_string(blob, offset, "pins" , "PD0");
 	ret |= fdt_appendprop_string(blob, offset, "pins", "PD1");
 	ret |= fdt_appendprop_string(blob, offset, "pins", "PD2");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD4");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD5");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD6");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD8");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD9");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD10");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD11");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD12");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD13");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD14");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD15");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD16");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD17");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD18");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD19");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD20");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD21");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD22");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD23");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD24");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD25");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD26");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PD27");
 	if (ret < 0)
 		return ret;

	/**
	 * panel@50 {
	 * 	compatible = "olimex,lcd-olinuxino";
	 * 	#address-cells = <1>;
	 * 	#size-cells = <0>;
	 * 	reg = <0x50>;
	 *
	 * 	pinctrl-names = "default";
	 * 	pinctrl-0 = <&lcd0_rgb888_pins>;
	 *
	 * 	power-supply = <&reg_vcc5v0>;
	 *
	 *	enable-gpios = <&pio 7 8 GPIO_ACTIVE_HIGH>;
	 * 	backlight = <&backlight>;
	 * 	status = "okay";
	 *
	 * 	port@0 {
	 * 		#address-cells = <1>;
	 * 		#size-cells = <0>;
	 * 		reg = <0>;
	 *
	 * 		panel_in_tcon0: endpoint@0 {
	 * 			#address-cells = <1>;
	 * 			#size-cells = <0>;
	 * 			reg = <0>;
	 * 			remote-endpoint = <&tcon0_out_panel>;
	 * 			};
	 *		};
	 *	};
	 * };
	 */

	if (!s) {
		offset = get_path_offset(blob, PATH_I2C2, path);
	  	if (offset < 0)
	  		return offset;

		offset = fdt_add_subnode(blob, offset, "panel@50");
		if (offset < 0)
			return offset;
	} else {
		path[0] = 0;
		offset = fdt_path_offset(blob, FDT_PATH_ROOT);
		if (offset < 0)
			return offset;

		offset = fdt_add_subnode(blob, offset, "panel@0");
		if (offset < 0)
			return offset;
	}

	ret = fdt_setprop_string(blob, offset, "compatible", lcd_olinuxino_compatible());
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", 1);
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", 0);
	if (!s)
		ret |= fdt_setprop_u32(blob, offset, "reg", 0x50);
	ret |= fdt_setprop_string(blob, offset, "pinctrl-names", "default");
	ret |= fdt_setprop_u32(blob, offset, "pinctrl-0", pins_phandle);
	ret |= fdt_setprop_u32(blob, offset, "power-supply", power_supply_phandle);
	ret |= fdt_setprop_u32(blob, offset, "backlight", backlight_phandle);

	strcpy(pin, olimex_get_lcd_pwr_pin());

	gpios[0] = cpu_to_fdt32(pinctrl_phandle);
	gpios[1] = cpu_to_fdt32(pin[1] - 'A');
	gpios[2] = cpu_to_fdt32(simple_strtoul(&pin[2], NULL, 10));
	gpios[3] = cpu_to_fdt32(0);
	ret |= fdt_setprop(blob, offset, "enable-gpios", gpios, sizeof(gpios));
	ret |= fdt_set_node_status(blob, offset, FDT_STATUS_OKAY, 0);
	if (ret < 0)
 		return ret;



	offset = fdt_add_subnode(blob, offset, "port@0");
	if (offset < 0)
		return offset;

	ret = fdt_setprop_u32(blob, offset, "reg", 0);
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", 0);
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", 1);
	if (ret < 0)
 		return ret;

	offset = fdt_add_subnode(blob, offset, "endpoint@0");
	if (offset < 0)
		return offset;
	ret = fdt_setprop_u32(blob, offset, "reg", 0);
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", 0);
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", 1);
	if (ret < 0)
 		return ret;

	panel_endpoint_phandle = fdt_create_phandle(blob, offset);
	if (!panel_endpoint_phandle)
		return -1;


	/**
	* &tcon0_out {
	* 	#address-cells = <1>;
	* 	#size-cells = <0>;
	*
	* 	tcon0_out_panel: endpoint@0 {
	* 		#address-cells = <1>;
	* 		#size-cells = <0>;
	* 		reg = <0>;
	* 		remote-endpoint = <&panel_in_tcon0>;
	* 		allwinner,tcon-channel = <0>;
	* 	};
	* };
	*/

	offset = get_path_offset(blob, PATH_TCON0, NULL);
  	if (offset < 0)
  		return offset;

	offset = fdt_subnode_offset(blob, offset, "ports");
	if (offset < 0)
		return offset;

	offset = fdt_subnode_offset(blob, offset, "port@1");
	if (offset < 0)
		return offset;

	offset = fdt_add_subnode(blob, offset, "endpoint@0");
	if (offset < 0)
		return offset;

	ret = fdt_setprop_u32(blob, offset, "allwinner,tcon-channel", 0);
	ret |= fdt_setprop_u32(blob, offset, "remote-endpoint", panel_endpoint_phandle);
	ret |= fdt_setprop_u32(blob, offset, "reg", 0);
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", 0);
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", 1);
	if (ret < 0)
 		return ret;

	tcon0_endpoint_phandle  = fdt_create_phandle(blob, offset);
	if (!tcon0_endpoint_phandle)
		return -1;

	if (!s)
		strcat(path, "/panel@50/port@0/endpoint@0");
	else
		strcat(path, "/panel@0/port@0/endpoint@0");

	offset = fdt_path_offset(blob, path);
	if (offset < 0)
		return offset;

	ret = fdt_setprop_u32(blob, offset, "remote-endpoint", tcon0_endpoint_phandle);
	if (ret < 0)
 		return ret;


	/* Enable TS */
	if (lcd_olinuxino_eeprom.id == 9278 ||	/* LCD-OLinuXino-7CTS */
	    lcd_olinuxino_eeprom.id == 9284 ||	/* LCD-OLinuXino-10CTS */
	    (s && !strcmp(s, "LCD-OLinuXino-5"))) {

		offset = get_path_offset(blob, PATH_I2C2, path);
		if (offset < 0)
			return offset;

		offset = fdt_add_subnode(blob, offset, "gt911@14");
		if (offset < 0)
			return offset;

		if (s && !strcmp(s, "LCD-OLinuXino-5")) {
			ret = fdt_setprop_string(blob, offset, "compatible", "edt,edt-ft5306");
			ret |= fdt_setprop_u32(blob, offset, "reg", 0x38);
			ret |= fdt_setprop_u32(blob, offset, "touchscreen-size-x", 800);
			ret |= fdt_setprop_u32(blob, offset, "touchscreen-size-y", 480);
		} else {
			ret = fdt_setprop_string(blob, offset, "compatible", "goodix,gt911");
			ret |= fdt_setprop_u32(blob, offset, "reg", 0x14);
		}
		ret |= fdt_setprop_u32(blob, offset, "interrupt-parent", pinctrl_phandle);

		gpio = sunxi_name_to_gpio(olimex_get_lcd_irq_pin());
		irq[0] = cpu_to_fdt32(gpio >> 5);
		irq[1] = cpu_to_fdt32(gpio & 0x1F);
		irq[2] = cpu_to_fdt32(2);
		ret |= fdt_setprop(blob, offset, "interrupts", irq, sizeof(irq));

		gpios[0] = cpu_to_fdt32(pinctrl_phandle);
		gpios[1] = cpu_to_fdt32(gpio >> 5);
		gpios[2] = cpu_to_fdt32(gpio & 0x1F);
		gpios[3] = cpu_to_fdt32(0);
		ret |= fdt_setprop(blob, offset, "irq-gpios", gpios, sizeof(gpios));

		gpio = sunxi_name_to_gpio(olimex_get_lcd_rst_pin());
		gpios[0] = cpu_to_fdt32(pinctrl_phandle);
		gpios[1] = cpu_to_fdt32(gpio >> 5);
		gpios[2] = cpu_to_fdt32(gpio & 0x1F);
		if (s && !strcmp(s, "LCD-OLinuXino-5"))
			gpios[3] = cpu_to_fdt32(1);
		else
			gpios[3] = cpu_to_fdt32(0);
		ret |= fdt_setprop(blob, offset, "reset-gpios", gpios, sizeof(gpios));

		if (lcd_olinuxino_eeprom.id == 9278)
			ret |= fdt_setprop_empty(blob, offset, "touchscreen-swapped-x-y");

	} else {
		/* Enable SUN4I-TS */
		offset = get_path_offset(blob, PATH_RTP, NULL);
		if (offset < 0)
			return offset;

		ret = fdt_setprop_empty(blob, offset, "allwinner,ts-attached");
	}

	return ret;
}
#endif

#if defined(CONFIG_OF_BOARD_FIXUP)
int board_fix_fdt(void *blob)
{
	return olinuxino_fdt_fixup(blob);
}
#endif

#if defined(CONFIG_OF_SYSTEM_SETUP)
int ft_system_setup(void *blob, bd_t *bd)
{
	int ret = 0;

#ifdef CONFIG_FDT_FIXUP_PARTITIONS
	static struct node_info nodes[] = {
		{ "fixed-partitions", MTD_DEV_TYPE_NOR, },
	};
#endif
	ret = olinuxino_fdt_fixup(blob);
	if (ret < 0)
		return ret;

#ifdef CONFIG_VIDEO_LCD_PANEL_OLINUXINO
	/* Check if lcd is the default monitor */
	char *s = env_get("monitor");
	if (s != NULL && !strncmp(s, "lcd", 3)) {
		ret = board_fix_lcd_olinuxino(blob);
		if (ret < 0)
			return ret;
		}
#endif

#ifdef CONFIG_FDT_FIXUP_PARTITIONS
	if (eeprom->config.storage == 'n')
		fdt_fixup_mtdparts(blob, nodes, ARRAY_SIZE(nodes));

#endif
	return ret;
}
#endif
