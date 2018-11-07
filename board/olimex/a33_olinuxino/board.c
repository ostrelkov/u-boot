/*
 * OLinuXino Board initialization
 *
 * Copyright (C) 2018 Olimex Ltd.
 *   Author: Stefan Mavrodiev <stefan@olimex.com>
 *
 * SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 */
#include <common.h>
#include <dm.h>
#include <environment.h>
#include <axp_pmic.h>
#include <generic-phy.h>
#include <phy-sun4i-usb.h>
#include <nand.h>
#include <crc.h>
#include <mmc.h>
#include <spl.h>
#include <cli.h>
#include <asm/arch/display.h>
#include <asm/arch/clock.h>
#include <asm/arch/dram.h>
#include <asm/arch/gpio.h>
#include <asm/arch/cpu.h>
#include <asm/arch/mmc.h>
#include <asm/arch/spl.h>
#include <asm/armv7.h>
#include <asm/setup.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/ctype.h>

#include <asm/arch/pmic_bus.h>

#include "../common/boards.h"

DECLARE_GLOBAL_DATA_PTR;

void i2c_init_board(void)
{
#ifdef CONFIG_I2C0_ENABLE
	sunxi_gpio_set_cfgpin(SUNXI_GPH(2), SUN8I_GPH_TWI0);
	sunxi_gpio_set_cfgpin(SUNXI_GPH(3), SUN8I_GPH_TWI0);
	clock_twi_onoff(0, 1);
#endif
}

#if defined(CONFIG_ENV_IS_IN_FAT) || defined(CONFIG_ENV_IS_IN_EXT4)
enum env_location env_get_location(enum env_operation op, int prio)
{
	uint32_t boot = sunxi_get_boot_device();

	switch (boot) {
		case BOOT_DEVICE_MMC1:
		case BOOT_DEVICE_BOARD:
			if (prio == 0)
				return ENVL_EXT4;
			else if (prio == 1)
				return ENVL_FAT;
			else
				return ENVL_UNKNOWN;
		default:
			return ENVL_UNKNOWN;
	}
}
#endif

#if defined(CONFIG_ENV_IS_IN_FAT)
char *get_fat_device_and_part(void)
{
	if (sunxi_get_boot_device() == BOOT_DEVICE_MMC1)
		return "0:auto";

	return CONFIG_ENV_FAT_DEVICE_AND_PART;
}
#endif

#if defined(CONFIG_ENV_IS_IN_EXT4)
char *get_ext4_device_and_part(void)
{
	if (sunxi_get_boot_device() == BOOT_DEVICE_MMC1)
		return "0:auto";

	return CONFIG_ENV_EXT4_DEVICE_AND_PART;
}
#endif

/* add board specific code here */
int board_init(void)
{
	__maybe_unused int id_pfr1, ret, satapwr_pin, macpwr_pin, btpwr_pin;

	gd->bd->bi_boot_params = (PHYS_SDRAM_0 + 0x100);

	asm volatile("mrc p15, 0, %0, c0, c1, 1" : "=r"(id_pfr1));
	debug("id_pfr1: 0x%08x\n", id_pfr1);
	/* Generic Timer Extension available? */
	if ((id_pfr1 >> CPUID_ARM_GENTIMER_SHIFT) & 0xf) {
		uint32_t freq;

		debug("Setting CNTFRQ\n");

		/*
		 * CNTFRQ is a secure register, so we will crash if we try to
		 * write this from the non-secure world (read is OK, though).
		 * In case some bootcode has already set the correct value,
		 * we avoid the risk of writing to it.
		 */
		asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(freq));
		if (freq != COUNTER_FREQUENCY) {
			debug("arch timer frequency is %d Hz, should be %d, fixing ...\n",
			      freq, COUNTER_FREQUENCY);
#ifdef CONFIG_NON_SECURE
			printf("arch timer frequency is wrong, but cannot adjust it\n");
#else
			asm volatile("mcr p15, 0, %0, c14, c0, 0"
				     : : "r"(COUNTER_FREQUENCY));
#endif
		}
	}

	axp_gpio_init();

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((long *)PHYS_SDRAM_0, PHYS_SDRAM_0_SIZE);

	return 0;
}

#ifdef CONFIG_NAND_SUNXI
static void nand_pinmux_setup(void)
{
	unsigned int pin;

	for (pin = SUNXI_GPC(0); pin <= SUNXI_GPC(2); pin++)
		sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_NAND);
	for (pin = SUNXI_GPC(4); pin <= SUNXI_GPC(6); pin++)
		sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_NAND);
	for (pin = SUNXI_GPC(4); pin <= SUNXI_GPC(6); pin++)
		sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_NAND);
	for (pin = SUNXI_GPC(8); pin <= SUNXI_GPC(15); pin++)
		sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_NAND);
}

