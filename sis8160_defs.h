/* 
 * File:   sis8160_defs.h
 * Author: petros
 *
 * Created on January 12, 2011, 5:20 PM
 */

/**
*Copyright 2016-  DESY (Deutsches Elektronen-Synchrotron, www.desy.de)
*
*This file is part of SIS8160 driver.
*
*SIS8160 is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*SIS8160 is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with SIS8160.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef	SIS8160_DEFS_H
#define	SIS8160_DEFS_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include "pciedev_io.h"

#define MUXA              0x00
#define MUXB              0x01
#define MUXC              0x02
#define MUXD               0x04
#define MUXE               0x05
#define SPI_AD9510     0x41
#define SPI_SI5326       0x42
#define SPI_DAC            0x44
#define SPI_ADC            0x48
#define MODE_RESET  0x4
#define MODE_TRG      0x2
#define MODE_NOW    0x1

#define FMC_CMD_DISARM              0x1
#define FMC_CMD_ARM	               0x2
#define FMC_CMD_AR_TIME_CLR   0x3
#define FMC_CMD_TIME_CLR          0x4
#define FMC_CMD_SOFT_TRG         0x5
#define FMC_CMD_USR_IRQ            0xF

#define FMC_RESET_TEST_COUNTER         0xE
#define FMC_SAMPLE_LOGIC		         0xF

#define FMC_TRG_MLVDS              0x1
#define FMC_TRG_FMC1                0x2
#define FMC_TRG_FMC2                0x3
#define FMC_TRG_FMC1_TIME     0x4
#define FMC_TRG_FMC2_TIME     0x5

#define FMC1              0x0
#define FMC2              0x1

#define ADC_NO_STORE                     0x0
#define ADC_STORE_BANK1              0x1
#define ADC_STORE_BANK2              0x3

#define SIS8160_DMA_SYZE                  4096

#define SI5326_SPI_POLL_COUNTER_MAX							1000
#define SI5326_SPI_CALIBRATION_READY_POLL_COUNTER_MAX		10000

struct device_ioctrl_data_buf  {
        u_int    offset;
        u_int    cmd;
        u_int    num;
        u_int    step;
        u_int    data[64];
};
typedef struct device_ioctrl_data_buf device_ioctrl_data_buf;

typedef struct t_sis8160reg{
	u_int offset; /* offset from bar0 */
	u_int data;   /* data which will be read / written */
}sis8160_reg;

/* Use 'o' as magic number */

#define SIS8160_IOC           			'0'
/*
#define SIS8160_USER_IRQ_SELECT			_IOWR(SIS8160_IOC, 5, int)
#define SIS8160_USR_IRQ_CS				_IOWR(SIS8160_IOC, 6, int)
#define SIS8160_PORT_LINK_CS				_IOWR(SIS8160_IOC, 7, int)
#define SIS8160_SI5326_CLOCKS_SPI			_IOWR(SIS8160_IOC, 8, int)
#define SIS8160_MGT_CLOCKS_SPI			_IOWR(SIS8160_IOC, 9, int)
*/
#define SIS8160_AUX_IO				_IOWR(SIS8160_IOC, 8, int)
#define SIS8160_AUX_IN_TERM			_IOWR(SIS8160_IOC, 9, int)
#define SIS8160_PHYSICAL_SLOT		_IOWR(SIS8160_IOC, 10, int)
#define SIS8160_REG_READ			_IOWR(SIS8160_IOC, 11, int)
#define SIS8160_REG_WRITE			 _IOWR(SIS8160_IOC, 12, int)
#define SIS8160_GET_DMA_TIME		_IOWR(SIS8160_IOC, 13, int)
#define SIS8160_DRIVER_VERSION		_IOWR(SIS8160_IOC, 14, int)
#define SIS8160_FIRMWARE_VERSION	_IOWR(SIS8160_IOC, 15, int)
#define SIS8160_READ_DMA			_IOWR(SIS8160_IOC, 16, int)
#define SIS8160_WRITE_DMA			_IOWR(SIS8160_IOC, 17, int)
#define SIS8160_WRITE_DMA_2PEER		_IOWR(SIS8160_IOC, 18, int)
#define SIS8160_BLINK_LED			_IOWR(SIS8160_IOC, 19, int)
#define SIS8160_FMC_COMMAND		_IOWR(SIS8160_IOC, 20, int)
#define SIS8160_GET_GLOBAL_TRIGGER		_IOWR(SIS8160_IOC, 21, int)
#define SIS8160_SET_GLOBAL_TRIGGER		_IOWR(SIS8160_IOC, 22, int)
#define SIS8160_GET_MLVDS_INPUT			_IOWR(SIS8160_IOC, 23, int)
#define SIS8160_GET_MLVDS_OUTPUT		_IOWR(SIS8160_IOC, 24, int)
#define SIS8160_SET_MLVDS_OUTPUT		_IOWR(SIS8160_IOC, 25, int)
#define SIS8160_ENABLE_MLVDS_OUTPUT		_IOWR(SIS8160_IOC, 26, int)
#define SIS8160_MASTER_RESET			_IOWR(SIS8160_IOC, 27, int)
#define SIS8160_SYNCH_COMMAND			_IOWR(SIS8160_IOC, 28, int)
#define SIS8160_CLOCK_MP_CONTROL		_IOWR(SIS8160_IOC, 29, int)

