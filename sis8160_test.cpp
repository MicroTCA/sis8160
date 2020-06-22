/**
*Copyright 2016-  DESY (Deutsches Elektronen-Synchrotron, www.desy.de)
*
*This file is part of SIS8300 driver.
*
*SIS8300 is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*SIS8300 is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with SIS8300.  If not, see <http://www.gnu.org/licenses/>.
**/

/*
*	Author: Ludwig Petrosyan (Email: ludwig.petrosyan@desy.de)
*/


#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>

#include <iostream>
#include <fstream>

#include "sis8160_defs.h"
#include "sis8160_reg.h"

using namespace std;


/* useconds from struct timeval */
#define	MIKRS(tv) (((double)(tv).tv_usec ) + ((double)(tv).tv_sec * 1000000.0)) 
#define	MILLS(tv) (((double)(tv).tv_usec/1000 )  + ((double)(tv).tv_sec * 1000.0)) 


int	         fd;
struct timeval   start_time;
struct timeval   end_time;

void fmc_hmc_spi_write (uint32_t fmc_no, uint32_t spi_addr, uint32_t spi_data)
{
	device_ioctrl_data	rw_Ctrl;
	
	rw_Ctrl.offset          = spi_addr;
	rw_Ctrl.data            = spi_data;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = fmc_no;
	ioctl(fd, SFMC_HMC_SPI_RW, &rw_Ctrl);
}

void fmc_hmc_spi_read (uint32_t fmc_no, uint32_t spi_addr, uint32_t *spi_data)
{
	device_ioctrl_data	rw_Ctrl;
	
	rw_Ctrl.offset          = spi_addr;
	rw_Ctrl.data            = 0;
	rw_Ctrl.cmd            = IOCTRL_R;
	rw_Ctrl.reserved    = fmc_no;
	ioctl(fd, SFMC_HMC_SPI_RW, &rw_Ctrl);
	*spi_data = rw_Ctrl.data;
}

void fmc_adc_spi_write (uint32_t fmc_no, uint32_t spi_addr, uint32_t spi_data)
{
	device_ioctrl_data	rw_Ctrl;
	
	rw_Ctrl.offset          = spi_addr;
	rw_Ctrl.data            = spi_data;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = fmc_no;
	ioctl(fd, SFMC_ADC_SPI_RW, &rw_Ctrl);
}

void fmc_adc_spi_read (uint32_t fmc_no, uint32_t spi_addr, uint32_t *spi_data)
{
	device_ioctrl_data	rw_Ctrl;
	
	rw_Ctrl.offset          = spi_addr;
	rw_Ctrl.data            = 0;
	rw_Ctrl.cmd            = IOCTRL_R;
	rw_Ctrl.reserved    = fmc_no;
	ioctl(fd, SFMC_ADC_SPI_RW, &rw_Ctrl);
	*spi_data = rw_Ctrl.data;
}

int  sfmc01_hmc_check_lock(uint32_t fmc_no )
{
	uint32_t spi_data ;
	uint32_t lock_bit = 0 ;

	// check HMC PLL1 Lock Status
	fmc_hmc_spi_read (fmc_no, 0x7c, &spi_data) ;
	if ((spi_data & 0x7f) !=  0x20) {
		//printf("PLL1 is not locked !   reg 0x7C = 0x%08X \n", spi_data);
		lock_bit = 0 ;
		return lock_bit;
	}

	// check HMC PLL1 and PLL2 Lock Status
	fmc_hmc_spi_read (fmc_no, 0x7d, &spi_data) ;
	if ((spi_data & 0x8) !=  0x8) {
		//printf("PLL1 or PLL2 is not locked !   reg 0x7D = 0x%08X \n", data);
		lock_bit = 0 ;
		return lock_bit;
	}


	lock_bit = 1 ;

	return  lock_bit;
}

int sfmc01_adc_check_lock(uint32_t fmc_no )
{
	uint32_t spi_addr ;
	uint32_t spi_rd_data ;
	uint32_t lock_bit = 0 ;

	// check ADC PLL Lock Status
	spi_addr = 0x56F ; //
	fmc_adc_spi_read(fmc_no,  spi_addr, &spi_rd_data); //
	if ((spi_rd_data & 0xff) !=  0x80) {
		//printf("ADC PLL is not locked !   reg 0x56F = 0x%08X \n", spi_rd_data);
	   lock_bit = 0 ;
		return lock_bit;
	}
	lock_bit = 1 ;
	return  lock_bit;
}

void fmc_hmc_load_reserved_congiruation_registers (uint32_t fmc_no)
{
	// reg_96[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x96, 0x0);

	// reg_97[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x97, 0x0);

	// reg_98[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x98, 0x0);

	// reg_99[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x99, 0x0);

	// reg_9A[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x9A, 0x0);

	// reg_9B[7:0] = 0xAA
	fmc_hmc_spi_write(fmc_no,  0x9B, 0xAA);

	// reg_9C[7:0] = 0xAA
	fmc_hmc_spi_write(fmc_no,  0x9C, 0xAA);

	// reg_9D[7:0] = 0xAA
	fmc_hmc_spi_write(fmc_no,  0x9D, 0xAA);

	// reg_9E[7:0] = 0xAA
	fmc_hmc_spi_write(fmc_no,  0x9E, 0xAA);

	// reg_9F[7:0] = 0x4D
	fmc_hmc_spi_write(fmc_no,  0x9F, 0x4D);

	// reg_A0[7:0] = 0xDF
	fmc_hmc_spi_write(fmc_no,  0xA0, 0xDF);

	// reg_A1[7:0] = 0x97
	fmc_hmc_spi_write(fmc_no,  0xA1, 0x97);

	// reg_A2[7:0] = 0x3
	fmc_hmc_spi_write(fmc_no,  0xA2, 0x3);

	// reg_A3[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xA3, 0x0);

	// reg_A4[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xA4, 0x0);

	// reg_A5[7:0] = 0x6
	fmc_hmc_spi_write(fmc_no,  0xA5, 0x6);

	// reg_A6[7:0] = 0x1C
	fmc_hmc_spi_write(fmc_no,  0xA6, 0x1C);

	// reg_A7[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xA7, 0x0);

	// reg_A8[7:0] = 0x6
	fmc_hmc_spi_write(fmc_no,  0xA8, 0x6);

	// reg_A9[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xA9, 0x0);

	// reg_AB[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xAB, 0x0);

	// reg_AC[7:0] = 0x20
	fmc_hmc_spi_write(fmc_no,  0xAC, 0x20);

	// reg_AD[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xAD, 0x0);

	// reg_AE[7:0] = 0x8
	fmc_hmc_spi_write(fmc_no,  0xAE, 0x8);

	// reg_AF[7:0] = 0x50
	fmc_hmc_spi_write(fmc_no,  0xAF, 0x50);

	// reg_B0[7:0] = 0x4
	fmc_hmc_spi_write(fmc_no,  0xB0, 0x4);

	// reg_B1[7:0] = 0xD
	fmc_hmc_spi_write(fmc_no,  0xB1, 0xD);

	// reg_B2[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xB2, 0x0);

	// reg_B3[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xB3, 0x0);

	// reg_B5[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xB5, 0x0);

	// reg_B6[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xB6, 0x0);

	// reg_B7[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xB7, 0x0);

	// reg_B8[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xB8, 0x0);
	return  ;
}

void fmc_hmc_program_pll2 (uint32_t fmc_no)
{

	// pll2_reserved[7:0] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x31, 0x1);

	// pll2_cfg1_rpath_x2_bypass[0:0] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x32, 0x1);

	// pll2_rdiv_cfg12_divratio_lsb[7:0] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x33, 0x1);

	// pll2_rdiv_cfg12_divratio_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x34, 0x0);

	// pll2_vdiv_cfg16_divratio_lsb[7:0] = 0x19
	fmc_hmc_spi_write(fmc_no,  0x35, 0x19);

	// pll2_vdiv_cfg16_divratio_msb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x36, 0x0);

	// pll2_cfg4_cp_gain[3:0] = 0xF
	fmc_hmc_spi_write(fmc_no,  0x37, 0xF);

	// pll2_pfd_cfg1_invert[0:0] = 0x0
	// pll2_pfd_cfg1_force_dn[1:1] = 0x0
	// pll2_pfd_cfg1_force_up[2:2] = 0x0
	// pll2_pfd_cfg1_dn_en[3:3] = 0x1
	// pll2_pfd_cfg1_up_en[4:4] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x38, 0x18);

	// pll2_cfg1_oscout_path_en[0:0] = 0x0
	// pll2_cfg2_oscout_divratio[2:1] = 0x0
	//fmc_hmc_spi_write(fmc_no,  0x39, 0x0);
	fmc_hmc_spi_write(fmc_no,  0x39, 0x1);  //  enable Osc path

	// pll2_cfg1_obuf0_drvr_en[0:0] = 0x0
	// pll2_cfg5_obuf0_drvr_res[2:1] = 0x0
	// pll2_cfg5_obuf0_drvr_mode[5:4] = 0x0
	//fmc_hmc_spi_write(fmc_no,  0x3A, 0x0);
	fmc_hmc_spi_write(fmc_no,  0x3A, 0x21);   // Enable LVDS output =SCOUT0

	// pll2_cfg1_obuf1_drvr_en[0:0] = 0x0
	// pll2_cfg5_obuf1_drvr_res[2:1] = 0x0
	// pll2_cfg5_obuf1_drvr_mode[5:4] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x3B, 0x0);

	return  ;
}