static void nand_clock_setup(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	setbits_le32(&ccm->ahb_gate0, (CLK_GATE_OPEN << AHB_GATE_OFFSET_NAND0));
	setbits_le32(&ccm->ahb_reset0_cfg, (1 << AHB_GATE_OFFSET_NAND0));
}

void board_nand_init(void)
{
	nand_pinmux_setup();
	nand_clock_setup();
#ifndef CONFIG_SPL_BUILD
	sunxi_nand_init();
#endif
}
#endif /* CONFIG_NAND_SUNXI */

#ifdef CONFIG_MMC
static void mmc_pinmux_setup(int sdc)
{
	unsigned int pin;

	switch (sdc) {
	case 0:
		/* SDC0: PF0-PF5 */
		for (pin = SUNXI_GPF(0); pin <= SUNXI_GPF(5); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPF_SDC0);
			sunxi_gpio_set_drv(pin, 2);
		}
		break;
	default:
		break;
	}
}

int board_mmc_init(bd_t *bis)
{
	struct mmc *mmc;

	/* Try to initialize MMC0 */
	mmc_pinmux_setup(0);
	mmc = sunxi_mmc_init(0);
	if (!mmc) {
		printf("Failed to init MMC0!\n");
		return -1;
	}

	return 0;
}
#ifndef CONFIG_SPL_BUILD
int mmc_get_env_dev(void)
{
	unsigned long bootdev = 0;
	char *bootdev_string;

	bootdev_string = env_get("mmc_bootdev");

	if (bootdev_string) {
		bootdev = simple_strtoul(bootdev_string, NULL, 10);
	}
	return bootdev;
}
#endif /* !CONFIG_SPL_BUILD */

#endif /* CONFIG_MMC */

#ifdef CONFIG_BOARD_EARLY_INIT_R
int board_early_init_r(void)
{
#ifdef CONFIG_MMC
	mmc_pinmux_setup(0);
#endif
	return 0;
}
#endif /* CONFIG_BOARD_EARLY_INIT_R */

void sunxi_board_init(void)
{
	int power_failed = 0;

	power_failed = axp_init();
	if (power_failed)
		printf("axp_init failed!\n");

	power_failed |= axp_set_dcdc1(CONFIG_AXP_DCDC1_VOLT);
	power_failed |= axp_set_dcdc2(CONFIG_AXP_DCDC2_VOLT);
	power_failed |= axp_set_dcdc3(CONFIG_AXP_DCDC3_VOLT);
	power_failed |= axp_set_dcdc4(CONFIG_AXP_DCDC4_VOLT);
	power_failed |= axp_set_dcdc5(CONFIG_AXP_DCDC5_VOLT);

	power_failed |= axp_set_aldo1(CONFIG_AXP_ALDO1_VOLT);
	power_failed |= axp_set_aldo2(CONFIG_AXP_ALDO2_VOLT);
	power_failed |= axp_set_aldo3(CONFIG_AXP_ALDO3_VOLT);

	power_failed |= axp_set_dldo(1, CONFIG_AXP_DLDO1_VOLT);
	power_failed |= axp_set_dldo(2, CONFIG_AXP_DLDO2_VOLT);
	power_failed |= axp_set_dldo(3, CONFIG_AXP_DLDO3_VOLT);
	power_failed |= axp_set_dldo(4, CONFIG_AXP_DLDO4_VOLT);

	power_failed |= axp_set_eldo(1, CONFIG_AXP_ELDO1_VOLT);
	power_failed |= axp_set_eldo(2, CONFIG_AXP_ELDO2_VOLT);
	power_failed |= axp_set_eldo(3, CONFIG_AXP_ELDO3_VOLT);


	/**
	 * AXP221 CHGLED is controlled by REG32H by default.
	 * Change behavoir here */
	pmic_bus_setbits(AXP221_SHUTDOWN, 1 << 3);

	printf("DRAM:");
	gd->ram_size = sunxi_dram_init();
	printf(" %d MiB\n", (int)(gd->ram_size >> 20));
	if (!gd->ram_size)
		hang();

	/*
	 * Only clock up the CPU to full speed if we are reasonably
	 * assured it's being powered with suitable core voltage
	 */
	if (!power_failed)
		clock_set_pll1(CONFIG_SYS_CLK_FREQ);
	else
		printf("Failed to set core voltage! Can't set CPU frequency\n");

}

#ifndef CONFIG_SPL_BUILD