//SFMC CTRL
#define SFMC_USER_LED					_IOWR(SIS8160_IOC, 30, int)
#define SFMC_TRG_TERMINATION			_IOWR(SIS8160_IOC, 31, int)
#define SFMC_POWER_UP					_IOWR(SIS8160_IOC, 32, int)
#define SFMC_CLOCK23_SELECT2			_IOWR(SIS8160_IOC, 33, int)
#define SFMC_CLOCK23_ENABLE			_IOWR(SIS8160_IOC, 34, int)
#define SFMC_HMC_CLOCK_PU				_IOWR(SIS8160_IOC, 35, int)
#define SFMC_COMMAND					_IOWR(SIS8160_IOC, 36, int)
#define SFMC_SAMPLE_STORE_MODE		_IOWR(SIS8160_IOC, 37, int)
#define SFMC_SAMPLE_DECIMATION			_IOWR(SIS8160_IOC, 38, int)
#define SFMC_SAMPLE_DECIMATION_SIGNED	_IOWR(SIS8160_IOC, 39, int)
#define SFMC_SAMPLE_START_ADDRESS		_IOWR(SIS8160_IOC, 40, int)
#define SFMC_SAMPLE_LENGHT			_IOWR(SIS8160_IOC, 41, int)
#define SFMC_PRETRG_DELAY				_IOWR(SIS8160_IOC, 42, int)
#define SFMC_EVENT_NUMBER				_IOWR(SIS8160_IOC, 43, int)
#define SFMC_EXTTRG_DELAY				_IOWR(SIS8160_IOC, 44, int)
#define SFMC_ACTUAL_ADDRESS			_IOWR(SIS8160_IOC, 45, int)
#define SFMC_EVENT_COUNTER			_IOWR(SIS8160_IOC, 46, int)

#define SFMC_ENABLE_HEADER			_IOWR(SIS8160_IOC, 47, int)
#define SFMC_EXTTRG_DELAY_ENABLE		_IOWR(SIS8160_IOC, 48, int)
#define SFMC_EXTTRG_FALLING_EDGE		_IOWR(SIS8160_IOC, 49, int)
#define SFMC_EXTTRG_RISING_EDGE			_IOWR(SIS8160_IOC, 50, int)

#define SFMC_HMC_SPI_RW				_IOWR(SIS8160_IOC, 51, int)
#define SFMC_ADC_SPI_RW				_IOWR(SIS8160_IOC, 52, int)
#define SIS8160_SI5326_SPI_RW				_IOWR(SIS8160_IOC, 53, int)
#define SIS8160_SI5326_CALIBRATION		_IOWR(SIS8160_IOC, 54, int)

#define SIS8160_IOC_MAXNR          59
#define SIS8160_IOC_MINNR           5

//#define SIS8160_IOC_MAXNR          29
//#define SIS8160_IOC_MINNR           5
#define SIS8160_SFMC_MAXNR       59
#define SIS8160_SFMC_MINNR        30

#endif	/* SIS8160_DEFS_H */