void fmc_hmc_program_pll1 (uint32_t fmc_no)
{

	// glbl_cfg5_ibuf0_en[0:0] = 0x0
	// glbl_cfg5_ibuf0_mode[4:1] = 0x3
	fmc_hmc_spi_write(fmc_no,  0xA, 0x6);

	// glbl_cfg5_ibuf1_en[0:0] = 0x0
	// glbl_cfg5_ibuf1_mode[4:1] = 0x3
	fmc_hmc_spi_write(fmc_no,  0xB, 0x6);

	// glbl_cfg5_ibuf2_en[0:0] = 0x0
	// glbl_cfg5_ibuf2_mode[4:1] = 0x3
	fmc_hmc_spi_write(fmc_no,  0xC, 0x6);

	// glbl_cfg5_ibuf3_en[0:0] = 0x1
	// glbl_cfg5_ibuf3_mode[4:1] = 0x3
	fmc_hmc_spi_write(fmc_no,  0xD, 0x7);	// clkin3 enable
	//fmc_hmc_spi_write(fmc_no,  0xD, 0x6); clkin3 disable



	// glbl_cfg5_ibufv_en[0:0] = 0x1
	// glbl_cfg5_ibufv_mode[4:1] = 0x3
	fmc_hmc_spi_write(fmc_no,  0xE, 0x7);
	//fmc_hmc_spi_write(fmc_no,  0xE, 0x13);    // th H-Imp


	// pll1_cfg2_rprior1[1:0] = 0x3
	// pll1_cfg2_rprior2[3:2] = 0x2
	// pll1_cfg2_rprior3[5:4] = 0x1
	// pll1_cfg2_rprior4[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x14, 0x1B);

	// pll1_cfg3_los_valtime_sel[2:0] = 0x3
	fmc_hmc_spi_write(fmc_no,  0x15, 0x3);

	// pll1_cfg2_holdover_exitcrit[1:0] = 0x0
	// pll1_cfg2_holdover_exitactn[3:2] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x16, 0x4);

	// pll1_cfg7_hodac_offsetval[6:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x17, 0x0);

	// pll1_cfg2_hoadc_bw_reduction[1:0] = 0x0
	// pll1_cfg1_hodac_force_quickmode[2:2] = 0x1
	// pll1_cfg1_hodac_dis_avg_track[3:3] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x18, 0x4);

	// pll1_cfg1_los_uses_vcxodiv[0:0] = 0x0
	// pll1_cfg1_los_bypass_lcmdiv[1:1] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x19, 0x0);

	// pll1_cfg4_cpi[3:0] = 0x6
	fmc_hmc_spi_write(fmc_no,  0x1A, 0x6);

	// pll1_cfg1_pfd_invert[0:0] = 0x0
	// pll1_cfg1_cppulldn[1:1] = 0x0
	// pll1_cfg1_cppullup[2:2] = 0x0
	// pll1_cfg1_cpendn[3:3] = 0x1
	// pll1_cfg1_cpenup[4:4] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x1B, 0x18);

	// pll1_cfg8_los_div_setpt_r0[7:0] = 0x4
	fmc_hmc_spi_write(fmc_no,  0x1C, 0x4);

	// pll1_cfg8_los_div_setpt_r1[7:0] = 0x4
	fmc_hmc_spi_write(fmc_no,  0x1D, 0x4);

	// pll1_cfg8_los_div_setpt_r2[7:0] = 0x4
	fmc_hmc_spi_write(fmc_no,  0x1E, 0x4);

	// pll1_cfg8_los_div_setpt_r3[7:0] = 0x4
	fmc_hmc_spi_write(fmc_no,  0x1F, 0x4);

	// pll1_cfg8_los_div_setpt_vcxo[7:0] = 0x4
	fmc_hmc_spi_write(fmc_no,  0x20, 0x4);

	// pll1_cfg16_refdivrat_lsb[7:0] = 0x4
	fmc_hmc_spi_write(fmc_no,  0x21, 0x4);

	// pll1_cfg16_refdivrat_msb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x22, 0x0);

	// pll1_cfg16_fbdivrat_lsb[7:0] = 0x4
	//fmc_hmc_spi_write(fmc_no,  0x26, 0x4);
	fmc_hmc_spi_write(fmc_no,  0x26, 0x10);                                    // th set to 16

	// pll1_cfg16_fbdivrat_msb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x27, 0x0);

	// pll1_cfg5_lkdtimersetpt[4:0] = 0xF
	// pll1_cfg1_use_slip_for_lkdrst[5:5] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x28, 0xF);

	// pll1_cfg1_automode[0:0] = 0x0
	// pll1_cfg1_autorevertive[1:1] = 0x0
	// pll1_cfg1_holdover_uses_dac[2:2] = 0x1
	// pll1_cfg2_manclksel[4:3] = 0x3
	// pll1_cfg1_byp_debouncer[5:5] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x29, 0x1C);
	//fmc_hmc_spi_write(fmc_no,  0x29, 0x05);          // th set to 05

	// pll1_hoff_timer_setpoint[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x2A, 0x0);

	return  ;
}

void fmc_hmc_program_gpio (uint32_t fmc_no)
{
	// glbl_cfg5_gpi1_en[0:0] = 0x0
	// glbl_cfg5_gpi1_sel[4:1] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x46, 0x0);

	// glbl_cfg5_gpi2_en[0:0] = 0x0
	// glbl_cfg5_gpi2_sel[4:1] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x47, 0x0);

	// glbl_cfg5_gpi3_en[0:0] = 0x0
	// glbl_cfg5_gpi3_sel[4:1] = 0x4
	fmc_hmc_spi_write(fmc_no,  0x48, 0x8);

	// glbl_cfg5_gpi4_en[0:0] = 0x0
	// glbl_cfg5_gpi4_sel[4:1] = 0x8
	fmc_hmc_spi_write(fmc_no,  0x49, 0x10);

	// glbl_cfg8_gpo1_en[0:0] = 0x1
	// glbl_cfg8_gpo1_mode[1:1] = 0x1
	// glbl_cfg8_gpo1_sel[7:2] = 0x7
	fmc_hmc_spi_write(fmc_no,  0x50, 0x1F);

	// glbl_cfg8_gpo2_en[0:0] = 0x1
	// glbl_cfg8_gpo2_mode[1:1] = 0x1
	// glbl_cfg8_gpo2_sel[7:2] = 0xA
	fmc_hmc_spi_write(fmc_no,  0x51, 0x2B);

	// glbl_cfg8_gpo3_en[0:0] = 0x1
	// glbl_cfg8_gpo3_mode[1:1] = 0x1
	// glbl_cfg8_gpo3_sel[7:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x52, 0x3);

	// glbl_cfg8_gpo4_en[0:0] = 0x1
	// glbl_cfg8_gpo4_mode[1:1] = 0x1
	// glbl_cfg8_gpo4_sel[7:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x53, 0x3);

	// glbl_cfg2_sdio_en[0:0] = 0x1
	// glbl_cfg2_sdio_mode[1:1] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x54, 0x3);

	return  ;
}

void fmc_hmc_program_sysref_sync(uint32_t fmc_no)
{
// sysr_cfg3_pulsor_mode[2:0] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x5A, 0x1);

	// sysr_cfg1_synci_invpol[0:0] = 0x0
	// sysr_cfg1_pll2_carryup_sel[1:1] = 0x1
	// sysr_cfg1_ext_sync_retimemode[2:2] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x5B, 0x6);

	// sysr_cfg16_divrat_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x5C, 0x0);

	// sysr_cfg16_divrat_msb[3:0] = 0x6
	fmc_hmc_spi_write(fmc_no,  0x5D, 0x6);

	return  ;
}

void fmc_hmc_program_clk_distr_network(uint32_t fmc_no)
{
	// dist_cfg1_extvco_islowfreq_sel[0:0] = 0x0
	// dist_cfg1_extvco_div2_sel[1:1] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x64, 0x0);

	// clkgrpx_cfg1_alg_dly_lowpwr_sel[0:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x65, 0x0);


	return  ;
}

void fmc_hmc_program_alarm_mask(uint32_t fmc_no)
{
	// alrm_cfg4_pll1_los4_allow[3:0] = 0x0
	// alrm_cfg1_pll1_hold_allow[4:4] = 0x0
	// alrm_cfg1_pll1_lock_allow[5:5] = 0x0
	// alrm_cfg1_pll1_acq_allow[6:6] = 0x0
	// alrm_cfg1_pll1_nearlock_allow[7:7] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x70, 0x0);

	// alrm_cfg1_pll2_lock_allow[0:0] = 0x0
	// alrm_cfg1_sysr_unsyncd_allow[1:1] = 0x0
	// alrm_cfg1_clkgrpx_validph_allow[2:2] = 0x0
	// alrm_cfg1_plls_both_locked_allow[3:3] = 0x0
	// alrm_cfg1_sync_req_allow[4:4] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x71, 0x10);

	return  ;
}

