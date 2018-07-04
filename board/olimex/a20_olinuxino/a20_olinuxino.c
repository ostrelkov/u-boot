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
#include <netdev.h>
#include <miiphy.h>
#include <crc.h>
#include <mmc.h>
#include <spl.h>
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

#include "board_detect.h"
#include "boards.h"

#define GMAC_MODE_RGMII		0
#define GMAC_MODE_MII		1

DECLARE_GLOBAL_DATA_PTR;

void eth_init_board(void)
{
	#if 0
	static struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	struct olimex_revision *rev;
	uint8_t tx_delay = 0;
	uint8_t mode;
	int pin;

 	mode = (olimex_board_is_lime() || olimex_board_is_micro()) ?
 		GMAC_MODE_MII : GMAC_MODE_RGMII;

	if (olimex_board_is_lime2()) {
		rev = olimex_get_eeprom_revision();

		if (rev->major > 'E')
			/* RTL8211E */
			tx_delay = 2;
		else if (rev->major > 'G')
			/* KSZ9031 */
			tx_delay = 4;
	} else if (olimex_board_is_som204()) {
		tx_delay = 4;
	}

	/* Set up clock gating */
	setbits_le32(&ccm->ahb_gate1, 0x1 << AHB_GATE_OFFSET_GMAC);

	if (mode == GMAC_MODE_RGMII) {
		setbits_le32(&ccm->gmac_clk_cfg,
			     CCM_GMAC_CTRL_TX_CLK_DELAY(tx_delay));
		setbits_le32(&ccm->gmac_clk_cfg,
			     CCM_GMAC_CTRL_TX_CLK_SRC_INT_RGMII |
			     CCM_GMAC_CTRL_GPIT_RGMII);
	} else {
		setbits_le32(&ccm->gmac_clk_cfg,
			     CCM_GMAC_CTRL_TX_CLK_SRC_MII |
			     CCM_GMAC_CTRL_GPIT_MII);
	}

	/* Configure pins for GMAC */
	for (pin = SUNXI_GPA(0); pin <= SUNXI_GPA(16); pin++) {

		/* skip unused pins in RGMII mode */
		if (mode == GMAC_MODE_RGMII ) {
			if (pin == SUNXI_GPA(9) || pin == SUNXI_GPA(14))
				continue;
		}

		sunxi_gpio_set_cfgpin(pin, SUN7I_GPA_GMAC);
		sunxi_gpio_set_drv(pin, 3);
	}

	if (olimex_board_is_micro()) {
		sunxi_gpio_set_cfgpin(SUNXI_GPA(17),  SUN7I_GPA_GMAC);
	}

#endif
}

void i2c_init_board(void)
{
#ifdef CONFIG_I2C0_ENABLE
	sunxi_gpio_set_cfgpin(SUNXI_GPB(0), SUN4I_GPB_TWI0);
	sunxi_gpio_set_cfgpin(SUNXI_GPB(1), SUN4I_GPB_TWI0);
	clock_twi_onoff(0, 1);
#endif

#ifdef CONFIG_I2C1_ENABLE
	sunxi_gpio_set_cfgpin(SUNXI_GPB(18), SUN4I_GPB_TWI1);
	sunxi_gpio_set_cfgpin(SUNXI_GPB(19), SUN4I_GPB_TWI1);
	clock_twi_onoff(1, 1);
#endif
}

#if defined(CONFIG_ENV_IS_IN_MMC) && defined(CONFIG_ENV_IS_IN_FAT)
enum env_location env_get_location(enum env_operation op, int prio)
{
	switch (prio) {
	case 0:
		return ENVL_FAT;

	case 1:
		return ENVL_MMC;

	default:
		return ENVL_UNKNOWN;
	}
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
#if 0
	/*
	 * Setup SATAPWR
	 */
	if(olimex_board_is_micro())
		satapwr_pin = sunxi_name_to_gpio("PB8");
	else if(olimex_board_is_lime() || olimex_board_is_lime2())
		satapwr_pin = sunxi_name_to_gpio("PC3");
	else
		satapwr_pin = sunxi_name_to_gpio("");

	if(satapwr_pin > 0) {
		gpio_request(satapwr_pin, "satapwr");
		gpio_direction_output(satapwr_pin, 1);
		/* Give attached sata device time to power-up to avoid link timeouts */
		mdelay(500);
	}

	/*
	 * A20-SOM204 needs manual reset for rt8723bs chip
	 */
	if (olimex_board_is_som204()) {
		btpwr_pin = sunxi_name_to_gpio("PB11");

		gpio_request(btpwr_pin, "btpwr");
		gpio_direction_output(btpwr_pin, 0);
		mdelay(100);
		gpio_direction_output(btpwr_pin, 1);
	}
	#endif

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((long *)PHYS_SDRAM_0, PHYS_SDRAM_0_SIZE);

