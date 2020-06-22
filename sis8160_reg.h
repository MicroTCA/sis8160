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

/*
*	Author: Ludwig Petrosyan (Email: ludwig.petrosyan@desy.de)
*/


#ifndef  SIS8160_REG_H_
#define SIS8160_REG_H_

	// all bit definitions are bit numbers
	#define SIS8160_INDENTIFIER_VERSION_REG			0x00
	#define SIS8160_SERIAL_NUMBER_REG				0x01
	#define SIS8160_USER_CONTROL_STATUS_REG			0x04
	#define SIS8160_FIRMWARE_OPTIONS_REG			0x05
	#define SIS8160_PCIE_STATUS_REG					0x07
	#define SIS8160_GLOBAL_FMC_COMMAND_REG			0x10
	#define SIS8160_GLOBAL_FMC_CONTROL_REG			0x11
	#define SIS8160_MLVDS_IO_CONTROL_REG			0x12
	#define SIS8160_SYNCH_COMMAND_REG				0x13
	#define SIS8160_USR_IRQ_SOURCE_REG				0x14
	#define SIS8160_USR_IRQ_CONTROL_STATUS_REG		0x15
	#define SIS8160_PORT12_LINK_CONTRO_STATUS_REG	0x16
	#define SIS8160_PORT13_LINK_CONTRO_STATUS_REG	0x17
	#define SIS8160_PORT14_LINK_CONTRO_STATUS_REG	0x18
	#define SIS8160_PORT15_LINK_CONTRO_STATUS_REG	0x19
	#define SIS8160_CLOCK_DISTRIBUTION_MUX_REG 		0x40
	#define SIS8160_RSERVED_REG						0x41
	#define SIS8160_CLOCK_MULTIPLIER_SPI_REG       		0x42
	#define SIS8160_CLOCK_SYNTHESIZER_MGTI_REG		0x43
	#define SIS8160_FPGA_BOOT_SPI_REG				0x44
	#define SIS8160_WHITE_RABBIT_DAC_SPI_REG			0x4A
	#define SIS8160_UBLAZE_CONTROL_STATUS_REG		0xFE
	#define SIS8160_MASTER_RESET_REG				0xFF
	#define SIS8160_DMA_READ_DST_ADR_LO32			0x200
	#define SIS8160_DMA_READ_DST_ADR_HI32			0x201
	#define SIS8160_DMA_READ_SRC_ADR_LO32			0x202
	#define SIS8160_DMA_READ_LEN					0x203
	#define SIS8160_DMA_READ_CTRL					0x204
	#define SIS8160_DMA_READ_SPACE_SELECT			0x207
	#define SIS8160_DMA_WRITE_SRC_ADR_LO32			0x210
	#define SIS8160_DMA_WRITE_SRC_ADR_HI32			0x211
	#define SIS8160_DMA_WRITE_DST_ADR_LO32			0x212
	#define SIS8160_DMA_WRITE_LEN					0x213
	#define SIS8160_DMA_WRITE_CTRL					0x214
	#define SIS8160_DMA_WRITE_MAX_REQUESTS_NUM		0x215
	#define SIS8160_DAQ_AUTO_DMA_CONTROL			0x216
	#define SIS8160_DMA_WRITE_SPACE_SELECT			0x217
	#define SIS8160_IRQ_ENABLE_REG					0x220
	#define SIS8160_IRQ_STATUS_REG					0x221
	#define SIS8160_IRQ_CLEAR_REG					0x222
	#define SIS8160_IRQ_REFRESH_REG					0x223
	#define SIS8160_MEMORY_TEST_MODE_REG			0x230
	#define SIS8160_RAM_FIFO_DEBUG_REG				0x231
	#define SIS8160_FMC1_SPACE_REG					0x800   //0x2000
	#define SIS8160_FMC2_SPACE_REG					0xC00  //0x3000
	#define SIS8160_FMC_SPACE_SIZE					0x3FF
	#define SIS8160_FMC_SPACE_SIZE_BYTE				0xFFC

	//FMC REGISTERS
	#define SFMC_VERSION_REG					0x00
	#define SFMC_RESRVED_REG					0x01
	#define SFMC_STATUS1_REG					0x02
	#define SFMC_STATUS2_REG					0x03
	#define SFMC_CONTROL1_REG					0x04
	#define SFMC_CONTROL2_REG					0x05
	#define SFMC_HMC_SPI_REG					0x06
	#define SFMC_ADC_SPI_REG					0x07
	#define SFMC_JESD_ADDRESS_REG				0x08
	#define SFMC_JESD_DATA_REG					0x09
	#define SFMC_ADCA_HEADER_REG				0x0A
	#define SFMC_ADCB_HEADER_REG				0x0B
	#define SFMC_ADC_ACQ_SC_REG				0x10
	#define SFMC_ADC_SAMPLE_SC_REG				0x11
	#define SFMC_ADCA_SAMPLE_ADDRESS_REG		0x12
	#define SFMC_ADCB_SAMPLE_ADDRESS_REG		0x13
	#define SFMC_ADC_SAMPLE_LENGHT_REG		0x14
	#define SFMC_ADC_PRETRG_DELAY_REG			0x15
	#define SFMC_ADC_MAX_NOF_EVENT_REG		0x16
	#define SFMC_ADC_TRG_DELAY_REG				0x17
	#define SFMC_ADCA_ACTUAL_ADDRESS_REG		0x18
	#define SFMC_ADCB_ACTUAL_ADDRESS_REG		0x19
	#define SFMC_ADC_EVENT_COUNTER_REG		0x1A
	

#endif // SIS8160_REG_