void fmc_hmc_program_channel_outputs (uint32_t fmc_no)
{

// group1 - out0  ->  Out 0: ADC Clock 2.5GHz
	// clkgrp1_div1_cfg1_en[0:0] = 0x0
	// clkgrp1_div1_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp1_div1_cfg2_startmode[3:2] = 0x0
	// clkgrp1_div1_cfg1_rev[4:4] = 0x1
	// clkgrp1_div1_cfg1_slipmask[5:5] = 0x0
	// clkgrp1_div1_cfg1_reseedmask[6:6] = 0x1
	// clkgrp1_div1_cfg1_hi_perf[7:7] = 0x0
	//fmc_hmc_spi_write(fmc_no,  0xC8, 0x51);
	//fmc_hmc_spi_write(fmc_no,  0xC8, 0x91);      // High Performance, reserved, Enable
	fmc_hmc_spi_write(fmc_no,  0xC8, 0xd1);      // High Performance, Synch reserved, Enable

	// clkgrp1_div1_cfg12_divrat_lsb[7:0] = 0x3
	fmc_hmc_spi_write(fmc_no,  0xC9, 1);         // 2.5 Gbit

	// clkgrp1_div1_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xCA, 0x0);

	// clkgrp1_div1_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xCB, 0x0);

	// clkgrp1_div1_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xCC, 0x0);

	// clkgrp1_div1_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xCD, 0x0);

	// clkgrp1_div1_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xCE, 0x0);

	// clkgrp1_div1_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp1_div1_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xCF, 0x0);

	// clkgrp1_div1_cfg5_drvr_res[1:0] = 0x0
	// clkgrp1_div1_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp1_div1_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp1_div1_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp1_div1_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xD0, 0x01); // CML and 100 Ohm enable


// group1 - out1  ->  Out 1: ADC SYSREF 2.5GHz/256
	// clkgrp1_div2_cfg1_en[0:0] = 0x0
	// clkgrp1_div2_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp1_div2_cfg2_startmode[3:2] = 0x0
	// clkgrp1_div2_cfg1_rev[4:4] = 0x1
	// clkgrp1_div2_cfg1_slipmask[5:5] = 0x0
	// clkgrp1_div2_cfg1_reseedmask[6:6] = 0x1
	// clkgrp1_div2_cfg1_hi_perf[7:7] = 0x0
	//fmc_hmc_spi_write(fmc_no,  0xD2, 0x51);
	//fmc_hmc_spi_write(fmc_no,  0xD2, 0x91);   // High Performance, reserved, Enable
	fmc_hmc_spi_write(fmc_no,  0xD2, 0xd1);      // High Performance, Synch reserved, Enable

	// clkgrp1_div2_cfg12_divrat_lsb[7:0] = 0x3
	fmc_hmc_spi_write(fmc_no,  0xD3, 0x0);

	// clkgrp1_div2_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xD4, 0x1);
	//fmc_hmc_spi_write(fmc_no,  0xD4, 0x0);

	// clkgrp1_div2_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xD5, 0x0);

	// clkgrp1_div2_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xD6, 0x0);

	// clkgrp1_div2_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xD7, 0x0);

	// clkgrp1_div2_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xD8, 0x0);

	// clkgrp1_div2_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp1_div2_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xD9, 0x0);

	// clkgrp1_div2_cfg5_drvr_res[1:0] = 0x1
	// clkgrp1_div2_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp1_div2_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp1_div2_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp1_div2_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xDA, 0x10);   // LVDS and and internal 100 Ohm disable



// group2 - out2   ->  2.5GHz/50 (test)
	// clkgrp2_div1_cfg1_en[0:0] = 0x1
	// clkgrp2_div1_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp2_div1_cfg2_startmode[3:2] = 0x0
	// clkgrp2_div1_cfg1_rev[4:4] = 0x1
	// clkgrp2_div1_cfg1_slipmask[5:5] = 0x0
	// clkgrp2_div1_cfg1_reseedmask[6:6] = 0x0
	// clkgrp2_div1_cfg1_hi_perf[7:7] = 0x1
	//fmc_hmc_spi_write(fmc_no,  0xDC, 0x51);
	//fmc_hmc_spi_write(fmc_no,  0xDC, 0x91);   // High Performance, reserved, Enable
	fmc_hmc_spi_write(fmc_no,  0xDC, 0xd1);      // High Performance, Synch reserved, Enable

	// clkgrp2_div1_cfg12_divrat_lsb[7:0] = 0x32
	fmc_hmc_spi_write(fmc_no,  0xDD, 0x32);

	// clkgrp2_div1_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xDE, 0x0);

	// clkgrp2_div1_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xDF, 0x0);

	// clkgrp2_div1_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xE0, 0x0);

	// clkgrp2_div1_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xE1, 0x0);

	// clkgrp2_div1_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xE2, 0x0);

	// clkgrp2_div1_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp2_div1_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xE3, 0x0);

	// clkgrp2_div1_cfg5_drvr_res[1:0] = 0x0
	// clkgrp2_div1_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp2_div1_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp2_div1_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp2_div1_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xE4, 0x10);   // LVDS and and internal 100 Ohm disable


// group2 - out3   ->   Out 3: 2.5GHz/8(test)
	// clkgrp2_div2_cfg1_en[0:0] = 0x1
	// clkgrp2_div2_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp2_div2_cfg2_startmode[3:2] = 0x0
	// clkgrp2_div2_cfg1_rev[4:4] = 0x1
	// clkgrp2_div2_cfg1_slipmask[5:5] = 0x0
	// clkgrp2_div2_cfg1_reseedmask[6:6] = 0x0
	// clkgrp2_div2_cfg1_hi_perf[7:7] = 0x1
	//fmc_hmc_spi_write(fmc_no,  0xE6, 0x51);
	//fmc_hmc_spi_write(fmc_no,  0xE6, 0x91);   // High Performance, reserved, Enable
	//fmc_hmc_spi_write(fmc_no,  0xE6, 0xd1);      // High Performance, Synch reserved, Enable
	fmc_hmc_spi_write(fmc_no,  0xE6, 0xd0);      // Disable

	// clkgrp2_div2_cfg12_divrat_lsb[7:0] = 0xA
	fmc_hmc_spi_write(fmc_no,  0xE7, 8);

	// clkgrp2_div2_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xE8, 0x0);

	// clkgrp2_div2_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xE9, 0x0);

	// clkgrp2_div2_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xEA, 0x0);

	// clkgrp2_div2_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xEB, 0x0);

	// clkgrp2_div2_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xEC, 0x0);

	// clkgrp2_div2_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp2_div2_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xED, 0x0);

	// clkgrp2_div2_cfg5_drvr_res[1:0] = 0x1
	// clkgrp2_div2_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp2_div2_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp2_div2_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp2_div2_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xEE, 0x10);   // LVDS and and internal 100 Ohm disable





// group3 - out4      ->  Out 4: 2.5GHz/4 = 625 MHz MGT Clock
	// clkgrp3_div1_cfg1_en[0:0] = 0x0
	// clkgrp3_div1_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp3_div1_cfg2_startmode[3:2] = 0x0
	// clkgrp3_div1_cfg1_rev[4:4] = 0x1
	// clkgrp3_div1_cfg1_slipmask[5:5] = 0x0
	// clkgrp3_div1_cfg1_reseedmask[6:6] = 0x1
	// clkgrp3_div1_cfg1_hi_perf[7:7] = 0x0
	//fmc_hmc_spi_write(fmc_no,  0xF0, 0x51);
	//fmc_hmc_spi_write(fmc_no,  0xF0, 0x91);   // High Performance, reserved, Enable
	fmc_hmc_spi_write(fmc_no,  0xF0, 0xd1);      // High Performance, Synch reserved, Enable


	// clkgrp3_div1_cfg12_divrat_lsb[7:0] = 0x2
	//fmc_hmc_spi_write(fmc_no,  0xF1, 0x8); // 312.5 MHz
	fmc_hmc_spi_write(fmc_no,  0xF1, 0x4);   // 625 MHz

	// clkgrp3_div1_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xF2, 0x0);

	// clkgrp3_div1_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xF3, 0x0);

	// clkgrp3_div1_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xF4, 0x0);

	// clkgrp3_div1_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xF5, 0x0);

	// clkgrp3_div1_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xF6, 0x0);

	// clkgrp3_div1_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp3_div1_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xF7, 0x0);

	// clkgrp3_div1_cfg5_drvr_res[1:0] = 0x0
	// clkgrp3_div1_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp3_div1_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp3_div1_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp3_div1_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xF8, 0x10);   // LVDS and and internal 100 Ohm disable


// group3 - out5     ->     Out 5: disabled
	// clkgrp3_div2_cfg1_en[0:0] = 0x0
	// clkgrp3_div2_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp3_div2_cfg2_startmode[3:2] = 0x0
	// clkgrp3_div2_cfg1_rev[4:4] = 0x1
	// clkgrp3_div2_cfg1_slipmask[5:5] = 0x0
	// clkgrp3_div2_cfg1_reseedmask[6:6] = 0x1
	// clkgrp3_div2_cfg1_hi_perf[7:7] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xFA, 0x50); // Disable

	// clkgrp3_div2_cfg12_divrat_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xFB, 0x0);

	// clkgrp3_div2_cfg12_divrat_msb[3:0] = 0x1
	fmc_hmc_spi_write(fmc_no,  0xFC, 0x1);

	// clkgrp3_div2_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xFD, 0x0);

	// clkgrp3_div2_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xFE, 0x0);

	// clkgrp3_div2_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0xFF, 0x0);

	// clkgrp3_div2_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x100, 0x0);

	// clkgrp3_div2_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp3_div2_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x101, 0x0);

	// clkgrp3_div2_cfg5_drvr_res[1:0] = 0x3
	// clkgrp3_div2_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp3_div2_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp3_div2_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp3_div2_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x102, 0x10);   // LVDS and and internal 100 Ohm disable




// group4 - out6 ->  Out 6: disabled
	// clkgrp4_div1_cfg1_en[0:0] = 0x0
	// clkgrp4_div1_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp4_div1_cfg2_startmode[3:2] = 0x0
	// clkgrp4_div1_cfg1_rev[4:4] = 0x1
	// clkgrp4_div1_cfg1_slipmask[5:5] = 0x0
	// clkgrp4_div1_cfg1_reseedmask[6:6] = 0x1
	// clkgrp4_div1_cfg1_hi_perf[7:7] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x104, 0x50); // Disable

	// clkgrp4_div1_cfg12_divrat_lsb[7:0] = 0x2
	fmc_hmc_spi_write(fmc_no,  0x105, 0x2);

	// clkgrp4_div1_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x106, 0x0);

	// clkgrp4_div1_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x107, 0x0);

	// clkgrp4_div1_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x108, 0x0);

	// clkgrp4_div1_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x109, 0x0);

	// clkgrp4_div1_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x10A, 0x0);

	// clkgrp4_div1_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp4_div1_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x10B, 0x0);

	// clkgrp4_div1_cfg5_drvr_res[1:0] = 0x0
	// clkgrp4_div1_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp4_div1_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp4_div1_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp4_div1_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x10C, 0x10);   // LVDS and and internal 100 Ohm disable


// group4 - out7 ->  Out 7: disabled
	// clkgrp4_div2_cfg1_en[0:0] = 0x0
	// clkgrp4_div2_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp4_div2_cfg2_startmode[3:2] = 0x0
	// clkgrp4_div2_cfg1_rev[4:4] = 0x1
	// clkgrp4_div2_cfg1_slipmask[5:5] = 0x0
	// clkgrp4_div2_cfg1_reseedmask[6:6] = 0x1
	// clkgrp4_div2_cfg1_hi_perf[7:7] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x10E, 0x50); // Disable

	// clkgrp4_div2_cfg12_divrat_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x10F, 0x0);

	// clkgrp4_div2_cfg12_divrat_msb[3:0] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x110, 0x1);

	// clkgrp4_div2_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x111, 0x0);

	// clkgrp4_div2_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x112, 0x0);

	// clkgrp4_div2_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x113, 0x0);

	// clkgrp4_div2_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x114, 0x0);

	// clkgrp4_div2_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp4_div2_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x115, 0x0);

	// clkgrp4_div2_cfg5_drvr_res[1:0] = 0x3
	// clkgrp4_div2_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp4_div2_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp4_div2_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp4_div2_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x116, 0x10);   // LVDS and and internal 100 Ohm disable




// group5 - out8  ->  Out 8: 2.5GHz/256 SYSREF0
	// clkgrp5_div1_cfg1_en[0:0] = 0x0
	// clkgrp5_div1_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp5_div1_cfg2_startmode[3:2] = 0x0
	// clkgrp5_div1_cfg1_rev[4:4] = 0x1
	// clkgrp5_div1_cfg1_slipmask[5:5] = 0x0
	// clkgrp5_div1_cfg1_reseedmask[6:6] = 0x1
	// clkgrp5_div1_cfg1_hi_perf[7:7] = 0x0
	//fmc_hmc_spi_write(fmc_no,  0x118, 0x91);// High Performance, reserved, Enable
	fmc_hmc_spi_write(fmc_no,  0x118, 0xd1);      // High Performance, Synch reserved, Enable

	// clkgrp5_div1_cfg12_divrat_lsb[7:0] = 0x2
	fmc_hmc_spi_write(fmc_no,  0x119, 0x0);

	// clkgrp5_div1_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x11A, 0x1);

	// clkgrp5_div1_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x11B, 0x0);

	// clkgrp5_div1_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x11C, 0x0);

	// clkgrp5_div1_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x11D, 0x0);

	// clkgrp5_div1_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x11E, 0x0);

	// clkgrp5_div1_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp5_div1_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x11F, 0x0);

	// clkgrp5_div1_cfg5_drvr_res[1:0] = 0x0
	// clkgrp5_div1_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp5_div1_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp5_div1_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp5_div1_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x120, 0x10);   // LVDS and and internal 100 Ohm disable


// group5 - out9 ->  Out 1: 2.5GHz/25 (test)
	// clkgrp5_div2_cfg1_en[0:0] = 0x1
	// clkgrp5_div2_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp5_div2_cfg2_startmode[3:2] = 0x0
	// clkgrp5_div2_cfg1_rev[4:4] = 0x1
	// clkgrp5_div2_cfg1_slipmask[5:5] = 0x0
	// clkgrp5_div2_cfg1_reseedmask[6:6] = 0x0
	// clkgrp5_div2_cfg1_hi_perf[7:7] = 0x1
	//fmc_hmc_spi_write(fmc_no,  0x122, 0x91);
	fmc_hmc_spi_write(fmc_no,  0x122, 0x50); // Disable


	// clkgrp5_div2_cfg12_divrat_lsb[7:0] = 0x19
	fmc_hmc_spi_write(fmc_no,  0x123, 0x19);

	// clkgrp5_div2_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x124, 0x0);

	// clkgrp5_div2_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x125, 0x0);

	// clkgrp5_div2_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x126, 0x0);

	// clkgrp5_div2_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x127, 0x0);

	// clkgrp5_div2_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x128, 0x0);

	// clkgrp5_div2_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp5_div2_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x129, 0x0);

	// clkgrp5_div2_cfg5_drvr_res[1:0] = 0x3
	// clkgrp5_div2_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp5_div2_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp5_div2_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp5_div2_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x12A, 0x10);   // LVDS and and internal 100 Ohm disable



// group6 - out10 ->  Out 10: 2.5GHz/8 = 312.5 MHz Coreclock 0
	// clkgrp6_div1_cfg1_en[0:0] = 0x0
	// clkgrp6_div1_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp6_div1_cfg2_startmode[3:2] = 0x0
	// clkgrp6_div1_cfg1_rev[4:4] = 0x1
	// clkgrp6_div1_cfg1_slipmask[5:5] = 0x0
	// clkgrp6_div1_cfg1_reseedmask[6:6] = 0x1
	// clkgrp6_div1_cfg1_hi_perf[7:7] = 0x0
	//fmc_hmc_spi_write(fmc_no,  0x12C, 0x51);
	//fmc_hmc_spi_write(fmc_no,  0x12C, 0x91);   // High Performance, reserved, Enable
	fmc_hmc_spi_write(fmc_no,  0x12C, 0xd1);      // High Performance, Synch reserved, Enable

	// clkgrp6_div1_cfg12_divrat_lsb[7:0] = 0x8
	fmc_hmc_spi_write(fmc_no,  0x12D, 0x8); // 312.5 MHz
	//fmc_hmc_spi_write(fmc_no,  0x12D, 0x10); // 156.25 MHz

	// clkgrp6_div1_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x12E, 0x0);

	// clkgrp6_div1_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x12F, 0x0);

	// clkgrp6_div1_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x130, 0x0);

	// clkgrp6_div1_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x131, 0x0);

	// clkgrp6_div1_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x132, 0x0);

	// clkgrp6_div1_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp6_div1_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x133, 0x0);

	// clkgrp6_div1_cfg5_drvr_res[1:0] = 0x0
	// clkgrp6_div1_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp6_div1_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp6_div1_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp6_div1_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x134, 0x10);   // LVDS and and internal 100 Ohm disable



// group6 - 11 ->  Out 11: 2.5GHz/256 SYSREF0
	// clkgrp6_div2_cfg1_en[0:0] = 0x1
	// clkgrp6_div2_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp6_div2_cfg2_startmode[3:2] = 0x0
	// clkgrp6_div2_cfg1_rev[4:4] = 0x1
	// clkgrp6_div2_cfg1_slipmask[5:5] = 0x0
	// clkgrp6_div2_cfg1_reseedmask[6:6] = 0x0
	// clkgrp6_div2_cfg1_hi_perf[7:7] = 0x1
	//fmc_hmc_spi_write(fmc_no,  0x136, 0x51);
	//fmc_hmc_spi_write(fmc_no,  0x136, 0x91);   // High Performance, reserved, Enable
	fmc_hmc_spi_write(fmc_no,  0x136, 0x50); // Disable

	// clkgrp6_div2_cfg12_divrat_lsb[7:0] = 0x5
	fmc_hmc_spi_write(fmc_no,  0x137, 0x0);

	// clkgrp6_div2_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x138, 0x1);

	// clkgrp6_div2_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x139, 0x0);

	// clkgrp6_div2_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x13A, 0x0);

	// clkgrp6_div2_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x13B, 0x0);

	// clkgrp6_div2_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x13C, 0x0);

	// clkgrp6_div2_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp6_div2_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x13D, 0x0);

	// clkgrp6_div2_cfg5_drvr_res[1:0] = 0x1
	// clkgrp6_div2_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp6_div2_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp6_div2_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp6_div2_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x13E, 0x10);   // LVDS and and internal 100 Ohm disable



// group7 - out12/13 ->  Out 12: disabled   Out 13: disabled
	// clkgrp7_div1_cfg1_en[0:0] = 0x0
	// clkgrp7_div1_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp7_div1_cfg2_startmode[3:2] = 0x0
	// clkgrp7_div1_cfg1_rev[4:4] = 0x1
	// clkgrp7_div1_cfg1_slipmask[5:5] = 0x0
	// clkgrp7_div1_cfg1_reseedmask[6:6] = 0x1
	// clkgrp7_div1_cfg1_hi_perf[7:7] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x140, 0x50);// Disable

	// clkgrp7_div1_cfg12_divrat_lsb[7:0] = 0x2
	fmc_hmc_spi_write(fmc_no,  0x141, 0x2);

	// clkgrp7_div1_cfg12_divrat_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x142, 0x0);

	// clkgrp7_div1_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x143, 0x0);

	// clkgrp7_div1_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x144, 0x0);

	// clkgrp7_div1_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x145, 0x0);

	// clkgrp7_div1_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x146, 0x0);

	// clkgrp7_div1_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp7_div1_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x147, 0x0);

	// clkgrp7_div1_cfg5_drvr_res[1:0] = 0x0
	// clkgrp7_div1_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp7_div1_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp7_div1_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp7_div1_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x148, 0x10);   // LVDS and and internal 100 Ohm disable

	// clkgrp7_div2_cfg1_en[0:0] = 0x0
	// clkgrp7_div2_cfg1_phdelta_mslip[1:1] = 0x0
	// clkgrp7_div2_cfg2_startmode[3:2] = 0x0
	// clkgrp7_div2_cfg1_rev[4:4] = 0x1
	// clkgrp7_div2_cfg1_slipmask[5:5] = 0x1
	// clkgrp7_div2_cfg1_reseedmask[6:6] = 0x1
	// clkgrp7_div2_cfg1_hi_perf[7:7] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x14A, 0x50); // Disable

	// clkgrp7_div2_cfg12_divrat_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x14B, 0x0);

	// clkgrp7_div2_cfg12_divrat_msb[3:0] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x14C, 0x1);

	// clkgrp7_div2_cfg5_fine_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x14D, 0x0);

	// clkgrp7_div2_cfg5_sel_coarse_delay[4:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x14E, 0x0);

	// clkgrp7_div2_cfg12_mslip_lsb[7:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x14F, 0x0);

	// clkgrp7_div2_cfg12_mslip_msb[3:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x150, 0x0);

	// clkgrp7_div2_cfg2_sel_outmux[1:0] = 0x0
	// clkgrp7_div2_cfg1_drvr_sel_testclk[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x151, 0x0);

	// clkgrp7_div2_cfg5_drvr_res[1:0] = 0x3
	// clkgrp7_div2_cfg5_drvr_spare[2:2] = 0x0
	// clkgrp7_div2_cfg5_drvr_mode[4:3] = 0x2
	// clkgrp7_div2_cfg_outbuf_dyn[5:5] = 0x0
	// clkgrp7_div2_cfg2_mutesel[7:6] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x152, 0x10);   // LVDS and and internal 100 Ohm disable


	return  ;
}