	return 0;
}

#ifdef CONFIG_MMC
static void mmc_pinmux_setup(int sdc)
{
	unsigned int pin;
	__maybe_unused int pins;

	switch (sdc) {
	case 0:
		/* SDC0: PF0-PF5 */
		for (pin = SUNXI_GPF(0); pin <= SUNXI_GPF(5); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPF_SDC0);
			sunxi_gpio_set_drv(pin, 2);
		}
		break;

	case 1:
		break;

	case 2:
		/* SDC2: PC6-PC11 */
		for (pin = SUNXI_GPC(6); pin <= SUNXI_GPC(11); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPC_SDC2);
			sunxi_gpio_set_pull(pin, SUNXI_GPIO_PULL_UP);
			sunxi_gpio_set_drv(pin, 2);
		}
		break;

	case 3:
		/* SDC3: PI4-PI9 */
		for (pin = SUNXI_GPI(4); pin <= SUNXI_GPI(9); pin++) {
			sunxi_gpio_set_cfgpin(pin, SUNXI_GPI_SDC3);
			sunxi_gpio_set_drv(pin, 2);
		}

	default:
		printf("sunxi: invalid MMC slot %d for pinmux setup\n", sdc);
		break;
	}
}

int board_mmc_init(bd_t *bis)
{
	struct mmc *mmc;

	/* Try to initialize MMC0 */
	mmc_pinmux_setup(0);
	mmc = sunxi_mmc_init(0);
	if (!mmc)
		return -1;

	/* Initialize MMC2 on boards with eMMC */
	if (eeprom->config.storage == 'e') {
		mmc_pinmux_setup(2);
		mmc = sunxi_mmc_init(2);
		if (!mmc)
			return -1;
	}

	return 0;
}
#endif

#ifdef CONFIG_SPL_BUILD
void sunxi_board_init(void)
{
	int power_failed = 0;

	power_failed = axp_init();
	if (power_failed)
		printf("axp_init failed!\n");

	power_failed |= axp_set_dcdc2(CONFIG_AXP_DCDC2_VOLT);
	power_failed |= axp_set_dcdc3(CONFIG_AXP_DCDC3_VOLT);
	power_failed |= axp_set_aldo2(CONFIG_AXP_ALDO2_VOLT);
	power_failed |= axp_set_aldo3(CONFIG_AXP_ALDO3_VOLT);
	power_failed |= axp_set_aldo4(CONFIG_AXP_ALDO4_VOLT);

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

	if (olimex_i2c_eeprom_read())
		printf("EEPROM: Error\n");
	else if (olimex_eeprom_is_valid())
		printf("EEPROM: Ready\n");
	else
		printf("EEPROM: Corrupted!\n");
}
#endif

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

	ret = sun4i_usb_phy_vbus_detect(&phy);
	if (ret == 1) {
		pr_err("A charger is plugged into the OTG\n");
		return -ENODEV;
	}

	return ret;
}
#endif

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
#endif

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
	struct olimex_revision *rev;
	char serial_string[17] = { 0 };
	unsigned int sid[4];
	uint8_t mac_addr[6] = { 0, 0, 0, 0, 0, 0 };
	char ethaddr[16], digit[3], strrev[3];
	int i, ret;
	char *p;

	if (olimex_eeprom_is_valid()) {

		if (!env_get("board_id"))
			env_set_ulong("board_id", eeprom->id);

		if (!env_get("board_name"))
			env_set("board_name", olimex_get_board_name(eeprom->id));

		if (!env_get("board_rev")) {
			strrev[0] = (eeprom->revision.major < 'A' || eeprom->revision.major > 'Z') ? 0 : eeprom->revision.major;
			strrev[1] = (eeprom->revision.minor < '1' || eeprom->revision.minor > '9') ? 0 : eeprom->revision.minor;
			strrev[2] = 0;
			env_set("board_rev", strrev);
		}

		p = eeprom->mac;
		for (i = 0; i < 6; i++) {
			sprintf(digit, "%c%c", p[i * 2], p[i * 2 + 1]);
			mac_addr[i] = simple_strtoul(digit, NULL, 16);
		}

		if (fdt_get_alias(fdt, "ethernet0"))
			if (!env_get("ethaddr"))
				eth_env_set_enetaddr("ethaddr", mac_addr);

		if (fdt_get_alias(fdt, "ethernet2")) {
			if (!strncmp(p, "301F9AD", 7))
				mac_addr[0] |= 0x02;
			if (!env_get("eth2addr"))
				eth_env_set_enetaddr("eth2addr", mac_addr);
		}
	}

	ret = sunxi_get_sid(sid);
	if (ret == 0 && sid[0] != 0) {

		/* Ensure the NIC specific bytes of the mac are not all 0 */
		if ((sid[3] & 0xffffff) == 0)
			sid[3] |= 0x800000;

		for (i = 0; i < 4; i++) {
			sprintf(ethaddr, "ethernet%d", i);
			if (!fdt_get_alias(fdt, ethaddr))
				continue;

			if (i == 0)
				strcpy(ethaddr, "ethaddr");
			else
				sprintf(ethaddr, "eth%daddr", i);

			/* Non OUI / registered MAC address */
			mac_addr[0] = (i << 4) | 0x02;
			mac_addr[1] = (sid[0] >>  0) & 0xff;
			mac_addr[2] = (sid[3] >> 24) & 0xff;
			mac_addr[3] = (sid[3] >> 16) & 0xff;
			mac_addr[4] = (sid[3] >>  8) & 0xff;
			mac_addr[5] = (sid[3] >>  0) & 0xff;

			if (!env_get(ethaddr))
				eth_env_set_enetaddr(ethaddr, mac_addr);
		}
	}

	if (!env_get("serial#")) {
		snprintf(serial_string, sizeof(serial_string),
			"%08x", eeprom->serial);
		env_set("serial#", serial_string);
	}

	if (!env_get("fdtfile"))
		env_set("fdtfile", olimex_get_board_fdt(eeprom->id));

