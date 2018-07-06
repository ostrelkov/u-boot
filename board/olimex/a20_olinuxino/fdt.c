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
#define FDT_EMAC_PATH		"/soc@01c00000/ethernet@01c0b000"
#define FDT_GMAC_PATH		"/soc@01c00000/ethernet@01c50000"
#define FDT_PINCTRL_PATH	"/soc@01c00000/pinctrl@01c20800"
#define FDT_SPI_PATH		"/soc@01c00000/spi@01c05000"

#if 0
static int board_fix_emac(void *blob)
{
	int offset;
	int ret;
	/*
	 * EMAC should be enabled only for A20-SOM204-1Gs16Me16G-MC board.
	 * Otherwise skip.
	 */
	if (olimex_get_eeprom_id() != 8958)
		return 0;

	/*
	 * Find /soc@01c00000/ethernet@1c0b000
	 */
	offset = fdt_path_offset(blob, FDT_EMAC_PATH);
	if (offset < 0) {
		printf("Path \"%s\" not found: %s (%d)\n", FDT_EMAC_PATH, fdt_strerror(offset), offset);
		return offset;
	};

	ret = fdt_setprop_string(blob, offset, "status" , "okay");
	if (ret < 0) {
		printf("Failed to change \"status\" to \"okay\": %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

	/*
	 * Add alias property
	 */
	offset = fdt_path_offset(blob, FDT_ALIASES);
	if (offset < 0) {
		printf("Path \"%s\" not found: %s (%d)\n", FDT_ALIASES, fdt_strerror(offset), offset);
		return offset;
	};

	ret = fdt_setprop_string(blob, offset, "ethernet2", FDT_EMAC_PATH);
	if (ret < 0) {
		printf("Failed to add \"ethernet2\" to \"/aliases\": %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}

	return 0;
}

static int board_fix_gmac(void *blob)
{
	uint32_t data[4];
	uint32_t phandle;
	int offset;
	int ret;

	/*
	 * Find /soc@01c00000/ethernet@01c50000
	 */
	offset = fdt_path_offset(blob, FDT_GMAC_PATH);
	if (offset < 0) {
		printf("Path \"%s\" not found: %s (%d)\n", FDT_GMAC_PATH, fdt_strerror(offset), offset);
		return offset;
	};

	/*
	 * A20-OLinuXino-MICRO and A20-OLinXino-Lime should run
	 * the gmac in mii mode.
	 */
	if (olimex_board_is_lime() || olimex_board_is_micro()) {
		/*
		 * Find /soc@01c00000/ethernet@01c50000
		 */
		offset = fdt_path_offset(blob, FDT_GMAC_PATH);
		if (offset < 0) {
			printf("Path \"%s\" not found: %s (%d)\n", FDT_GMAC_PATH, fdt_strerror(offset), offset);
			return offset;
		};

		ret = fdt_setprop_string(blob, offset, "phy-mode", "mii");
		if (ret < 0) {
			printf("Failed to set \"phy-mode\" to \"mii\": %s (%d)\n", fdt_strerror(ret), ret);
			return ret;
		}
	}

	/*
	 * Further fixups are only for som204
	 */
	if (!olimex_board_is_som204())
		return 0;

	/*
	 * Find /soc@01c00000/pinctrl@01c20800
	 */
	offset = fdt_path_offset(blob, FDT_PINCTRL_PATH);
	if (offset < 0) {
		printf("Path \"%s\" not found: %s (%d)\n", FDT_PINCTRL_PATH, fdt_strerror(offset), offset);
		return offset;
	}

	phandle = fdt_get_phandle(blob, offset);
	if (!phandle) {
		printf("Node \"%s\" has no phandle!\n", FDT_PINCTRL_PATH);
		return FDT_ERR_NOTFOUND;
	};

	/*
	 * Find /soc@01c00000/ethernet@01c50000
	 *
	 * A20-SOM204 additional property to gmac node:
	 *		snps,reset-gpio = <&pio 0 17 GPIO_ACTIVE_LOW>;
	 *		snps,reset-delays-us = <0 10000 1000000>;
	 */

	offset = fdt_path_offset(blob, FDT_GMAC_PATH);
 	if (offset < 0) {
 		printf("Path \"%s\" not found: %s (%d)\n", FDT_GMAC_PATH, fdt_strerror(offset), offset);
 		return offset;
 	};

	data[0] = cpu_to_be32(0);
	data[1] = cpu_to_be32(10000);
	data[2] = cpu_to_be32(1000000);
	ret = fdt_setprop(blob, offset, "snps,reset-delays-us", data, sizeof(uint32_t) * 3);
	if (ret < 0) {
		printf("Failed to set \"%s\" property: %s (%d)\n", "snps,reset-delays-us", fdt_strerror(ret), ret);
		return ret;
	}

	data[0] = cpu_to_be32(phandle);
	data[1] = cpu_to_be32(0);
	data[2] = cpu_to_be32(17);
	data[3] = cpu_to_be32(1);
	ret = fdt_setprop(blob, offset, "snps,reset-gpio", data, sizeof(uint32_t) * 4);
	if (ret < 0) {
		printf("Failed to set \"%s\" property: %s (%d)\n", "snps,reset-gpio", fdt_strerror(ret), ret);
		return ret;
	}

	return 0;
}

#endif
static int board_fix_spi_flash(void *blob)
{
	uint32_t phandle;
	int offset;
	int ret = 0;

	/**
	 * Some boards, have both eMMC and SPI flash:
	 *   - A20-SOM204-1Gs16Me16G-MC (8958)
	 */
	// if (eeprom->config.storage != 's' && eeprom->id != 8958)
	// 	return 0;


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

	phandle = fdt32_to_cpu(fdt_get_max_phandle(blob) + 1);

	ret |= fdt_setprop_u32(blob, offset, "phandle", cpu_to_fdt32(phandle));
	ret |= fdt_setprop_u32(blob, offset, "allwinner,pull", cpu_to_fdt32(0));
	ret |= fdt_setprop_u32(blob, offset, "allwinner,drive", cpu_to_fdt32(0));
	ret |= fdt_setprop_string(blob, offset, "allwinner,function" , "spi0");

	ret |= fdt_setprop_string(blob, offset, "allwinner,pins" , "PC0");
	ret |= fdt_appendprop_string(blob, offset, "allwinner,pins", "PC1");
	ret |= fdt_appendprop_string(blob, offset, "allwinner,pins", "PC2");
	ret |= fdt_appendprop_string(blob, offset, "allwinner,pins", "PC23");
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
	ret |= fdt_setprop_u32(blob, offset, "pinctrl-0", cpu_to_fdt32(phandle));
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
	ret |= fdt_setprop_u32(blob, offset, "spi-max-frequency", cpu_to_fdt32(20000000));
	ret |= fdt_setprop_u32(blob, offset, "#reg", cpu_to_fdt32(0));
	ret |= fdt_setprop_u32(blob, offset, "#size-cells", cpu_to_fdt32(1));
	ret |= fdt_setprop_u32(blob, offset, "#address-cells", cpu_to_fdt32(1));
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

int board_fix_fdt(void *blob)
{
	int ret;

	debug("Address of FDT blob: %p\n", (uint32_t *)blob);

	ret = fdt_increase_size(blob, 512);
	if (ret < 0) {
		printf("Failed to increase size: %s (%d)\n", fdt_strerror(ret), ret);
		return ret;
	}


#if 0
	if ((ret = board_fix_gmac(blob)) < 0)
		return ret;

	if ((ret = board_fix_emac(blob)) < 0)
		return ret;
#endif
	if ((ret = board_fix_spi_flash(blob)) < 0)
		return ret;




	return 0;
}
