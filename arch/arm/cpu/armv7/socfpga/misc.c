/*
 *  Copyright Altera Corporation (C) 2012-2013. All rights reserved
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms and conditions of the GNU General Public License,
 *  version 2, as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/reset_manager.h>
#include <asm/arch/system_manager.h>
#include <asm/arch/interrupts.h>
#include <asm/arch/sdram.h>
#include <asm/arch/dwmmc.h>
#include <mmc.h>
#include <dwmmc.h>
#include <altera.h>
#include <fpga.h>
#ifndef CONFIG_SPL_BUILD
#include <netdev.h>
#include <phy.h>
#endif


DECLARE_GLOBAL_DATA_PTR;

static const struct socfpga_reset_manager *reset_manager_base =
		(void *)SOCFPGA_RSTMGR_ADDRESS;

/*
 * FPGA programming support for SoC FPGA Cyclone V
 */
/* currently only single FPGA device avaiable on dev kit */
Altera_desc altera_fpga[CONFIG_FPGA_COUNT] = {
	{Altera_SoCFPGA, /* family */
	fast_passive_parallel, /* interface type */
	-1,		/* no limitation as
			additional data will be ignored */
	NULL,		/* no device function table */
	NULL,		/* base interface address specified in driver */
	0}		/* no cookie implementation */
};

/* add device descriptor to FPGA device table */
void socfpga_fpga_add(void)
{
	int i;
	fpga_init();
	for (i = 0; i < CONFIG_FPGA_COUNT; i++)
		fpga_add(fpga_altera, &altera_fpga[i]);
}

/*
 * Print CPU information
 */
int print_cpuinfo(void)
{
	puts("CPU   : Altera SOCFPGA Platform\n");
	return 0;
}

int misc_init_r(void)
{
	char buf[32];

	/* add device descriptor to FPGA device table */
	socfpga_fpga_add();

	/* create environment for bridges and handoff */

	/* hps peripheral controller to fgpa */
	setenv_addr("fpgaintf", (void *)SYSMGR_FPGAINTF_MODULE);
	sprintf(buf, "0x%08x", readl(ISWGRP_HANDOFF_FPGAINTF));
	setenv("fpgaintf_handoff", buf);

	/* fpga2sdram port */
	setenv_addr("fpga2sdram", (void *)(SOCFPGA_SDR_ADDRESS +
		SDR_CTRLGRP_FPGAPORTRST_ADDRESS));
	sprintf(buf, "0x%08x", readl(ISWGRP_HANDOFF_FPGA2SDR));
	setenv("fpga2sdram_handoff", buf);
	setenv_addr("fpga2sdram_apply", (void *)sdram_applycfg_uboot);

	/* axi bridges (hps2fpga, lwhps2fpga and fpga2hps) */
	setenv_addr("axibridge", (void *)&reset_manager_base->brg_mod_reset);
#ifndef CONFIG_LARK_BOARD
	sprintf(buf, "0x%08x", readl(ISWGRP_HANDOFF_AXIBRIDGE));
#else
	sprintf(buf, "0x%08x", 0x0);
#endif
	setenv("axibridge_handoff", buf);

	/* l3 remap register */
	setenv_addr("l3remap", (void *)SOCFPGA_L3REGS_ADDRESS);
#ifndef CONFIG_LARK_BOARD
	sprintf(buf, "0x%08x", readl(ISWGRP_HANDOFF_L3REMAP));
#else
	sprintf(buf, "0x%08x", 0x19);
#endif
	setenv("l3remap_handoff", buf);


	/* add signle command to enable all bridges based on handoff */
	setenv("bridge_enable_handoff",
		"mw $fpgaintf ${fpgaintf_handoff}; "
		"mw $fpga2sdram ${fpga2sdram_handoff}; "
		"go $fpga2sdram_apply; "
		"mw $axibridge ${axibridge_handoff}; "
		"mw $l3remap ${l3remap_handoff} ");

	/* add signle command to disable all bridges */
	setenv("bridge_disable",
		"mw $fpgaintf 0; "
		"mw $fpga2sdram 0; "
		"go $fpga2sdram_apply; "
		"mw $axibridge 0; "
		"mw $l3remap 0x1 ");

	return 0;
}

#if defined(CONFIG_SYS_CONSOLE_IS_IN_ENV) && \
defined(CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE)
int overwrite_console(void)
{
	return 0;
}
#endif

/*
 * Initializes MMC controllers.
 * to override, implement board_mmc_init()
 */
int cpu_mmc_init(bd_t *bis)
{
#ifdef CONFIG_DWMMC
	return altera_dwmmc_init(CONFIG_SDMMC_BASE,
		CONFIG_DWMMC_BUS_WIDTH, 0);
#else
	return 0;
#endif
}

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

