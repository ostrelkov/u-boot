/*
 * Device Tree fixup for olinuxino
 *
 * Copyright (C) 2018 Olimex Ltd.
 *   Author: Stefan Mavrodiev <stefan@olimex.com>
 *
 * SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 */

#define DEBUG
#include <common.h>
#include <linux/libfdt.h>
#include <fdt_support.h>

#include "board_detect.h"

#define FDT_ALIASES		"/aliases"
#define FDT_NAND_PATH		"/soc@01c00000/nand@1c03000"
#define FDT_EMAC_PATH		"/soc@01c00000/ethernet@01c0b000"
#define FDT_GMAC_PATH		"/soc@01c00000/ethernet@01c50000"
#define FDT_PINCTRL_PATH	"/soc@01c00000/pinctrl@01c20800"
#define FDT_SPI_PATH		"/soc@01c00000/spi@01c05000"

static int board_fix_spi_flash(void *blob)
{
	uint32_t phandle;
	int offset;
	int ret = 0;

	/**
	 * Some boards, have both eMMC and SPI flash:
	 *   - A20-SOM204-1Gs16Me16G-MC (8958)
	 */
	if (eeprom->config.storage != 's' && eeprom->id != 8958)
		return 0;

	debug("Updating \"%s\" node\n", FDT_SPI_PATH);

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
	offset = fdt_path_offset(blob, FDT_PINCTRL_PATH);
	if (offset < 0) {
		printf("Path \"%s\" not found: %s (%d)\n", FDT_PINCTRL_PATH, fdt_strerror(offset), offset);
		return offset;
	}

	offset = fdt_add_subnode(blob, offset, "spi0@1");
	if (offset < 0) {
		printf("Failed to add subnode: %s (%d)\n", fdt_strerror(offset), offset);
		return offset;
	}

	phandle = fdt_get_max_phandle(blob) + 1;

	ret |= fdt_setprop_u32(blob, offset, "phandle", phandle);
	ret |= fdt_setprop_string(blob, offset, "function" , "spi0");

	ret |= fdt_setprop_string(blob, offset, "pins" , "PC0");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC1");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC2");
	ret |= fdt_appendprop_string(blob, offset, "pins", "PC23");
	if (ret < 0) {
		printf("Failed to populate spi0@1 subnode: %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

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
	offset = fdt_path_offset(blob, FDT_SPI_PATH);
	if (offset < 0) {
		printf("Path \"%s\" not found: %s (%d)\n", FDT_SPI_PATH, fdt_strerror(offset), offset);
		return offset;
	}

	/* Change status to okay */
	ret |= fdt_setprop_string(blob, offset, "status" , "okay");
	ret |= fdt_setprop_u32(blob, offset, "spi-max-frequency", 20000000);
	ret |= fdt_setprop_u32(blob, offset, "pinctrl-0", phandle);
	ret |= fdt_setprop_string(blob, offset, "pinctrl-names", "default");
	if (ret < 0) {
		printf("Failed to update properties for spi0@0 node: %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

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
	if (offset < 0) {
		printf("Failed to add subnode: %s (%d)\n", fdt_strerror(offset), offset);
		return offset;
	}

	ret |= fdt_setprop_string(blob, offset, "status" , "okay");
	ret |= fdt_setprop_u32(blob, offset, "spi-max-frequency", 20000000);
	ret |= fdt_setprop_u32(blob, offset, "reg", 0);
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", 1);
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", 1);
	ret |= fdt_setprop_string(blob, offset, "compatible", "winbond,w25q128");
	ret |= fdt_appendprop_string(blob, offset, "compatible", "jedec,spi-nor");
	ret |= fdt_appendprop_string(blob, offset, "compatible", "spi-flash");
	if (ret < 0) {
		printf("Failed to populate spi-nor@0 subnode: %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

	/*
	 * Add alias property
	 *
	 * fdt print /aliases
	 *     spi0 = "/soc@01c00000/spi@01c05000"
	 */
	offset = fdt_path_offset(blob, FDT_ALIASES);
	if (offset < 0) {
		printf("Path \"%s\" not found: %s (%d)\n", FDT_ALIASES, fdt_strerror(offset), offset);
		return offset;
	};

	ret = fdt_setprop_string(blob, offset, "spi0", FDT_SPI_PATH);
	if (ret < 0) {
		printf("Failed to add \"spi0\" to \"/aliases\": %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

	return 0;
}

static int board_fix_nand(void *blob)
{
	uint32_t phandle;
	int offset;
	int ret = 0;
	/* Modify only boards with nand storage */
	// if (eeprom->config.storage != 'n')
	// 	return 0;

	debug("Updating \"%s\" node\n", FDT_NAND_PATH);


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
	offset = fdt_path_offset(blob, FDT_PINCTRL_PATH);
 	if (offset < 0) {
 		printf("Path \"%s\" not found: %s (%d)\n", FDT_PINCTRL_PATH, fdt_strerror(offset), offset);
 		return offset;
 	}

 	offset = fdt_add_subnode(blob, offset, "nand0@0");
 	if (offset < 0) {
 		printf("Failed to add subnode: %s (%d)\n", fdt_strerror(offset), offset);
 		return offset;
 	}

 	phandle = fdt_get_max_phandle(blob) + 1;

 	ret |= fdt_setprop_u32(blob, offset, "phandle", phandle);
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
 	if (ret < 0) {
 		printf("Failed to populate nand0@0 subnode: %s (%d)\n", fdt_strerror(ret), ret);
 		return ret;
 	}

	/**
	 * Find /soc@01c00000/nand@1c03000
	 *
	 * Change following properties:
	 *   - pinctrl-names = "default";
	 *   - pinctrl-0 = <&nand0@0>;
	 *   - #address-cells = <1>;
	 *   - #size-cells = <0>;
	 *   - status = "okay";
	 *
	 * Test:
	 * fdt print /soc@01c00000/nand@1c03000
	 */
	offset = fdt_path_offset(blob, FDT_NAND_PATH);
	if (offset < 0) {
		printf("Path \"%s\" not found: %s (%d)\n", FDT_NAND_PATH, fdt_strerror(offset), offset);
		return offset;
	}

	/* Change status to okay */
	ret |= fdt_setprop_string(blob, offset, "status" , "okay");
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", 0);
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", 1);
	ret |= fdt_setprop_u32(blob, offset, "pinctrl-0", phandle);
	ret |= fdt_setprop_string(blob, offset, "pinctrl-names", "default");
	if (ret < 0) {
		printf("Failed to update properties for nand0@0 node: %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

	/**
	 * Add the following node:
	 * nand@0 {
	 *     reg = <0>;
	 *     allwinner,rb = <0>;
	 *     nand-ecc-mode = "hw";
	 *     nand-on-flash-bbt;
	 * }
	 */
	offset = fdt_add_subnode(blob, offset, "nand@0");
	if (offset < 0) {
		printf("Failed to add subnode: %s (%d)\n", fdt_strerror(offset), offset);
		return offset;
	}

	ret |= fdt_setprop_empty(blob, offset, "nand-on-flash-bbt");
	ret |= fdt_setprop_string(blob, offset, "nand-ecc-mode", "hw");
	ret |= fdt_setprop_u32(blob, offset, "allwinner,rb", 0);
	ret |= fdt_setprop_u32(blob, offset, "reg", 0);
	if (ret < 0) {
		printf("Failed to populate nand@0 subnode: %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

	return 0;
}

#if defined(CONFIG_OF_BOARD_FIXUP)
static int (*uboot_fix[]) (void *blob) = {
	board_fix_spi_flash,
};

int board_fix_fdt(void *blob)
{
	uint8_t i;
	int ret;

	debug("Address of FDT blob: %p\n", (uint32_t *)blob);

	ret = fdt_increase_size(blob, 512);
	if (ret < 0) {
		printf("Failed to increase size: %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

	/* Apply fixes */
	for (i = 0; i < ARRAY_SIZE(uboot_fix); i++) {
		debug("%d: %p\n", i, uboot_fix[i]);
		ret = uboot_fix[i](blob);
		if (ret < 0)
			return ret;
	}

	return ret;
}
#endif

#if defined(CONFIG_OF_SYSTEM_SETUP)
static int (*kernel_fix[]) (void *blob) = {
	board_fix_spi_flash,
	board_fix_nand,
};

int ft_system_setup(void *blob, bd_t *bd)
{
	uint8_t i;
	int ret;

	debug("Address of FDT blob: %p\n", (uint32_t *)blob);

	ret = fdt_increase_size(blob, 512);
	if (ret < 0) {
		printf("Failed to increase size: %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

	/* Apply fixes */
	for (i = 0; i < ARRAY_SIZE(kernel_fix); i++) {
		debug("%d: %p\n", i, kernel_fix[i]);
		ret = kernel_fix[i](blob);
		if (ret < 0)
			return ret;
	}

	return ret;
}
#endif