void fmc_hmc_program_global_control (uint32_t fmc_no)
{
	// sysr_cfg1_rev[0:0] = 0x0
	// sysr_cfg1_slipN_req[1:1] = 0x0
	// pll2_cfg1_autotune_trig[2:2] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x2, 0x0);

	// glbl_cfg1_ena_pll1[0:0] = 0x1
	// glbl_cfg1_ena_pll2[1:1] = 0x1
	// glbl_cfg1_ena_sysr[2:2] = 0x0
	// glbl_cfg2_ena_vcos[4:3] = 0x2
	// glbl_cfg1_ena_sysri[5:5] = 0x0
	//fmc_hmc_spi_write(fmc_no,  0x3, 0x13);
	fmc_hmc_spi_write(fmc_no,  0x3, 0x37); // RF reseeder , VCO selection low, Enable internal Sysref timer, Enable PLL1 PLL2

	// glbl_cfg7_ena_clkgr[6:0] = 0x32
	fmc_hmc_spi_write(fmc_no,  0x4, 0x3f);

	// glbl_cfg4_ena_rpath[3:0] = 0x8
	// dist_cfg1_refbuf0_as_rfsync[4:4] = 0x0
	// dist_cfg1_refbuf1_as_extvco[5:5] = 0x0
	// pll2_cfg2_syncpin_modesel[7:6] = 0x0
	//fmc_hmc_spi_write(fmc_no,  0x5, 0x2); // clkIN1
	//fmc_hmc_spi_write(fmc_no,  0x5, 0x8); // clkin3
	fmc_hmc_spi_write(fmc_no,  0x5, 0x48); // use SYNC pin, clkin3

	// glbl_cfg1_clear_alarms[0:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x6, 0x0);

	// glbl_reserved[0:0] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x7, 0x0);

	// glbl_cfg1_dis_pll2_syncatlock[0:0] = 0x1
	fmc_hmc_spi_write(fmc_no,  0x9, 0x1);


	return  ;
}

void fmc_hmc_global_control_restart_cmd (uint32_t fmc_no)
{

	// glbl_cfg1_sleep[0:0] = 0x0
	// glbl_cfg1_restart[1:1] = 0x0
	// sysr_cfg1_pulsor_req[2:2] = 0x0
	// grpx_cfg1_mute[3:3] = 0x0
	// pll1_cfg1_forceholdover[4:4] = 0x0
	// glbl_cfg1_perf_pllvco[5:5] = 0x0
	// dist_cfg1_perf_floor[6:6] = 0x0
	// sysr_cfg1_reseed_req[7:7] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x1, 0x2);
	usleep(100) ;
 	fmc_hmc_spi_write(fmc_no,  0x1, 0x0);
	return  ;
}

//*************************************************************************************************************************************************

void fmc_hmc_global_control_reseed_cmd (uint32_t fmc_no)
{

	// glbl_cfg1_sleep[0:0] = 0x0
	// glbl_cfg1_restart[1:1] = 0x0
	// sysr_cfg1_pulsor_req[2:2] = 0x0
	// grpx_cfg1_mute[3:3] = 0x0
	// pll1_cfg1_forceholdover[4:4] = 0x0
	// glbl_cfg1_perf_pllvco[5:5] = 0x0
	// dist_cfg1_perf_floor[6:6] = 0x0
	// sysr_cfg1_reseed_req[7:7] = 0x0
	fmc_hmc_spi_write(fmc_no,  0x1, 0x80);
	usleep(100) ;
 	fmc_hmc_spi_write(fmc_no,  0x1, 0x0);

	return  ;
}


