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
#include <linux/sizes.h>
#include <fdt_support.h>
#include <jffs2/load_kernel.h>
#include <mtd_node.h>

#include "board_detect.h"

#define FDT_PATH_ALIASES	"/aliases"

#define FDT_COMP_PINCTRL	"allwinner,sun7i-a20-pinctrl"

enum devices {
	PATH_I2C2 = 0,
	PATH_NAND,
	PATH_SPI0
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
	FDT_PATH("i2c",		0x01c2b400),
	FDT_PATH("nand",	0x01c03000),
	FDT_PATH("spi",		0x01c05000),
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

	debug("Updating atecc508a@60 node...\n");

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

	debug("Updating spi-nor@0 node...\n");

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

	debug("Updating nand0@0 node...\n");

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
	}

	return 0;
}

static int (*olinuxino_fixes[]) (void *blob) = {
	board_fix_spi_flash,
	board_fix_nand,
	board_fix_atecc508a,
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

#if defined(CONFIG_OF_BOARD_FIXUP)
int board_fix_fdt(void *blob)
{
	return olinuxino_fdt_fixup(blob);
}
#endif

#if defined(CONFIG_OF_SYSTEM_SETUP)
int ft_system_setup(void *blob, bd_t *bd)
{
	int ret;
#ifdef CONFIG_FDT_FIXUP_PARTITIONS
	static struct node_info nodes[] = {
		{ "fixed-partitions", MTD_DEV_TYPE_NOR, },
	};
#endif
	ret = olinuxino_fdt_fixup(blob);
	if (ret < 0)
		return ret;

#ifdef CONFIG_FDT_FIXUP_PARTITIONS
	fdt_fixup_mtdparts(blob, nodes, ARRAY_SIZE(nodes));
#endif
	return ret;
}
#endif
