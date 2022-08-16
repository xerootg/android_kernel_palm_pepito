/* Copyright (c) 2013-2014,2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/export.h>
#include <linux/types.h>
#include <soc/qcom/boot_stats.h>
//add begin by TCTSZ.haimei.liu@tcl.com.2017/12/19.for print powerup reason.task: 5558422.
#include <soc/qcom/smem.h>
//add end by TCTSZ.haimei.liu@tcl.com.2017/12/19.for print powerup reason.task: 5558422.

static void __iomem *mpm_counter_base;
static uint32_t mpm_counter_freq;
struct boot_stats __iomem *boot_stats;

static int mpm_parse_dt(void)
{
	struct device_node *np;
	u32 freq;

	np = of_find_compatible_node(NULL, NULL, "qcom,msm-imem-boot_stats");
	if (!np) {
		pr_err("can't find qcom,msm-imem node\n");
		return -ENODEV;
	}
	boot_stats = of_iomap(np, 0);
	if (!boot_stats) {
		pr_err("boot_stats: Can't map imem\n");
		return -ENODEV;
	}

	np = of_find_compatible_node(NULL, NULL, "qcom,mpm2-sleep-counter");
	if (!np) {
		pr_err("mpm_counter: can't find DT node\n");
		return -ENODEV;
	}

	if (!of_property_read_u32(np, "clock-frequency", &freq))
		mpm_counter_freq = freq;
	else
		return -ENODEV;

	if (of_get_address(np, 0, NULL, NULL)) {
		mpm_counter_base = of_iomap(np, 0);
		if (!mpm_counter_base) {
			pr_err("mpm_counter: cant map counter base\n");
			return -ENODEV;
		}
	}

	return 0;
}
//add begin by TCTSZ.haimei.liu@tcl.com.2017/12/19.for print powerup reason.task: 5558422.
static void tct_print_pon_reason(void)
{
	u8 TCT_PON_PON_REASON1;
	u16 TCT_PON_WARM_RESET_REASON, TCT_PON_POFF_REASON, TCT_PON_SOFT_RESET_REASON;
	uint32_t* tct_ret = NULL;
	char pwr_on_reason[180]={0};

	tct_ret = (uint32_t *) smem_alloc(SMEM_PRINT_PON_REASON,4*sizeof(uint32_t),0,SMEM_ANY_HOST_FLAG);
	if(tct_ret != NULL){
		TCT_PON_PON_REASON1 = (u8) *tct_ret;
		TCT_PON_POFF_REASON = (u16) *(tct_ret+1);
		TCT_PON_WARM_RESET_REASON = (u16) *(tct_ret+2);
		TCT_PON_SOFT_RESET_REASON = (u16) *(tct_ret+3);
	}else{
		pr_info("tct_print_pon_reason in kernel error\n");
		return;
	}
	
	pr_info(" tct_print_pon_reason:TCT_PON_PON_REASON1=0x%x,TCT_PON_POFF_REASON=0x%x,TCT_PON_WARM_RESET_REASON=0x%x,TCT_PON_SOFT_RESET_REASON=0x%x,\n",
		TCT_PON_PON_REASON1,TCT_PON_POFF_REASON,TCT_PON_WARM_RESET_REASON,TCT_PON_SOFT_RESET_REASON);


	sprintf(pwr_on_reason, "Power-on reason: 0x%hhx ", TCT_PON_PON_REASON1);
   	 if(TCT_PON_PON_REASON1 & 0x80)
                strcat(pwr_on_reason, " KPD (power key press),");
        if(TCT_PON_PON_REASON1 & 0x40)
                strcat(pwr_on_reason, " CBL (external power supply),");
        if(TCT_PON_PON_REASON1 & 0x20)
                strcat(pwr_on_reason, " PON1 (secondary PMIC),");
        if(TCT_PON_PON_REASON1 & 0x10)
                strcat(pwr_on_reason, " USB (USB charger insertion),");
        if(TCT_PON_PON_REASON1 & 0x08)
                strcat(pwr_on_reason, " DC (DC charger insertion),");
        if(TCT_PON_PON_REASON1 & 0x04)
                strcat(pwr_on_reason, " RTC (RTC alarm expiry),");
        if(TCT_PON_PON_REASON1 & 0x02)
                strcat(pwr_on_reason, " SMPL (sudden momentary power loss),");
        if(TCT_PON_PON_REASON1 & 0x01)
                strcat(pwr_on_reason, " Hard Reset");
	pr_info(" %s\n",pwr_on_reason);

	  sprintf(pwr_on_reason, "Power-off reason: 0x%hx  ", TCT_PON_POFF_REASON);
      switch(TCT_PON_POFF_REASON) {
                case 0x8000:
                        strcat(pwr_on_reason, "stage3 reset");
                        break;
                case 0x4000:
                        strcat(pwr_on_reason, "Overtemp");
                        break;
                case 0x2000:
                        strcat(pwr_on_reason, "UVLO");
                        break;
                case 0x1000:
                        strcat(pwr_on_reason, "TFT");
                        break;
                case 0x0800:
                        strcat(pwr_on_reason, "Charger");
                        break;
                case 0x0400:
                        strcat(pwr_on_reason, "AVDD_RB");
                        break;
                case 0x80:
                        strcat(pwr_on_reason, "KPDPWR_N");
                        break;
                case 0x40:
                        strcat(pwr_on_reason, "RESIN_N");
                        break;
                case 0x20:
                        strcat(pwr_on_reason, "simultaneous KPDPWR_N + RESIN_N");
                        break;
                case 0x10:
                        strcat(pwr_on_reason, "Keypad_Reset2");
                        break;
                case 0x08:
                        strcat(pwr_on_reason, "Keypad_Reset1");
                        break;
                case 0x04:
                        strcat(pwr_on_reason, "PMIC watchdog");
                        break;
                case 0x02:
                        strcat(pwr_on_reason, "PS_HOLD");
                        break;
                case 0x01:
                        strcat(pwr_on_reason, "Software");
                        break;
                default:
                        break;
        }
 	pr_info(" %s\n",pwr_on_reason);

  	sprintf(pwr_on_reason, "PON_WARM_RESET_REASON: 0x%hx  ", TCT_PON_WARM_RESET_REASON);
        switch(TCT_PON_WARM_RESET_REASON) {
                case 0x1000:
                        strcat(pwr_on_reason, "TFT");
                        break;

                case 0x80:
                        strcat(pwr_on_reason, "KPDPWR_N");
                        break;
                case 0x40:
                        strcat(pwr_on_reason, "RESIN_N");
                        break;
                case 0x20:
                        strcat(pwr_on_reason, "simultaneous KPDPWR_N + RESIN_N");
                        break;
                case 0x10:
                        strcat(pwr_on_reason, "Keypad_Reset2");
                        break;
                case 0x08:
                        strcat(pwr_on_reason, "Keypad_Reset1");
                        break;
                case 0x04:
                        strcat(pwr_on_reason, "PMIC watchdog");
                        break;
                case 0x02:
                        strcat(pwr_on_reason, "PS_HOLD");
                        break;
                case 0x01:
                        strcat(pwr_on_reason, "Software");
                        break;
                default:
                        break;
        }

  	pr_info(" %s\n",pwr_on_reason);

  	sprintf(pwr_on_reason, "PON_SOFT_RESET_REASON: 0x%hx  ", TCT_PON_SOFT_RESET_REASON);
        switch(TCT_PON_SOFT_RESET_REASON) {
                case 0x1000:
                        strcat(pwr_on_reason, "TFT");

                case 0x80:
                        strcat(pwr_on_reason, "KPDPWR_N");
                        break;
                case 0x40:
                        strcat(pwr_on_reason, "RESIN_N");
                        break;
                case 0x20:
                        strcat(pwr_on_reason, "KPDPWR_AND_RESIN");
                        break;
                case 0x10:
                        strcat(pwr_on_reason, "Keypad_Reset2");
                        break;
                case 0x08:
                        strcat(pwr_on_reason, "Keypad_Reset1");
                        break;
                case 0x04:
                        strcat(pwr_on_reason, "PMIC watchdog");
                        break;
                case 0x02:
                        strcat(pwr_on_reason, "PS_HOLD");
                        break;
                case 0x01:
                        strcat(pwr_on_reason, "Software");
                        break;
                default:
                        break;
        }

	pr_info(" %s\n",pwr_on_reason);	
	
}
//add end by TCTSZ.haimei.liu@tcl.com.2017/12/19.for print powerup reason.task: 5558422.

static void print_boot_stats(void)
{
	pr_info("KPI: Bootloader start count = %u\n",
		readl_relaxed(&boot_stats->bootloader_start));
	pr_info("KPI: Bootloader end count = %u\n",
		readl_relaxed(&boot_stats->bootloader_end));
	pr_info("KPI: Bootloader display count = %u\n",
		readl_relaxed(&boot_stats->bootloader_display));
	pr_info("KPI: Bootloader load kernel count = %u\n",
		readl_relaxed(&boot_stats->bootloader_load_kernel));
	pr_info("KPI: Kernel MPM timestamp = %u\n",
		readl_relaxed(mpm_counter_base));
	pr_info("KPI: Kernel MPM Clock frequency = %u\n",
		mpm_counter_freq);

//add begin by TCTSZ.haimei.liu@tcl.com.2017/12/19.for print powerup reason.task: 5558422.
	tct_print_pon_reason();
//add end by TCTSZ.haimei.liu@tcl.com.2017/12/19.for print powerup reason.task: 5558422.
}

unsigned long long int msm_timer_get_sclk_ticks(void)
{
	unsigned long long int t1, t2;
	int loop_count = 10;
	int loop_zero_count = 3;
	int tmp = USEC_PER_SEC;
	void __iomem *sclk_tick;

	do_div(tmp, TIMER_KHZ);
	tmp /= (loop_zero_count-1);
	sclk_tick = mpm_counter_base;
	if (!sclk_tick)
		return -EINVAL;
	while (loop_zero_count--) {
		t1 = __raw_readl_no_log(sclk_tick);
		do {
			udelay(1);
			t2 = t1;
			t1 = __raw_readl_no_log(sclk_tick);
		} while ((t2 != t1) && --loop_count);
		if (!loop_count) {
			pr_err("boot_stats: SCLK  did not stabilize\n");
			return 0;
		}
		if (t1)
			break;

		udelay(tmp);
	}
	if (!loop_zero_count) {
		pr_err("boot_stats: SCLK reads zero\n");
		return 0;
	}
	return t1;
}

int boot_stats_init(void)
{
	int ret;

	ret = mpm_parse_dt();
	if (ret < 0)
		return -ENODEV;

	print_boot_stats();

	if (!(boot_marker_enabled()))
		boot_stats_exit();
	return 0;
}

int boot_stats_exit(void)
{
	iounmap(boot_stats);
	iounmap(mpm_counter_base);
	return 0;
}