void fmc_hmc_set_hardware_reset_cmd(uint32_t fmc_no)
{
	device_rw	  l_RW;
	uint32_t addr ;
	 if (fmc_no == 0) {
		addr = SIS8160_FMC1_SPACE_REG*4 + SFMC_CONTROL2_REG*4 ;
	}
	else {
		addr = SIS8160_FMC2_SPACE_REG*4 + SFMC_CONTROL2_REG*4 ;
	}
	l_RW.data_rw   = 0x10000;
	l_RW.offset_rw = addr;
	l_RW.mode_rw   = 2;
	l_RW.barx_rw   = 0;
	l_RW.size_rw   = 0;
	write (fd, &l_RW, sizeof(device_rw));
	usleep(100) ;
	return  ;
}

void fmc_hmc_release_hardware_reset_cmd(uint32_t fmc_no)
{
    device_rw	  l_RW;
	uint32_t addr ;
	 if (fmc_no == 0) {
		addr = SIS8160_FMC1_SPACE_REG*4 + SFMC_CONTROL2_REG*4 ;
	}
	else {
		addr = SIS8160_FMC2_SPACE_REG*4 + SFMC_CONTROL2_REG*4 ;
	}
	l_RW.data_rw   = 0x1;
	l_RW.offset_rw = addr;
	l_RW.mode_rw   = 2;
	l_RW.barx_rw   = 0;
	l_RW.size_rw   = 0;
	write (fd, &l_RW, sizeof(device_rw));
	usleep(100) ;
	return  ;
}

void sfmc01_adc_setup ( uint32_t fmc_no, uint8_t coupling)
{

        fmc_adc_spi_write(fmc_no,  0x000, 0x81); // softreset
        usleep(10) ;  // wait 10us
       //usleep(10000) ;  // wait 10ms
        fmc_adc_spi_write(fmc_no,  0x001, 0x02); // data path reset
        //usleep(10000) ;  // wait 10ms
        usleep(10) ;  // wait 10us

        fmc_adc_spi_write(fmc_no,  0x200, 0x00);  // full bandwith mode

#if 1
        switch(coupling)
        {
        case 1: // Enable AC coupling
            fmc_adc_spi_write(fmc_no,  0x1908, 0x00);  // Register default values
            fmc_adc_spi_write(fmc_no,  0x18A6, 0x00);  //
            fmc_adc_spi_write(fmc_no,  0x18E6, 0x00);  //
            fmc_adc_spi_write(fmc_no,  0x18E0, 0x00);  //
            fmc_adc_spi_write(fmc_no,  0x18E1, 0x00);  //
            fmc_adc_spi_write(fmc_no,  0x18E2, 0x00);  //
            fmc_adc_spi_write(fmc_no,  0x18E3, 0x00);  //
 //           fmc_adc_spi_write(fmc_no,  0x0701, 0x86);  // Enable DC offset calibration
            break;

        default:
            // Enable DC coupling (See ADC manual of AD9689: 'Input Common Mode')
            fmc_adc_spi_write(fmc_no,  0x1908, 0x04);  // Disconnect the internal common-mode buffer from the analog input
            fmc_adc_spi_write(fmc_no,  0x18A6, 0x00);  // Turn off the voltage reference (Internal referemce)
            fmc_adc_spi_write(fmc_no,  0x18E6, 0x00);  // Turn off temperature diode

            fmc_adc_spi_write(fmc_no,  0x18E0, 0x03);  // VCM replica generator control
            fmc_adc_spi_write(fmc_no,  0x18E1, 0x92);  //
            fmc_adc_spi_write(fmc_no,  0x18E2, 0x1E);  //

            fmc_adc_spi_write(fmc_no,  0x18E3, 0x4F);  // Turn on the VCM export
            fmc_adc_spi_write(fmc_no,  0x0701, 0x06);  // Disable DC offset calibration
            break;
        }
#else
#if 1
        fmc_adc_spi_write(fmc_no,  0x1908, 0x04);  // bit 2: Enable DC coupling
        fmc_adc_spi_write(fmc_no,  0x18A6, 0x00);  // VREF: internal Reference
        fmc_adc_spi_write(fmc_no,  0x18E6, 0x00);  // turn off temperature diode
        fmc_adc_spi_write(fmc_no,  0x18E3, 0x4F);  // bit 6: 1 VCM export and
#endif
#endif

        fmc_adc_spi_write(fmc_no,  0x571, 0x15);  // JESD204B link power down

	    //fmc_adc_spi_write(fmc_no,  0x120, 0x08);  //  SYSREF capture on falling edge  of clk
	    //fmc_adc_spi_write(fmc_no,  0x120, 0x0A);  //  SYSREF clk negative edge, SYSREF mode=Continues

        fmc_adc_spi_write(fmc_no,  0x120, 0x2);  // SYSREF mode: Continous

        fmc_adc_spi_write(fmc_no,  0x590, 0x2f);  // subclass 1, N=16
	    //fmc_adc_spi_write(fmc_no,  0x590, 0x0f);  // subclass 0


	    fmc_adc_spi_write(fmc_no,  0x58B, 0x87);  // scrambling enabled, L=8
	    //fmc_adc_spi_write(fmc_no,  0x58B, 0x07);  // scrambling disabled, L=8 ,8 lanes per link

	    fmc_adc_spi_write(fmc_no,  0x58E, 0x01);  // M=2
	    fmc_adc_spi_write(fmc_no,  0x58C, 0x01);  // F=2
	    //fmc_adc_spi_write(fmc_no,  0x58C, 0x00);  // F=1
	    fmc_adc_spi_write(fmc_no,  0x58F, 0x0F);  // CS=0;  N=16 bit resolution

	    fmc_adc_spi_write(fmc_no,  0x201, 0x00);  // chip decimation rate = 1
	    fmc_adc_spi_write(fmc_no,  0x56E, 0x00);  // lane rate = 6.75 Gbps to 13.5 Gbsps


	    //fmc_adc_spi_write(fmc_no,  0x5B2, 0x65);  // Lane 1/0  <- logical 6/5
	    //fmc_adc_spi_write(fmc_no,  0x5B3, 0x74);  // Lane 3/2  <- adc 7/4
	    //fmc_adc_spi_write(fmc_no,  0x5B5, 0x23);  // Lane 5/4  <- adc 2/3
	    //fmc_adc_spi_write(fmc_no,  0x5B6, 0x10);  // Lane 7/6  <- adc 1/0
	    fmc_adc_spi_write(fmc_no,  0x5B6, 0x67);  // swap 7/6  <-
	    fmc_adc_spi_write(fmc_no,  0x5BF, 0xff);  // invert Lanes 0 to 7


	    fmc_adc_spi_write(fmc_no,  0x561, 0x4 + 0x1);  //  ADC data inverted (is not inverted !) + two complement (signed)


        fmc_adc_spi_write(fmc_no,  0x571, 0x14);  // JESD204B link power up
        //usleep(10000) ;  // wait 10ms
        usleep(15000) ;  // wait 15ms

		//addr = 0x56f ;
		//fmc_adc_spi_read (device, 0, addr, &data);
		//printf("adc reg 0x%04X = 0x%02X \n", (addr & 0x3FFF), (data & 0xff) );


        fmc_adc_spi_write(fmc_no,  0x1228, 0x4F);  // JESD2048B PLL startup control: Reset
        usleep(1000) ;  // wait 1ms
        fmc_adc_spi_write(fmc_no,  0x1228, 0x0F);  // JESD2048B PLL startup control: normal operation
        usleep(1000) ;  // wait 1ms

        fmc_adc_spi_write(fmc_no,  0x1222, 0x00);  // JESD2048B PLL calibration:  normal operation
        usleep(1000) ;  // wait 1ms
        fmc_adc_spi_write(fmc_no,  0x1222, 0x04);  // JESD2048B PLL calibration:  Reset
        usleep(1000) ;  // wait 1ms
        fmc_adc_spi_write(fmc_no,  0x1222, 0x00);  // JESD2048B PLL calibration:  normal operation
        usleep(1000) ;  // wait 1ms

        fmc_adc_spi_write(fmc_no,  0x1262, 0x08);  // JESD2048B PLL LOL bit control: Clear
        usleep(1000) ;  // wait 1ms
        fmc_adc_spi_write(fmc_no,  0x1262, 0x00);  // JESD2048B PLL LOL bit control: normal operation
        usleep(1000) ;  // wait 1ms

        //usleep(10000) ;  // wait 10ms

        return  ;
}

void sfmc01_hmc_configuration(uint32_t fmc_no)
{

	// Reset HMC on FMC1
	fmc_hmc_set_hardware_reset_cmd(fmc_no) ;
	usleep(100) ;
	// (2.) Release hardware Reset HMC on FMC1
	fmc_hmc_release_hardware_reset_cmd(fmc_no) ;
	usleep(1000) ; // 1ms
	// (3.) Load the configuration updates registers, 0x96 to 0xB8  (wrong default values) HMC on FMC1
	fmc_hmc_load_reserved_congiruation_registers (fmc_no);
	// (4.) Program PLL2, 0x31 to 0x3C
	fmc_hmc_program_pll2 (fmc_no) ;
	// (5.) Program PLL1, 0x0A to 0x2A
	fmc_hmc_program_pll1 (fmc_no) ;
	// GPIO registers , 0x46 to 0x54
	fmc_hmc_program_gpio(fmc_no);
	// SYSREF/SYNC registers , 0x5A to 0x5E
	fmc_hmc_program_sysref_sync(fmc_no);
	// Clock Distribution Network registers , 0x64 to 0x65
	fmc_hmc_program_clk_distr_network(fmc_no);
	// Alarm Mask registers , 0x70 to 0x71
	fmc_hmc_program_alarm_mask(fmc_no);
	// Output Channels , 0xC8 to 0x153
	fmc_hmc_program_channel_outputs(fmc_no);
	// fmc_hmc_program_global_control , 0x2 to 0x9
	fmc_hmc_program_global_control(fmc_no);
	// wait 10ms (8.)
	usleep(10000);
	usleep(2000);
	// issue a softare Restart
	fmc_hmc_global_control_restart_cmd(fmc_no);
	usleep(10000);
	// issue a Reseed
	fmc_hmc_global_control_reseed_cmd(fmc_no);

	return  ;
}