#if 0
	if (!env_get("olinuxino_overlays"))
		env_set("olinuxino_overlays", olimex_get_board_overlays());
#endif

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
	} else if (boot == BOOT_DEVICE_MMC2) {
		env_set("mmc_bootdev", "1");
	} else if (boot == BOOT_DEVICE_SPI) {
		env_set("spi_booted", "1");
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
	struct olimex_revision *rev;
	const char *name;
	char *mac = eeprom->mac;

	if (olimex_eeprom_is_valid()) {

		/* Get board name and compare if with eeprom content */
		name = olimex_get_board_name(eeprom->id);

		printf("Model: %s Rev.%c%c\n", name,
		       (eeprom->revision.major < 'A' ||
			eeprom->revision.major > 'Z') ?
			0 : eeprom->revision.major,
		       (eeprom->revision.minor < '1' ||
			eeprom->revision.minor > '9') ?
			0 : eeprom->revision.minor);

		printf("Serial:%08X\n", eeprom->serial);
		printf("MAC:   %c%c:%c%c:%c%c:%c%c:%c%c:%c%c\n",
		       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
		       mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);
	} else {
		printf("Model: Unknown\n");
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







#if 0


static int do_config_info(cmd_tbl_t *cmdtp, int flag,
			  int argc, char *const argv[])
{
	struct olimex_revision *rev;
	const char *name;
	char *mac;

	if (olimex_i2c_eeprom_read()) {
		printf("Failed to read the current EEPROM configuration!\n");
		return CMD_RET_FAILURE;
	}
	if (!olimex_eeprom_is_valid()) {
		printf("Current configuration in the EEPROM is not valid!\n"
		       "Run \"olimex config write\" to restore it.\n");
		return CMD_RET_SUCCESS;
	}

	/* Get board info */
	rev = olimex_get_eeprom_revision();
	mac = olimex_get_eeprom_mac();
	name = olimex_get_board_name();

	/* Major revision should be A-Z */
	if (rev->major < 'A' || rev->major > 'Z') {
		rev->major = ' ';
	} else {
		/* Minor is not required */
		if (rev->minor < '1' || rev->minor > '9')
			rev->minor = ' ';
	}

	printf("Model: %s\n", name);
	printf("Rev:   %c%c\n", rev->major, rev->minor);
	printf("Serial:%08X\n", olimex_get_eeprom_serial());
	printf("MAC:   %c%c:%c%c:%c%c:%c%c:%c%c:%c%c\n",
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
	       mac[6], mac[7], mac[8], mac[9], mac[10], mac[11]);

	return CMD_RET_SUCCESS;
}

static int do_config_list(cmd_tbl_t *cmdtp, int flag,
			  int argc, char *const argv[])
{
	struct board_table *board;

	printf("Supported boards:\n");
	printf("----------------------------------------\n");


	for (board = olimex_get_board_list(); board->id != 0; board++)
		printf("%-30s - %-10d\n", board->name, board->id);
	return CMD_RET_SUCCESS;
}

static int do_config_write(cmd_tbl_t *cmdtp, int flag,
			   int argc, char *const argv[])
{
	struct olimex_revision rev;
	struct board_table *board;
	uint32_t serial;
	uint32_t id;
	uint8_t i = 0;
	char mac[13];
	char *p;



	if (argc < 3 || argc > 5)
		return CMD_RET_USAGE;

	id = simple_strtoul(argv[1], NULL, 10);
	board = olimex_get_board_list();
	do {
		if (board->id == id)
			break;

		board++;
		if (board->id == 0) {
			printf("%d is not valid ID!\n"
			       "Run olimex config list to get supported IDs.\n", id);
			return CMD_RET_FAILURE;
		}
	} while (board->id != 0);

	rev.major = argv[2][0];
	rev.minor = '\0';

	/* Make uppercase */
	if (rev.major >= 'a' && rev.major <= 'z')
		rev.major += 0x20;

	if (rev.major < 'A' || rev.major > 'Z') {
		printf("%c in not valid revision!\n"
		       "Revision should be one character: A, C, J, etc...\n", (char)rev.major);
		return CMD_RET_FAILURE;
	}

	if (argc > 3)
		serial = simple_strtoul(argv[3], NULL, 16);

	if (argc > 4) {
		p = argv[4];
		while (*p) {
			if ((*p < '0' || *p > '9') && (*p < 'a' || *p > 'f') && (*p < 'A' || *p > 'F') && (*p != ':')) {
				printf("Invalid character: %d(%c)!\n", *p, *p);
				return CMD_RET_FAILURE;
			}

			if (*p != ':')
				mac[i++] = toupper(*p);
			p++;
		};

		mac[i] = 0;
		if (i != 12) {
			printf("Invalid MAC address lenght: %d!\n", i);
			return CMD_RET_FAILURE;
		}
	}

	printf("Erasing previous configuration...\n");
	if (olimex_i2c_eeprom_erase())
		return CMD_RET_FAILURE;

	printf("Writting configuration EEPROM...\n");
	olimex_set_eeprom_id(id);
	olimex_set_eeprom_revision(&rev);
	olimex_set_eeprom_config(&board->config);
	if (argc > 3)
		olimex_set_eeprom_serial(serial);
	if (argc > 4)
		olimex_set_eeprom_mac(mac);

	olimex_i2c_eeprom_write();
	olimex_i2c_eeprom_read();
	return 0;
}

static int do_config_erase(cmd_tbl_t *cmdtp, int flag,
			   int argc, char *const argv[])
{
	printf("Erasing configuration EEPROM...\n");
	return olimex_i2c_eeprom_erase();
}

static cmd_tbl_t cmd_config[] = {
	U_BOOT_CMD_MKENT(info,	1, 0, do_config_info,  "", ""),
	U_BOOT_CMD_MKENT(list,	1, 0, do_config_list,  "", ""),
	U_BOOT_CMD_MKENT(write, 5, 0, do_config_write, "", ""),
	U_BOOT_CMD_MKENT(erase, 1, 0, do_config_erase, "", ""),
};

static int do_config_opts(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *cp;

	cp = find_cmd_tbl(argv[1], cmd_config, ARRAY_SIZE(cmd_config));

	argc--;
	argv++;

	if (cp == NULL || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cp->repeatable)
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}



























static cmd_tbl_t cmd_olimex[] = {
	U_BOOT_CMD_MKENT(config, CONFIG_SYS_MAXARGS, 0, do_config_opts, "", ""),
};

static int do_olimex_ops(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *cp;

	cp = find_cmd_tbl(argv[1], cmd_olimex, ARRAY_SIZE(cmd_olimex));

	argc--;
	argv++;

	if (cp == NULL || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cp->repeatable)
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(
	olimex, 7, 0, do_olimex_ops,
	"OLinuXino board configurator",
	"config info		- Print current configuration: ID, serial, ram, storage, grade...\n"
	"olimex config list		- Print supported boards and their IDs\n"
	"olimex config erase		- Erase currently stored configuration\n"
	"olimex config write [id] [revision] [serial] [mac]\n"
	"  arguments:\n"
	"    [id]			- Specific board ID\n"
	"    [revision]			- Board revision: C, D1, etc...\n"
	"    [serial]			- New serial number for the board\n"
	"    [mac]			- New MAC address for the board\n"
	"				  Format can be:\n"
	"					aa:bb:cc:dd:ee:ff\n"
	"					FF:FF:FF:FF:FF:FF\n"
	"					aabbccddeeff\n"
	);

#endif
#endif