#ifdef CONFIG_USB_GADGET
int g_dnl_board_usb_cable_connected(void)
{
	struct udevice *dev;
	struct phy phy;
	int ret;

	ret = uclass_get_device(UCLASS_USB_DEV_GENERIC, 0, &dev);
	if (ret) {
		pr_err("%s: Cannot find USB device\n", __func__);
		return ret;
	}

	ret = generic_phy_get_by_name(dev, "usb", &phy);
	if (ret) {
		pr_err("failed to get %s USB PHY\n", dev->name);
		return ret;
	}

	ret = generic_phy_init(&phy);
	if (ret) {
		pr_err("failed to init %s USB PHY\n", dev->name);
		return ret;
	}

	return sun4i_usb_phy_vbus_detect(&phy);
}
#endif /* CONFIG_USB_GADGET */

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	char *serial_string;
	unsigned long long serial;

	serial_string = env_get("serial#");

	if (serial_string) {
		serial = simple_strtoull(serial_string, NULL, 16);

		serialnr->high = (unsigned int) (serial >> 32);
		serialnr->low = (unsigned int) (serial & 0xffffffff);
	} else {
		serialnr->high = 0;
		serialnr->low = 0;
	}
}
#endif /* CONFIG_SERIAL_TAG */

/*
 * Check the SPL header for the "sunxi" variant. If found: parse values
 * that might have been passed by the loader ("fel" utility), and update
 * the environment accordingly.
 */
static void parse_spl_header(const uint32_t spl_addr)
{
	struct boot_file_head *spl = (void *)(ulong)spl_addr;
	if (memcmp(spl->spl_signature, SPL_SIGNATURE, 3) != 0)
		return; /* signature mismatch, no usable header */

	uint8_t spl_header_version = spl->spl_signature[3];
	if (spl_header_version != SPL_HEADER_VERSION) {
		printf("sunxi SPL version mismatch: expected %u, got %u\n",
		       SPL_HEADER_VERSION, spl_header_version);
		return;
	}
	if (!spl->fel_script_address)
		return;

	if (spl->fel_uEnv_length != 0) {
		/*
		 * data is expected in uEnv.txt compatible format, so "env
		 * import -t" the string(s) at fel_script_address right away.
		 */
		himport_r(&env_htab, (char *)(uintptr_t)spl->fel_script_address,
			  spl->fel_uEnv_length, '\n', H_NOCLEAR, 0, 0, NULL);
		return;
	}
	/* otherwise assume .scr format (mkimage-type script) */
	env_set_hex("fel_scriptaddr", spl->fel_script_address);
}
/*
 * Note this function gets called multiple times.
 * It must not make any changes to env variables which already exist.
 */
static void setup_environment(const void *fdt)
{
	if (!env_get("board_name"))
		env_set("board_name", "A33-OLinuXino-n8GB");

	if (!env_get("fdtfile"))
		env_set("fdtfile", olimex_get_board_fdt());

}

int misc_init_r(void)
{
	__maybe_unused int ret;
	uint boot;

	env_set("fel_booted", NULL);
	env_set("fel_scriptaddr", NULL);
	env_set("mmc_bootdev", NULL);

	boot = sunxi_get_boot_device();
	/* determine if we are running in FEL mode */
	if (boot == BOOT_DEVICE_BOARD) {
		env_set("fel_booted", "1");
		parse_spl_header(SPL_ADDR);
	/* or if we booted from MMC, and which one */
	} else if (boot == BOOT_DEVICE_MMC1) {
		env_set("mmc_bootdev", "0");
	}

	setup_environment(gd->fdt_blob);

#ifdef CONFIG_USB_ETHER
	usb_ether_init();
#endif

	return 0;
}

int ft_board_setup(void *blob, bd_t *bd)
{
	int __maybe_unused r;

	/*
	 * Call setup_environment again in case the boot fdt has
	 * ethernet aliases the u-boot copy does not have.
	 */
	setup_environment(blob);

#ifdef CONFIG_VIDEO_DT_SIMPLEFB
	r = sunxi_simplefb_setup(blob);
	if (r)
		return r;
#endif
	return 0;
}


int show_board_info(void)
{
	printf("Model: %s\n", olimex_get_board_name());
	return 0;
}

#ifdef CONFIG_SET_DFU_ALT_INFO
void set_dfu_alt_info(char *interface, char *devstr)
{
	char *p = NULL;

	printf("interface: %s, devstr: %s\n", interface, devstr);

#ifdef CONFIG_DFU_MMC
	if (!strcmp(interface, "mmc"))
		p = env_get("dfu_alt_info_mmc0");
#endif

#ifdef CONFIG_DFU_RAM
	if (!strcmp(interface, "ram"))
		p = env_get("dfu_alt_info_ram");
#endif

#ifdef CONFIG_DFU_NAND
	if (!strcmp(interface, "nand"))
		p = env_get("dfu_alt_info_nand");
#endif

	if (p != NULL)
		env_set("dfu_alt_info", p);
}

#endif /* CONFIG_SET_DFU_ALT_INFO */

#endif /* !CONFIG_SPL_BUILD */
