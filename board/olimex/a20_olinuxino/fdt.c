/*
 * Device Tree fixup for olinuxino
 *
 * Copyright (C) 2018 Olimex Ltd.
 *   Author: Stefan Mavrodiev <stefan@olimex.com>
 *
 * SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 */

#include <common.h>
#include <linux/libfdt.h>
#include <fdt_support.h>

#include "board_detect.h"

#define FDT_ALIASES		"/aliases"
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
	 *         allwinner,pins = "PC0", "PC1", "PC2", "PC23";
	 *         allwinner,function = "spi0";
	 *         allwinner,drive = <SUN4I_PINCTRL_10_MA>;
	 *         allwinner,pull = <SUN4I_PINCTRL_NO_PULL>;
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

static int olinuxino_fix_fdt(void *blob)
{
	int ret;

	debug("Address of FDT blob: %p\n", (uint32_t *)blob);

	ret = fdt_increase_size(blob, 512);
	if (ret < 0) {
		printf("Failed to increase size: %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

	/* Add spi flash overlay */
	ret = board_fix_spi_flash(blob);
	if (ret < 0)
		return ret;

	return ret;
}

#if defined(CONFIG_OF_BOARD_FIXUP)
int board_fix_fdt(void *blob)
{
	return olinuxino_fix_fdt(blob);
}
#endif

#if defined(CONFIG_OF_SYSTEM_SETUP)
int ft_system_setup(void *blob, bd_t *bd)
{
	return olinuxino_fix_fdt(blob);
}
#endif