void        si5326_set_internal_clock_multiplier()
{
	device_ioctrl_data	rw_Ctrl;
	uint32_t n1_hs = 5;
	uint32_t nc1_ls = 10; 
	uint32_t nc2_ls = 10;
	uint32_t n2_hs = 10;
	uint32_t n2_ls = 250;
	uint32_t n31 = 250;
	uint32_t n32 = 250; 
	uint32_t bw_sel = 10;
	uint32_t clkin_mhz = 100;

	volatile uint32_t  n1_val ;
	volatile uint32_t  n1_hs_val ;
	volatile uint32_t  n2_hs_val, n2_ls_val;
	volatile uint32_t  n31_val, n32_val ;
	
	rw_Ctrl.offset          = 0;
	rw_Ctrl.data            = 0;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 1;
	rw_Ctrl.data            = 0x01;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 3;
	rw_Ctrl.data            = 0x40;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 4;
	rw_Ctrl.data            = 0x00;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 6;
	rw_Ctrl.data            = (0x07 << 3) + 0x07;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 7;
	rw_Ctrl.data            = 0x02;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 10;
	rw_Ctrl.data            = 0x00;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 11;
	rw_Ctrl.data            = 0x01;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 19;
	rw_Ctrl.data            = 0x6c;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 21;
	rw_Ctrl.data            = 0x00;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 23;
	rw_Ctrl.data            = 0x03;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 24;
	rw_Ctrl.data            = 0x06;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 55;
	rw_Ctrl.data            = 0x20;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	n31_val = n31 - 1 ;
	rw_Ctrl.offset          = 43;
	rw_Ctrl.data            = ((n31_val >> 16) & 0x7);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 44;
	rw_Ctrl.data            =((n31_val >> 8) & 0xff);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 45;
	rw_Ctrl.data            =(n31_val  & 0xff);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	n32_val = n32 - 1 ;
	rw_Ctrl.offset          = 46;
	rw_Ctrl.data            = ((n32_val >> 16) & 0x7);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 47;
	rw_Ctrl.data            =((n32_val >> 8) & 0xff);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 48;
	rw_Ctrl.data            =(n32_val  & 0xff);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	n2_hs_val = n2_hs - 4 ;
	n2_ls_val = n2_ls - 1 ;
	rw_Ctrl.offset          = 40;
	rw_Ctrl.data            =(n2_hs_val << 5) | ((n2_ls_val >> 16) & 0x0f);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 41;
	rw_Ctrl.data            =((n2_ls_val >> 8) & 0xff);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 42;
	rw_Ctrl.data            =(n2_ls_val  & 0xff);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	n1_hs_val = n1_hs - 4 ;
	rw_Ctrl.offset          = 25;
	rw_Ctrl.data            =(n1_hs_val << 5);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	n1_val = nc1_ls - 1 ;
	rw_Ctrl.offset          = 31;
	rw_Ctrl.data            =((n1_val >> 16) & 0xf);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 32;
	rw_Ctrl.data            =((n1_val >> 8) & 0xff);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 33;
	rw_Ctrl.data            =(n1_val  & 0xff);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	n1_val = nc2_ls - 1 ;
	rw_Ctrl.offset          = 34;
	rw_Ctrl.data            =((n1_val >> 16) & 0xf);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 35;
	rw_Ctrl.data            =((n1_val >> 8) & 0xff);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 36;
	rw_Ctrl.data            =(n1_val  & 0xff);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 2;
	rw_Ctrl.data            =(bw_sel << 5);
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_SPI_RW, &rw_Ctrl);
	
	rw_Ctrl.offset          = 0;
	rw_Ctrl.data            = 0;
	rw_Ctrl.cmd            = IOCTRL_W;
	rw_Ctrl.reserved    = 0;
	ioctl(fd, SIS8160_SI5326_CALIBRATION, &rw_Ctrl);
	
	return;
}

int 	   sfmc_adc_clock_config(int fmc_num){
	int ret  = 0;
	device_rw	  l_RW;
	uint32_t fmc_addr_offset ;
	
	if (fmc_num == 0) {
    		fmc_addr_offset = SIS8160_FMC1_SPACE_REG*4  ;
    	}
    	else {
    		fmc_addr_offset = SIS8160_FMC2_SPACE_REG*4  ;
    	}
	
	// Power down ADC on FMC1
	l_RW.data_rw   = 0x1000000;
	l_RW.offset_rw = fmc_addr_offset + SFMC_CONTROL2_REG*4;
	l_RW.mode_rw   = 2;
	l_RW.barx_rw   = 0;
	l_RW.size_rw   = 0;
	write (fd, &l_RW, sizeof(device_rw));
	usleep(100) ;
	
	// configure internal Reference clock (100 MHz) on SIS8160
	//printf("setup SI326:  MCLK1 = 100 MHz\n");
	si5326_set_internal_clock_multiplier();
	usleep(100000) ; // 100 ms
	
	// select MCLK1 as FMC1 CLK2
	l_RW.data_rw   = 0x0;
	l_RW.offset_rw = SIS8160_CLOCK_DISTRIBUTION_MUX_REG*4;
	l_RW.mode_rw   = 2;
	l_RW.barx_rw   = 0;
	l_RW.size_rw   = 0;
	write (fd, &l_RW, sizeof(device_rw));

	// select FMC1 CLK2 and enable as HMC CLKIN3
	l_RW.data_rw   = 0x200010;
	l_RW.offset_rw = fmc_addr_offset + SFMC_CONTROL2_REG*4;
	l_RW.mode_rw   = 2;
	l_RW.barx_rw   = 0;
	l_RW.size_rw   = 0;
	write (fd, &l_RW, sizeof(device_rw));

	// configure JESD-clock generator HMC7044
	sfmc01_hmc_configuration(fmc_num);
	
	// checK HMC PLL Lock
	ret = sfmc01_hmc_check_lock(fmc_num);
	if(!ret){
        	printf("Error: sis8160_sfmc01_hmc_check_lock (hmc is not locked)  ret = 0x%08X\n", ret);
		return ret;
	}
	
	// Set JESD Core-Reset
	l_RW.data_rw   = 0x2000;
	l_RW.offset_rw = fmc_addr_offset + SFMC_CONTROL2_REG*4;
	l_RW.mode_rw   = 2;
	l_RW.barx_rw   = 0;
	l_RW.size_rw   = 0;
	write (fd, &l_RW, sizeof(device_rw));
	
	// Power-up ADC using the PWDN pin
	l_RW.data_rw   = 0x100;
	l_RW.offset_rw = fmc_addr_offset + SFMC_CONTROL2_REG*4;
	l_RW.mode_rw   = 2;
	l_RW.barx_rw   = 0;
	l_RW.size_rw   = 0;
	write (fd, &l_RW, sizeof(device_rw));
	
	// Setting Up AD9689 adc
	sfmc01_adc_setup(fmc_num, 0);
	usleep(100000); // 100 ms
	
	// F=2 (reserved function)
	l_RW.data_rw   = 0x4000;
	l_RW.offset_rw = fmc_addr_offset + SFMC_CONTROL2_REG*4;
	l_RW.mode_rw   = 2;
	l_RW.barx_rw   = 0;
	l_RW.size_rw   = 0;
	write (fd, &l_RW, sizeof(device_rw));
	
	// Release JESD Core-Reset
	l_RW.data_rw   = 0x20000000;
	l_RW.offset_rw = fmc_addr_offset + SFMC_CONTROL2_REG*4;
	l_RW.mode_rw   = 2;
	l_RW.barx_rw   = 0;
	l_RW.size_rw   = 0;
	write (fd, &l_RW, sizeof(device_rw));
	usleep(10) ;
	
	return 1;
}

int main(int argc, char* argv[])
{
	int  di               = 0;
	int	 ch_in        = 0;
	char nod_name[15] = "";
	device_rw	  l_RW;
	device_ioctrl_data	  l_Read;
	sis8160_reg                myReg;
	device_ioctrl_dma     DMA_RW;
	device_ioctrl_time     DMA_TIME;
	u_int	          tmp_offset;
	int                     tmp_mode;
	int      	          tmp_barx;
	u_int	          tmp_size;
	u_int	          tmp_sample;
	u_int	          tmp_pattern;
	u_int	          tmp_pattern1;
	u_int	          tmp_pattern2;
	u_int	          tmp_pattern3;
	u_int	          tmp_pattern4;
	int      	          tmp_data;
	int      	          tmp_print = 0;
	int      	          tmp_print_start = 0;
	int      	          tmp_print_stop  = 0;
	int                   len = 0;
	int                   code = 0;
	int*                  tmp_dma_buf;
	int*                  tmp_write_buf;
	int                   k = 0;
	double                time_tmp = 0;
	double                time_tmp_loop               = 0;
	double                time_tmp_loop_dlt        = 0;
	double                time_tmp_loop_drv      = 0;
	double                time_tmp_loop_dlt_drv = 0;
	double                time_dlt;
	float                 tmp_fdata;
	int                   itemsize = 0;
	int                   tmp_loop = 0;
	int                   hmc_addc_ppl_lock_flag = 0;
	int ret = 0;


	itemsize = sizeof(device_rw);

	if(argc ==1){
		printf("Input \"prog /dev/sis8300-0\" \n");
		return 0;
	}
	strncpy(nod_name,argv[1],sizeof(nod_name));
	fd = open (nod_name, O_RDWR);
	if (fd < 0) {
		printf ("#CAN'T OPEN FILE \n");
		exit (1);
	}
	while (ch_in != 11){
		printf("\n READ (1) or WRITE (0) ?-");
		printf("\n GET DRIVER VERSION (2) or GET FIRMWARE VERSION (3) or GET SLOT NUM (4) ?-");
		printf("\n WRITE_IOCTL (5) or READ_ICTL (6) ?-");
		printf("\n GET_FMC_COMMAND (7) or SET_FMC_COMMAND (8) ?-");
		printf("\n GET_GLOBAL_TRIGGER (9) or SET_GLOBAL_TRIGGER (10) ?-");
		printf("\n GET_MLVDS_OUT (12) or MLVDS_OUT_ENBL (13) ?-");
		printf("\n SET_MLVDS_OUT (14) or GET_MLVDS_INPUT (15)  ?-");
		printf("\n  MASTER_RESET (16) ?-");
		printf("\n GET_DRV_INFO (17) BLINK_USER_LED (18)?-");
		printf("\n GET HMC_LOCK (19)  GET_ADC_LOCK (20)?-");
		printf("\n CHECK SFMC TRIGGER (21)?-");
		printf("\n CTRL_DMA READ (30) CTRL_DMA WRITE (31) ?-");
		//  printf("\n CTRL_DMA READ in LOOP (32))?-");
		//  printf("\n CTRL_DMA READ STAT (33) CTRL_DMA READ in LOOP STAT (34)?-");
		// printf("\n PEER2PEER DMA (35) )?-");
		printf("\n  SFMC ADC_CLOCK CONFRIG (40) SFMC CHECK ALL LOCKS(41) ?-");
		printf("\n END (11) ?-");
		scanf("%d",&ch_in);
		fflush(stdin);
		myReg.offset   = 0;
		myReg.data     = 0;
		switch (ch_in){
			case 0 :
				printf ("\n INPUT  BARx (0,1,2,3,4,5)  -");
				scanf ("%x",&tmp_barx);
				fflush(stdin);

				printf ("\n INPUT  MODE  (0-D8,1-D16,2-D32)  -");
				scanf ("%x",&tmp_mode);
				fflush(stdin);

				printf ("\n INPUT  ADDRESS (IN HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);

				printf ("\n INPUT DATA (IN HEX)  -");
				scanf ("%x",&tmp_data);
				fflush(stdin);

				l_RW.data_rw   = tmp_data;
				l_RW.offset_rw = tmp_offset;
				l_RW.mode_rw   = tmp_mode;
				l_RW.barx_rw   = tmp_barx;
				l_RW.size_rw   = 0;

				printf ("MODE - %X , OFFSET - %X, DATA - %X\n",
					 l_RW.mode_rw, l_RW.offset_rw, l_RW.data_rw);

				len = write (fd, &l_RW, sizeof(device_rw));
				if (len != itemsize ){
						printf ("#CAN'T READ FILE \n");
				}
				break;
			case 1 :
				printf ("\n INPUT  BARx (0,1,2,3,4,5)  -");
				scanf ("%x",&tmp_barx);
				fflush(stdin);
				printf ("\n INPUT  MODE  (0-D8,1-D16,2-D32)  -");
				scanf ("%x",&tmp_mode);
				fflush(stdin);
				printf ("\n INPUT OFFSET (IN HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				printf ("\n INPUT SAMPLE NUM (DEC)  -");
				scanf ("%i",&tmp_sample);
				fflush(stdin);

				l_RW.data_rw    = 0;
				l_RW.offset_rw  = tmp_offset;
				l_RW.mode_rw  = tmp_mode;
				l_RW.barx_rw    = tmp_barx;
				l_RW.size_rw     = tmp_sample;
				switch(tmp_mode){
					case 0:
						 tmp_size = sizeof(u_char)*tmp_sample;
						break;
				   case 1:
						 tmp_size = sizeof(u_short)*tmp_sample;
						break;
				   case 2:
						 tmp_size = sizeof(u_int)*tmp_sample;
						break;
				  default:
						 tmp_size = sizeof(u_int)*tmp_sample;
						break;
				}
				 printf ("MODE - %X , OFFSET - %X, SAMPLE %i DATA - %X\n", l_RW.mode_rw, l_RW.offset_rw, l_RW.size_rw , l_RW.data_rw);
				if(tmp_sample < 2){
						len = read (fd, &l_RW, sizeof(device_rw));
						if (len != itemsize ){
						   printf ("#CAN'T READ FILE ERROR %i \n", len);
						}
						printf ("READED : MODE - %X , OFFSET - %X, DATA - %X\n",  l_RW.mode_rw, l_RW.offset_rw, l_RW.data_rw);
				}else{
					   tmp_dma_buf     = new int[tmp_size + itemsize];
					   memcpy(tmp_dma_buf, &l_RW, itemsize);
					   gettimeofday(&start_time, 0);
					   len = read (fd, tmp_dma_buf, sizeof(device_rw));
					   gettimeofday(&end_time, 0);
						if (len != itemsize ){
						   printf ("#CAN'T READ FILE ERROR %i \n", len);
						}
					   time_tmp    =  MIKRS(end_time) - MIKRS(start_time);
					   time_dlt       =  MILLS(end_time) - MILLS(start_time);
					   printf("STOP READING TIME %fms : %fmks  SIZE %lu\n", time_dlt, time_tmp,tmp_size);
					   printf("STOP READING KBytes/Sec %f\n",(tmp_size*1000)/time_tmp);

						printf ("PRINT (0 NO, 1 YES)  -\n");
						scanf ("%d",&tmp_print);
						fflush(stdin);
						while (tmp_print){
							printf ("START POS  -\n");
							scanf ("%d",&tmp_print_start);
							fflush(stdin);
							printf ("STOP POS  -\n");
							scanf ("%d",&tmp_print_stop);
							fflush(stdin);
							k = tmp_print_start*4;
							for(int i = tmp_print_start; i < tmp_print_stop; i++){
									printf("NUM %i OFFSET %X : DATA %X\n", i,k, (u_int)(tmp_dma_buf[i] & 0xFFFFFFFF));
									k += 4;
							}
							printf ("PRINT (0 NO, 1 YES)  -\n");
							scanf ("%d",&tmp_print);
							fflush(stdin);
					}
					 if(tmp_dma_buf) delete tmp_dma_buf;
				}
				break;
			case 2 :
				ioctl(fd, SIS8160_DRIVER_VERSION, &l_Read);
				tmp_fdata = (float)((float)l_Read.offset/10.0);
				tmp_fdata += (float)l_Read.data;
				printf ("DRIVER VERSION IS %f\n", tmp_fdata);
				break;
			case 3 :
				ioctl(fd, SIS8160_FIRMWARE_VERSION, &l_Read);
				printf ("FIRMWARE VERSION IS - %X\n", l_Read.data);
				break;
			case 4 :
				ioctl(fd, SIS8160_PHYSICAL_SLOT, &l_Read);
				printf ("SLOT NUM IS - %X\n", l_Read.data);
				break;
			case 5 :
				printf ("\n INPUT  ADDRESS (IN HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				printf ("\n INPUT DATA (IN HEX)  -");
				scanf ("%x",&tmp_data);
				fflush(stdin);
				myReg.data   = tmp_data;
				myReg.offset = tmp_offset;
				printf ("OFFSET - %X, DATA - %X\n", myReg.data, myReg.offset);
				ioctl(fd, SIS8160_REG_WRITE, &myReg);
				break;
			case 6 :
				printf ("\n INPUT OFFSET (IN HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);		
				myReg.data    = 0;
				myReg.offset  = tmp_offset;
				printf ("OFFSET - %X, DATA - %X\n", myReg.data, myReg.offset);                
				ioctl(fd, SIS8160_REG_READ, &myReg);
				printf ("READED : OFFSET - %X, DATA - %X\n", myReg.offset, myReg.data);
				break;

			case 7 : // GET_FMC_COMMAND (7) 
				l_Read.data     = 0;
				l_Read.offset   = 0;
				l_Read.cmd      = 1;
				l_Read.reserved = 0;
				ioctl(fd, SIS8160_FMC_COMMAND, &l_Read);
				printf ("READED : DATA - %X\n", l_Read.data);
				break;
			case 8 : // SET_FMC_COMMAND (8) 
				tmp_pattern = 0;
				tmp_offset    = 0;
				printf ("\n INPUT COMMAND (1-5 or 0xF) -");
				scanf ("%x",&tmp_data);
				fflush(stdin);
				if(tmp_pattern == 1){
				printf ("\n INPUT CHANNEL (1-2) -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				}
				l_Read.data     = tmp_data;
				l_Read.offset   = tmp_offset;
				l_Read.cmd      = 0;
				l_Read.reserved = tmp_pattern;
				ioctl(fd, SIS8160_FMC_COMMAND, &l_Read);
				break;
			case 9 : // GET_GLOBAL_TRIGGER 
				l_Read.data     = 0;
				l_Read.offset   = 0;
				l_Read.cmd      = 1;
				l_Read.reserved = 0;
				ioctl(fd, SIS8160_GET_GLOBAL_TRIGGER, &l_Read);
				printf ("READED : DATA - %X\n", l_Read.data);
				break;
			case 10 : // SET_GLOBAL_TRIGGER
				tmp_pattern = 0;
				tmp_offset    = 0;
				printf ("\n INPUT COMMAND (1-5) -");
				scanf ("%x",&tmp_mode);
				fflush(stdin);
				if(tmp_mode == 1){
				printf ("\n INPUT CHANNEL (1-7) -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				}
				printf ("\n EDGE (0 rising/1 falling)  -");
				scanf ("%x",&tmp_pattern);
				fflush(stdin);
				tmp_data     = 0;
				if(tmp_mode){
					printf ("\n DATA  -");
					scanf ("%x",&tmp_data);
					fflush(stdin);
				}
				l_Read.data     = tmp_data;
				l_Read.offset   = tmp_offset;
				l_Read.cmd      = tmp_mode;
				l_Read.reserved = tmp_pattern;
				ioctl(fd, SIS8160_SET_GLOBAL_TRIGGER, &l_Read);
				break;
			case 12 : //GET_MLVDS_OUT
				l_Read.data     = tmp_data;
				l_Read.offset   = tmp_offset;
				l_Read.cmd      = tmp_mode;
				l_Read.reserved = 0;
				ioctl(fd, SIS8160_GET_MLVDS_OUTPUT, &l_Read);
				printf ("READED : MLVDS OUT DATA - %X\n", l_Read.offset, l_Read.data);
				break;
			case 13 : //MLVDS_OUT_ENBL
				printf ("\n INPUT CHANNEL (IN HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				printf ("\n READ/WRITE (0/1)  -");
				scanf ("%x",&tmp_mode);
				fflush(stdin);
				tmp_data     = 0;
				if(tmp_mode){
					printf ("\n DATA  -");
					scanf ("%x",&tmp_data);
					fflush(stdin);
				}
				l_Read.data     = tmp_data;
				l_Read.offset   = tmp_offset;
				l_Read.cmd      = tmp_mode;
				l_Read.reserved = 0;
				ioctl(fd, SIS8160_ENABLE_MLVDS_OUTPUT, &l_Read);
				printf ("READED : CHANNEL - %X, DATA - %X\n", l_Read.offset, l_Read.data);
				break;
			case 14 : //SET_MLVDS_OUT
				printf ("\n INPUT CHANNEL (IN HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				printf ("\n DATA  -");
				scanf ("%x",&tmp_data);
				fflush(stdin);
				l_Read.data     = tmp_data;
				l_Read.offset   = tmp_offset;
				l_Read.cmd      = tmp_mode;
				l_Read.reserved = 0;
				ioctl(fd, SIS8160_SET_MLVDS_OUTPUT, &l_Read);
				printf ("READED : CHANNEL - %X, DATA - %X\n", l_Read.offset, l_Read.data);
				break;
			case 15 : //GET_MLVDS_IN
				l_Read.data     = tmp_data;
				l_Read.offset   = tmp_offset;
				l_Read.cmd      = tmp_mode;
				l_Read.reserved = 0;
				ioctl(fd, SIS8160_GET_MLVDS_INPUT, &l_Read);
				printf ("READED : MLVDS IN DATA - %X\n", l_Read.offset, l_Read.data);
				break;
			case 16 : //MASTER_RESET
				printf ("\n RESET ? (0-NO, 1-YES) -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				l_Read.data     = tmp_data;
				l_Read.offset   = tmp_offset;
				l_Read.cmd     = tmp_mode;
				l_Read.reserved = 0;
				if(tmp_offset)
					ioctl(fd, SIS8160_MASTER_RESET, &l_Read);
				break;
			case 17 :
				fflush(stdin);
				l_RW.data_rw   = 0;
				l_RW.offset_rw = 0;
				l_RW.mode_rw   = RW_INFO;
				l_RW.barx_rw   = 0;
				l_RW.size_rw   = 0;
				len = read (fd, &l_RW, sizeof(device_rw));
				if (len != itemsize ){
					printf ("#CAN'T READ FILE \n");
				}
				printf ("READED : SLOT             - %X \n", l_RW.barx_rw);
				printf ("READED : FIRMWARE VERSION - %X \n", l_RW.mode_rw);
				tmp_fdata = (float)((float)l_RW.offset_rw/10.0);
				tmp_fdata += (float)l_RW.data_rw;
				printf ("DRIVER VERSION IS %f\n", tmp_fdata);
				break;
			  case 18 :
				printf ("\n INPUT  DELAY (usec)  -");
				scanf ("%x",&tmp_data);
				fflush(stdin);
				printf ("\n INPUT  ON->OFF (1), OFF->ON (0) (IN HEX)");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				myReg.offset = tmp_offset;
				myReg.data   = tmp_data;
				ioctl(fd, SIS8160_BLINK_LED, &myReg);
				break;
			case 19 :
				printf ("\nSFMC NO -");
				scanf ("%i",&tmp_offset);
				fflush(stdin);
				l_Read.data     = 0;
				l_Read.offset   = 0x7c;
				l_Read.cmd     = IOCTRL_R;
				l_Read.reserved = tmp_offset;
				ioctl(fd, SFMC_HMC_SPI_RW, &l_Read);
				printf ("DATA 0x%X : 0x%X\n", l_Read.data, (l_Read.data & 0x7F));
				if ((l_Read.data & 0x7f) !=  0x20) {
					printf("PLL1 is not locked !   reg 0x7C = 0x%08X \n", l_Read.data);
					break;
				}
				l_Read.data     = 0;
				l_Read.offset   = 0x7d;
				l_Read.cmd     = IOCTRL_R;
				l_Read.reserved = tmp_offset;
				ioctl(fd, SFMC_HMC_SPI_RW, &l_Read);
				printf ("DATA 0x%X : 0x%X\n", l_Read.data, (l_Read.data & 0x8));
				if ((l_Read.data & 0x8) !=  0x8) {
					printf("PLL1 or PLL2 is not locked !   reg 0x7D = 0x%08X \n", l_Read.data);
					break;;
				}
				printf("PLL1 or PLL2 is LOCKED ! \n");
				break;
			case 20 :
				printf ("\n SFMC NO -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				l_Read.data     = 0;
				l_Read.offset   = 0x56F;
				l_Read.cmd     = IOCTRL_R;
				l_Read.reserved = tmp_offset;
				ioctl(fd, SFMC_ADC_SPI_RW, &l_Read);
				printf ("DATA 0x%X : 0x%X\n", l_Read.data, (l_Read.data & 0xFF));
				if ((l_Read.data & 0xff) !=  0x80) {
					printf("ADC PLL is not locked !   reg 0x56F = 0x%08X \n", l_Read.data);
				   break;
				}
				printf("ADC PLL is LOCKED ! \n");
				break;
			case 21 :
				printf ("\n FMC NUM (0/1)  -");
				scanf ("%i",&tmp_offset);
				fflush(stdin);		
				myReg.data    = 0;
				myReg.offset  = 0x800 + 0x400*tmp_offset + 0x4;
				printf ("OFFSET - %X, DATA - %X\n", myReg.data, myReg.offset);     
				while(1){
					ioctl(fd, SIS8160_REG_READ, &myReg);
					printf ("READED : OFFSET - %X, DATA - %X\n", myReg.offset, myReg.data);
					usleep(100) ;
				}
				break;
			case 40 :
				printf ("\n SFMC NO -");
				scanf ("%x",&tmp_offset);
				sfmc_adc_clock_config(tmp_offset);
				break;
			case 41 :
				printf ("\n SFMC NO -");
				scanf ("%x",&tmp_offset);
				do {
					hmc_addc_ppl_lock_flag = 1 ;
					ret = sfmc01_hmc_check_lock(tmp_offset);
					printf("sis8160_sfmc01_hmc_check_lock: ret = 0x%08X  \n", ret);
					if(!ret){
						printf("HMC clock generator PLL is not locked !    \n");
						hmc_addc_ppl_lock_flag = 0 ;
					}

					// check ADC PLL Lock Status
					ret = sfmc01_adc_check_lock(tmp_offset);
					printf("sis8160_sfmc01_adc_check_lock: ret = 0x%08X  \n", ret);
					if(!ret){
						printf("ADC PLL is not locked !    \n");
						hmc_addc_ppl_lock_flag = 0 ;
					}

					if((hmc_addc_ppl_lock_flag == 0)) {
						printf("configure Clock and ADC  \n");
						ret = sfmc_adc_clock_config (tmp_offset);
						if(!ret){
							printf("sfmc01_clock_and_adc_configuration: ret = 0x%08X  \n", ret);
							break;
						}
					}
					printf("\n");
				} while (hmc_addc_ppl_lock_flag == 0) ;
				printf("Clock and ADC are configured and PLLs are locked \n");
				printf("\n");
				break;
			case 30 :
				DMA_RW.dma_offset  = 0;
				DMA_RW.dma_size    = 0;
				DMA_RW.dma_cmd     = 0;
				DMA_RW.dma_pattern = 0; 
				printf ("\n INPUT  DMA_SIZE (num of sumples (int))  -");
				scanf ("%d",&tmp_size);
				fflush(stdin);
				DMA_RW.dma_size    = sizeof(int)*tmp_size;
				printf ("\n INPUT OFFSET (HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				DMA_RW.dma_offset = tmp_offset;
				printf ("\n INPUT MEMORY (0,1)  -");
				scanf ("%x",&tmp_pattern);
				fflush(stdin);
				DMA_RW.dma_reserved1 = tmp_pattern;

				printf ("DMA_OFFSET - %X, DMA_SIZE - %X\n", DMA_RW.dma_offset, DMA_RW.dma_size);
				printf ("MAX_MEM- %X, DMA_MEM - %X:%X\n", 536870912,  (DMA_RW.dma_offset + DMA_RW.dma_size),
																							  (DMA_RW.dma_offset + DMA_RW.dma_size*4));

				tmp_dma_buf     = new int[tmp_size + DMA_DATA_OFFSET];
				memcpy(tmp_dma_buf, &DMA_RW, sizeof (device_ioctrl_dma));

				gettimeofday(&start_time, 0);
				code = ioctl (fd, SIS8160_READ_DMA, tmp_dma_buf);
				gettimeofday(&end_time, 0);
				printf ("===========READED  CODE %i\n", code);
				time_tmp    =  MIKRS(end_time) - MIKRS(start_time);
				time_dlt       =  MILLS(end_time) - MILLS(start_time);
				printf("STOP READING TIME %fms : %fmks  SIZE %lu\n", time_dlt, time_tmp,(sizeof(int)*tmp_size));
				printf("STOP READING KBytes/Sec %f\n",((sizeof(int)*tmp_size*1000)/time_tmp));
				code = ioctl (fd, SIS8160_GET_DMA_TIME, &DMA_TIME);
				if (code) {
					printf ("######ERROR GET TIME %d\n", code);
				}
				printf ("===========DRIVER TIME \n");
				time_tmp = MIKRS(DMA_TIME.stop_time) - MIKRS(DMA_TIME.start_time);
				time_dlt    = MILLS(DMA_TIME.stop_time) - MILLS(DMA_TIME.start_time);
				printf("STOP DRIVER TIME START %li:%li STOP %li:%li\n",
															DMA_TIME.start_time.tv_sec, DMA_TIME.start_time.tv_usec, 
															DMA_TIME.stop_time.tv_sec, DMA_TIME.stop_time.tv_usec);
				printf("STOP DRIVER READING TIME %fms : %fmks  SIZE %lu\n", time_dlt, time_tmp,(sizeof(int)*tmp_size));
				printf("STOP DRIVER READING KBytes/Sec %f\n",((sizeof(int)*tmp_size*1000)/time_tmp));
				printf ("PRINT (0 NO, 1 YES)  -\n");
				scanf ("%d",&tmp_print);
				fflush(stdin);
				while (tmp_print){
					printf ("START POS  -\n");
					scanf ("%d",&tmp_print_start);
					fflush(stdin);
					printf ("STOP POS  -\n");
					scanf ("%d",&tmp_print_stop);
					fflush(stdin);
					k = tmp_print_start*4;
					for(int i = tmp_print_start; i < tmp_print_stop; i++){
							printf("NUM %i OFFSET %X : DATA %X\n", i,k, (u_int)(tmp_dma_buf[i] & 0xFFFFFFFF));
							k += 4;
					}
					printf ("PRINT (0 NO, 1 YES)  -\n");
					scanf ("%d",&tmp_print);
					fflush(stdin);
				}
				if(tmp_dma_buf) delete tmp_dma_buf;
				break;
			case 31 :
				DMA_RW.dma_offset  = 0;
				DMA_RW.dma_size    = 0;
				DMA_RW.dma_cmd     = 0;
				DMA_RW.dma_pattern = 0; 
				DMA_RW.dma_reserved1 = 33; 
				DMA_RW.dma_reserved2 = 44;
				printf ("\n INPUT  DMA_SIZE (num of sumples (int))  -");
				scanf ("%d",&tmp_size);
				fflush(stdin);
				DMA_RW.dma_size = sizeof(int)*tmp_size;
				printf ("\n INPUT OFFSET (HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				DMA_RW.dma_offset = tmp_offset;
				printf ("\n INPUT MEMORY (0,1)  -");
				scanf ("%x",&tmp_pattern);
				fflush(stdin);
				DMA_RW.dma_reserved1 = tmp_pattern;

				printf ("\n INPUT PATTERN (HEX)  -");
				scanf ("%x",&tmp_pattern);
				fflush(stdin);

				tmp_write_buf     = new int[tmp_size + DMA_DATA_OFFSET];
				memcpy(tmp_write_buf, &DMA_RW, sizeof (device_ioctrl_dma));


				for(int ii = 0; ii < tmp_size; ++ii){
					tmp_write_buf[ii + DMA_DATA_OFFSET] = ((tmp_pattern + ii) & 0xFFFF) + (((tmp_pattern + ii) & 0xFFFF)<< 16);
				}
				k= 0;
				for(int i = 0; i < 10; i++){
					printf("OFFSET %d : DATA %X\n", k, (u_int)(tmp_write_buf[i] & 0xFFFFFFFF));
					k++;
				}

				code = ioctl (fd, SIS8160_WRITE_DMA, tmp_write_buf);
				if (code) {
					printf ("######ERROR DMA  %d\n", code);
				}   

				if(tmp_write_buf) delete tmp_write_buf;
				break;

			default:
				  break;
		}
	}
	close(fd);
	return 0;
}

