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


#include <linux/types.h>
#include <linux/timer.h>
//#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/delay.h>

#include "sis8160_fnc.h"
#include "sis8160_defs.h"
#include "sis8160_reg.h"


/*			
		//case SIS8160_USER_IRQ_SELECT:
		//case SIS8160_USR_IRQ_CS:
		//case SIS8160_PORT_LINK_CS:
		//case SIS8160_SI5326_CLOCKS_SPI:
		//case SIS8160_MGT_CLOCKS_SPI:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			tmp_data_32       = ioread32(address + SIS8160_MLVDS_IO_CONTROL_REG*4);
			smp_rmb();
			iowrite32(tmp_data_32, ((void*)(address + SIS8160_MLVDS_IO_CONTROL_REG*4)));
			smp_wmb();
			udelay(2);
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
*/

long     sis8160_ioctl_dma(struct file *filp, unsigned int *cmd_p, unsigned long *arg_p)
{
	unsigned int     cmd;
	unsigned long  arg;
	 pid_t                 cur_proc = 0;
	int                      minor    = 0;
	int                      d_num    = 0;
	int                      retval   = 0;
	long                   timeDMAwait;
	ulong                 value;
	u_int	           tmp_dma_size;
	u_int	           tmp_dma_trns_size;
	u_int	           tmp_dma_offset;
	void*                  pWriteBuf          = 0;
	void*                  address;
	int                      tmp_order          = 0;
	unsigned long  length             = 0;
	dma_addr_t      pTmpDmaHandle;
	u32                    dma_sys_addr ;
	int                      tmp_source_address = 0;
	u_int                  tmp_offset;
	u_int                  tmp_data;
	u_int                  tmp_cmd;
	u_int                  tmp_reserved;
	u32                    tmp_data_32 = 0;
	u32                    rd_data_32 = 0;
	u32                    mask_data_32 = 0;
	
	uint32_t read_data      = 0;
	uint32_t write_data     = 0;
	uint32_t poll_counter  = 0;
	uint32_t cal_poll_counter  = 0;

	struct pci_dev*          pdev;
	struct pciedev_dev*  dev ;
	struct sis8160_dev*  sis8160dev ;
    
	sis8160_reg              reg_data;
	device_ioctrl_dma   dma_data;
	device_ioctrl_data   data;
	device_ioctrl_time   time_data;
	int                              reg_size;
	int                              io_size;
	int                              io_buf_size;
	int                              time_size;
	int                              io_dma_size;
	int                             dma_done_count;

	cmd                            = *cmd_p;
	arg                              = *arg_p;
	reg_size                      = sizeof(sis8160_reg);
	io_size                         = sizeof(device_ioctrl_data);
	io_buf_size                  = sizeof(device_ioctrl_data_buf);
	time_size	= sizeof(device_ioctrl_time);
	io_dma_size = sizeof(device_ioctrl_dma);
    
	dev                 = filp->private_data;
	sis8160dev    = (sis8160_dev   *)dev->dev_str;
	pdev               = (dev->pciedev_pci_dev);
	minor             = dev->dev_minor;
	d_num           = dev->dev_num;	
	cur_proc       = current->group_leader->pid;

	if(!dev->dev_sts){
		printk(KERN_ALERT  "SIS8160_IOCTL_DMA: NO DEVICE %d\n", dev->dev_num);
		retval = -EFAULT;
		return retval;
	}

	address = pciedev_get_baraddress(BAR0, dev);
	if(!address){
		printk(KERN_ALERT  "SIS8160_IOCTL_DMA: NO MEMORY\n");
		retval = -EFAULT;
		return retval;
	}
    

    
	if (mutex_lock_interruptible(&dev->dev_mut)){
		printk(KERN_ALERT  "SIS8160_IOCTL_DMA: NO MUTEX\n");
		return -ERESTARTSYS;
	}
    
	switch (cmd) {
		case SIS8160_PHYSICAL_SLOT:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset;
			tmp_data     = data.data;
			tmp_cmd      = data.cmd;
			tmp_reserved = data.reserved;
			data.data    = dev->slot_num;
			data.cmd     = SIS8160_PHYSICAL_SLOT;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			break;
		case SIS8160_DRIVER_VERSION:
			data.data   =   dev->parent_dev->PCIEDEV_DRV_VER_MAJ;
			data.offset =  dev->parent_dev->PCIEDEV_DRV_VER_MIN;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			break;
		case SIS8160_FIRMWARE_VERSION:
			tmp_data_32       = ioread32(address + 0);
			smp_rmb();
			data.data = tmp_data_32;
			printk (KERN_ALERT "SIS8160_FIRMWARE_VERSION: DATA %X:%X\n", tmp_data_32, data.data );
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				printk (KERN_ALERT "SIS8160_FIRMWARE_VERSION:ERROR COPY TO USER SPACE \n" );
				return retval;
			}
			break;
		case SIS8160_REG_READ:
			retval = 0;
			if (copy_from_user(&reg_data, (sis8160_reg*)arg, (size_t)reg_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset     = reg_data.offset*4;
			tmp_data       = reg_data.data;
			if (tmp_offset  > dev->rw_off[0]) {
				printk (KERN_ALERT "SIS8160_REG_READ: OUT OF MEM\n");
				mutex_unlock(&dev->dev_mut);
				return EFAULT;
			}
			tmp_data_32       = ioread32(address + tmp_offset);
			smp_rmb();
			udelay(2);
			reg_data.data = tmp_data_32;
			if (copy_to_user((sis8160_reg*)arg, &reg_data, (size_t)reg_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			break;
		case SIS8160_REG_WRITE:
			 retval = 0;
			if (copy_from_user(&reg_data, (sis8160_reg*)arg, (size_t)reg_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset     = reg_data.offset*4;
			tmp_data       = reg_data.data;
			if (tmp_offset >dev->rw_off[0]) {
				printk (KERN_ALERT "SIS8160_REG_WRITE: OUT OF MEM\n");
				mutex_unlock(&dev->dev_mut);
				return EFAULT;
			}
			tmp_data_32 = reg_data.data &  0xFFFFFFFF;
			iowrite32(tmp_data_32, ((void*)(address + tmp_offset)));
			smp_wmb();
			udelay(2);
			if (copy_to_user((sis8160_reg*)arg, &reg_data, (size_t)reg_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			break;
		 case SIS8160_BLINK_LED:
			 retval = 0;
			if (copy_from_user(&data, (sis8160_reg*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset     = SIS8160_USER_CONTROL_STATUS_REG*4;
			tmp_data       = data.data;
			if (!address) {
				printk (KERN_ALERT "SIS8300_REG_WRITE: NO MEM\n");
				mutex_unlock(&dev->dev_mut);
				return EFAULT;
			}

			if(reg_data.offset){
				iowrite32(0x1, ((void*)(address + tmp_offset)));
				smp_wmb();
				udelay(tmp_data);
				iowrite32(0x0, ((void*)(address + tmp_offset)));
				smp_wmb();
				udelay(2);
			}else{
				iowrite32(0x0, ((void*)(address + tmp_offset)));
				smp_wmb();
				udelay(tmp_data);
				iowrite32(0x1, ((void*)(address + tmp_offset)));
				smp_wmb();
				udelay(2);
			}
			break;
		/**
		* Aux In/OUT, sets/clear Output Enable (4)
		* Read: If Output Enable - Output Enable bit (4)
		* If Output not enabled - Aux Input Status (21)
		*
		* use struct device_ioctrl_data
		* @param offset		0 enable, 1 read input
		* @param data		not used
		* @param cmd		read/write (IOCTRL_R/IOCTRL_W)
		* @param reserved	not used 
		* @return      0 or negative error code
		**/
		case SIS8160_AUX_IO:
			 retval = 0;
			if (copy_from_user(&data, (sis8160_reg*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset     = data.offset;
			tmp_data       = data.data;
			tmp_cmd        = data.cmd; //R/W 1/0
			if(tmp_data >0) tmp_data = 1;
			if(tmp_data <=0) tmp_data = 0;
			
			if(tmp_cmd ==IOCTRL_R){
				tmp_data_32       = ioread32(address + SIS8160_USER_CONTROL_STATUS_REG*4);
				smp_rmb();
				udelay(2);
				if(tmp_offset){
					data.data    =(tmp_data_32>>21)&0x1;
				}else{
					data.data    =(tmp_data_32>>4)&0x1;
				}
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}else{
				if(tmp_data){
					tmp_data_32       = 0x10;
				}else{
					tmp_data_32       = 0x100000;
				}
				iowrite32(tmp_data_32, ((void*)(address + SIS8160_USER_CONTROL_STATUS_REG*4)));
				smp_wmb();
				udelay(2);
			}
			break;
		case SIS8160_AUX_IN_TERM:
			retval = 0;
			if (copy_from_user(&data, (sis8160_reg*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset     = data.offset;
			tmp_data       = data.data;
			tmp_cmd        = data.cmd; //R/W 1/0
			if(tmp_data >0) tmp_data = 1;
			if(tmp_data <=0) tmp_data = 0;
			
			if(tmp_cmd ==IOCTRL_R){
				tmp_data_32       = ioread32(address + SIS8160_USER_CONTROL_STATUS_REG*4);
				smp_rmb();
				udelay(2);
				data.data    =(tmp_data_32>>5)&0x1;
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}else{
				if(tmp_data){
					tmp_data_32       = 0x20;
				}else{
					tmp_data_32       = 0x200000;
				}
				iowrite32(tmp_data_32, ((void*)(address + SIS8160_USER_CONTROL_STATUS_REG*4)));
				smp_wmb();
				udelay(2);
			}
			break;
		case SIS8160_FMC_COMMAND:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; //FMC num
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; //R/W 1/0
			tmp_reserved = data.reserved;
			
			if(tmp_cmd==IOCTRL_R){
				tmp_data_32       = ioread32(address + SIS8160_GLOBAL_FMC_COMMAND_REG*4);
				smp_rmb();
				udelay(2);
				data.data    =tmp_data_32;
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}else{
				tmp_data_32       = tmp_data;
				iowrite32(tmp_data_32, ((void*)(address + SIS8160_GLOBAL_FMC_COMMAND_REG*4)));
				smp_wmb();
				udelay(2);
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		case SIS8160_GET_GLOBAL_TRIGGER:
			retval = 0;
			tmp_data_32       = ioread32(address + SIS8160_GLOBAL_FMC_CONTROL_REG*4);
			smp_rmb();
			udelay(2);
			data.data    =tmp_data_32;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		case SIS8160_SET_GLOBAL_TRIGGER:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset       = data.offset; //FMC num/ MLVDS num
			tmp_data         = data.data;   // data
			tmp_cmd         = data.cmd; //trigger source and means 
			tmp_reserved = data.reserved; // falling edge
			if(tmp_cmd > 5) tmp_cmd = 1;
			if(tmp_data)        tmp_data = 1;
			if(tmp_reserved)  tmp_reserved = 1;
			
			rd_data_32       = ioread32(address + SIS8160_GLOBAL_FMC_CONTROL_REG*4);
			smp_rmb();
			udelay(2);
			switch (tmp_cmd){
				case FMC_TRG_MLVDS:
					if(tmp_offset > 7) tmp_offset = 7;
					mask_data_32 = 0x1 << (tmp_offset  + 16 +8*tmp_reserved);
					tmp_data_32 = 0;
					if(tmp_data)
						tmp_data_32 = mask_data_32;
					rd_data_32       &= ~mask_data_32;
					tmp_data_32 += rd_data_32;
					break;
				case FMC_TRG_FMC1:
					if(tmp_offset >= 1) tmp_offset = 1;
					mask_data_32 = 0x1 <<tmp_reserved;
					tmp_data_32 = 0;
					if(tmp_data)
						tmp_data_32 = mask_data_32;
					rd_data_32       &= ~mask_data_32;
					tmp_data_32 += rd_data_32;
					break;
				case FMC_TRG_FMC2:
					if(tmp_offset >= 1) tmp_offset = 1;
					mask_data_32 = 0x1 <<( 2 + tmp_reserved);
					tmp_data_32 = 0;
					if(tmp_data)
						tmp_data_32 = mask_data_32;
					rd_data_32       &= ~mask_data_32;
					tmp_data_32 += rd_data_32;
					break;
				case FMC_TRG_FMC1_TIME:
					if(tmp_offset >= 1) tmp_offset = 1;
					mask_data_32 = 0x1 <<( 8 + tmp_reserved);
					tmp_data_32 = 0;
					if(tmp_data)
						tmp_data_32 = mask_data_32;
					rd_data_32       &= ~mask_data_32;
					tmp_data_32 += rd_data_32;
					break;
				case FMC_TRG_FMC2_TIME:
					if(tmp_offset >= 1) tmp_offset = 1;
					mask_data_32 = 0x1 <<( 10 + tmp_reserved);
					tmp_data_32 = 0;
					if(tmp_data)
						tmp_data_32 = mask_data_32;
					rd_data_32       &= ~mask_data_32;
					tmp_data_32 += rd_data_32;
					break;
				default:
					retval = -ENOTTY;
					break;
			}
			iowrite32(tmp_data_32, ((void*)(address + SIS8160_GLOBAL_FMC_CONTROL_REG*4)));
			smp_wmb();
			udelay(2);
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		case SIS8160_GET_MLVDS_INPUT:
			retval = 0;
			tmp_data_32       = ioread32(address + SIS8160_MLVDS_IO_CONTROL_REG*4);
			smp_rmb();
			udelay(2);
			data.data    =(tmp_data_32 >> 16) &0xFF;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		case SIS8160_GET_MLVDS_OUTPUT:
			retval = 0;
			tmp_data_32       = ioread32(address + SIS8160_MLVDS_IO_CONTROL_REG*4);
			smp_rmb();
			udelay(2);
			data.data    =tmp_data_32  &0xFF;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		case SIS8160_SET_MLVDS_OUTPUT:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			if(tmp_offset > 7) tmp_offset = 7;
			
			tmp_data_32       = ioread32(address + SIS8160_MLVDS_IO_CONTROL_REG*4);
			smp_rmb();
			udelay(2);
			mask_data_32 = (0x1 << tmp_offset);
			if(tmp_data){
				tmp_data_32  |= mask_data_32;
			}else{
				tmp_data_32  &= ~mask_data_32;
			}
			iowrite32(tmp_data_32, ((void*)(address + SIS8160_MLVDS_IO_CONTROL_REG*4)));
			smp_wmb();
			udelay(2);
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		case SIS8160_ENABLE_MLVDS_OUTPUT:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			if(tmp_offset > 7) tmp_offset = 7;
			
			tmp_data_32       = ioread32(address + SIS8160_MLVDS_IO_CONTROL_REG*4);
			smp_rmb();
			udelay(2);
			rd_data_32 = (tmp_data_32 >> 8) & 0xFF;
			
			if(tmp_cmd==IOCTRL_W){
				mask_data_32 = 0x1 << tmp_offset;
				if(tmp_data){
					rd_data_32  |= mask_data_32;
				}else{
					rd_data_32  &= ~mask_data_32;
				}
				tmp_data_32 &= 0xFFFF00FF;
				tmp_data_32 += (rd_data_32 << 8);
				iowrite32(tmp_data_32, ((void*)(address + SIS8160_MLVDS_IO_CONTROL_REG*4)));
				smp_wmb();
				udelay(2);
				tmp_data_32       = ioread32(address + SIS8160_MLVDS_IO_CONTROL_REG*4);
				smp_rmb();
				udelay(2);
			}
			data.data    = (tmp_data_32 >> 8)  &0xFF;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* sends Synch command
		*
		* use struct device_ioctrl_data
		* @param offset		0-FMCx_LA_22 or 1-FMCx_LA_11
		* @param data		not used
		* @param cmd		not used
		* @param reserved	not used 
		* @return      0 or negative error code
		**/
		case SIS8160_SYNCH_COMMAND:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			if(tmp_offset >=1) tmp_offset = 1;
			if(tmp_offset <=0) tmp_offset = 0;
			iowrite32((0x1 <<tmp_offset ), ((void*)(address + SIS8160_SYNCH_COMMAND_REG*4)));
			smp_wmb();
			udelay(2);
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		
		/**
		* select multiplexer input
		*
		* use struct device_ioctrl_data
		* @param offset		select multiplexer (0-A, 1-B, 2-C, 3-D)
		* @param data		data
		* @param cmd		0-set 1-get
		* @param reserved	not used 
		* @return      0 or negative error code
		**/
		case SIS8160_CLOCK_MP_CONTROL:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_offset >=4) tmp_offset = 4;
			if(tmp_offset <=0) tmp_offset = 0;
			if(tmp_data >=1) tmp_data = 1;
			if(tmp_data <=0) tmp_data = 0;
			if(tmp_data ==2) tmp_data = 0;
			
			rd_data_32       = ioread32(address + SIS8160_CLOCK_DISTRIBUTION_MUX_REG*4);
			smp_rmb();
			udelay(2);
			
			if(tmp_cmd==IOCTRL_W){
				rd_data_32 = rd_data_32 & ~(0x1 <<tmp_offset);
				tmp_data_32 = rd_data_32 + (tmp_data << tmp_offset);

				iowrite32(tmp_data_32, ((void*)(address + SIS8160_CLOCK_DISTRIBUTION_MUX_REG*4)));
				smp_wmb();
				udelay(2);
			}else{
				rd_data_32 = (rd_data_32 >> tmp_offset) &0x1;
				data.data = rd_data_32;
			}
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
			
		//SFMC CTRL
		/**
		* user LED control
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		not used
		* @param reserved	for all SFMC ioctrl used as FMC num 0-FMC1 1-FMC2
		* @return      0 or negative error code
		**/
		case SFMC_USER_LED:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_CONTROL1_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_CONTROL1_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			if(tmp_data >= 1){ 
				tmp_data = 1;
				tmp_data_32 =  0x1;
			}
			if(tmp_data <= 0){
				tmp_data = 0;
				tmp_data_32 =  0x00010000;
			}
			iowrite32(tmp_data_32, ((void*)(address )));
			smp_wmb();
			udelay(2);
			
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get Trigger termination
		*
		* use struct device_ioctrl_data
		* @param offset		not used for set, get: return status trigger in
		* @param data		data, return status of the trigger  termination
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2
		* @return      0 or negative error code
		**/
		case SFMC_TRG_TERMINATION:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_CONTROL1_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_CONTROL1_REG*4;
			}
						
			if(tmp_cmd==IOCTRL_W){
				if(tmp_data >= 1){ 
					tmp_data = 1;
					tmp_data_32 =  0x10;
				}
				if(tmp_data <= 0){
					tmp_data = 0;
					tmp_data_32 =  0x100000;
				}
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}else{
				rd_data_32       = ioread32(address );
				smp_rmb();
				data.offset = (rd_data_32 >>20) & 0x1;
				data.data= (rd_data_32 >>4) & 0x1;
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get ADC Power Up
		*
		* use struct device_ioctrl_data
		* @param offset		not used 
		* @param data		data, return status of the Power Up
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2
		* @return      0 or negative error code
		**/
		case SFMC_POWER_UP:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_CONTROL2_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_CONTROL2_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				if(tmp_data >= 1){ 
					tmp_data = 1;
					tmp_data_32 =  0x100;
				}
				if(tmp_data <= 0){
					tmp_data = 0;
					tmp_data_32 =  0x1000000;
				}
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}else{
				rd_data_32       = ioread32(address );
				smp_rmb();
				data.data= (rd_data_32 >>8) & 0x1;
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get Clock23 select Clock2
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data, return status of Clock23 select 2
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2
		* @return      0 or negative error code
		**/
		case SFMC_CLOCK23_SELECT2:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_CONTROL2_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_CONTROL2_REG*4;
			}
			
			if(tmp_cmd==IOCTRL_W){
				if(tmp_data >= 1){ 
					tmp_data = 1;
					tmp_data_32 =  0x20;
				}
				if(tmp_data <= 0){
					tmp_data = 0;
					tmp_data_32 =  0x200000;
				}
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}else{
				rd_data_32       = ioread32(address );
				smp_rmb();
				data.data= (rd_data_32 >>5) & 0x1;
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get Clock23 enable
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data, return status of Clock23 enable
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2
		* @return      0 or negative error code
		**/
		case SFMC_CLOCK23_ENABLE:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_CONTROL2_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_CONTROL2_REG*4;
			}
			
			if(tmp_cmd==IOCTRL_W){
				if(tmp_data >= 1){ 
					tmp_data = 1;
					tmp_data_32 =  0x10;
				}
				if(tmp_data <= 0){
					tmp_data = 0;
					tmp_data_32 = 0x100000;
				}
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}else{
				rd_data_32       = ioread32(address );
				smp_rmb();
				data.data= (rd_data_32 >>4) & 0x1;
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get HMC Clock IC Power Up
		*
		* use struct device_ioctrl_data
		* @param offset		not used 
		* @param data		data, return status of the HMC Clock IC Power Up
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2
		* @return      0 or negative error code
		**/
		case SFMC_HMC_CLOCK_PU:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_CONTROL2_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_CONTROL2_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				if(tmp_data >= 1){ 
					tmp_data = 1;
					tmp_data_32 =  0x1;
				}
				if(tmp_data <= 0){
					tmp_data = 0;
					tmp_data_32 =  0x10000;
				}
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}else{
				rd_data_32       = ioread32(address );
				smp_rmb();
				data.data= (rd_data_32) & 0x1;
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get ADC Acquisition CS register
		*
		* use struct device_ioctrl_data
		* @param offset		set: not used, get: return Memory Bank 2 Busy 
		* @param data		set: command data, get: Memory Bank 1 Busy 
		* @param cmd		0- set 1- get, get: return Sampling Busy
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_COMMAND:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_ACQ_SC_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_ACQ_SC_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 = tmp_data;
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = rd_data_32  & 0xFF;
			//data.offset = (rd_data_32 >> 3) & 0x1;
			//data.data = (rd_data_32 >> 2) & 0x1;
			//data.cmd = (rd_data_32 >>1) & 0x1;
			//data.reserved = (rd_data_32) & 0x1;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get ADC Sample Control register Enable Header
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_ENABLE_HEADER:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				if(tmp_data >= 1){ 
					tmp_data = 1;
					tmp_data_32 =  rd_data_32 | 0x1000;
				}
				if(tmp_data <= 0){
					tmp_data = 0;
					tmp_data_32 =  rd_data_32 & 0xFFFFEFFF;
				}
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = (rd_data_32 >> 12) & 0x1;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get ADC Sample Control register Store Mode
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_SAMPLE_STORE_MODE:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_offset >=1) tmp_offset = 1;
			if(tmp_offset <1) tmp_offset = 0;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				if(tmp_offset){
					tmp_data_32 =  rd_data_32 & 0xFFFFF3FF;
					tmp_data_32 = tmp_data_32 + ((tmp_data <<10));
				}else{
					tmp_data_32 =  rd_data_32 & 0xFFFFFCFF;
					tmp_data_32 = tmp_data_32 + ((tmp_data <<8));
				}
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = (rd_data_32 >> (8 +2*tmp_offset)) & 0x3;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get ADC Sample Control register Decimation/Average
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_SAMPLE_DECIMATION:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  rd_data_32 & 0xFFFFFF8F;
				tmp_data_32 = tmp_data_32 + ((tmp_data <<4));
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = (rd_data_32 >> 4) & 0x7;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get ADC Sample Control register Decimation/Average Signed
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_SAMPLE_DECIMATION_SIGNED:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  rd_data_32 & 0xFFFFFF7F;
				tmp_data_32 = tmp_data_32 + ((tmp_data <<7));
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = (rd_data_32 >> 7) & 0x1;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get ADC Sample Control register External Trigger Delay
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_EXTTRG_DELAY_ENABLE:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  rd_data_32 & 0xFFFFFFFB;
				tmp_data_32 = tmp_data_32 + ((tmp_data <<2));
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = (rd_data_32 >> 2) & 0x1;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get ADC Sample Control register External Trigger Falling Edge
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_EXTTRG_FALLING_EDGE:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  rd_data_32 & 0xFFFFFFFD;
				tmp_data_32 = tmp_data_32 + ((tmp_data <<1));
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = (rd_data_32 >> 1) & 0x1;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get ADC Sample Control register External Trigger Rising Edge
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_EXTTRG_RISING_EDGE:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_SAMPLE_SC_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  rd_data_32 & 0xFFFFFFFE;
				tmp_data_32 = tmp_data_32 + ((tmp_data &0x1));
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = rd_data_32 & 0x1;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get Sample Start Address, given in blocks of 512bit-64B
		*
		* use struct device_ioctrl_data
		* @param offset		ADC number 0-A , 1-B
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_SAMPLE_START_ADDRESS:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			//if(tmp_data%0x20) tmp_data +=tmp_data%20;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADCA_SAMPLE_ADDRESS_REG*4 +tmp_offset*4 ;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADCA_SAMPLE_ADDRESS_REG*4 +tmp_offset*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  tmp_data;
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = rd_data_32 & 0x1FFFFFF;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get Sample Start Lrnght, given in blocks of 512bit-64B
		*
		* use struct device_ioctrl_data
		* @param offset		ADC number 0-A , 1-B
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_SAMPLE_LENGHT:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			//if(tmp_data%0x20) tmp_data +=tmp_data%20;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_SAMPLE_LENGHT_REG*4 ;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_SAMPLE_LENGHT_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  tmp_data;
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = rd_data_32 & 0x1FFFFFF;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get Pre Trigger Delay in Samples, data*8 samples
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_PRETRG_DELAY:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			tmp_data =tmp_data /8;
			tmp_data =tmp_data & 0xFFFF;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_PRETRG_DELAY_REG*4 ;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_PRETRG_DELAY_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  tmp_data;
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = (rd_data_32 & 0xFFFF)*8;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get Acquisition Number of Events
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_EVENT_NUMBER:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			tmp_data =tmp_data & 0xFFFFFF;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_MAX_NOF_EVENT_REG*4 ;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_MAX_NOF_EVENT_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  tmp_data;
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = rd_data_32 & 0xFFFFFF;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* set/get ADC Extyernal Trigger delay in Samples (Samples is Register-Value * 8)
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set 1- get
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_EXTTRG_DELAY:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			tmp_data =tmp_data /8;
			tmp_data =tmp_data & 0xFFFFFFF;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_TRG_DELAY_REG*4 ;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 + SFMC_ADC_TRG_DELAY_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  tmp_data;
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(2);
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = (rd_data_32 & 0xFFFFFFF)*8;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* get Memory Actual Sample Block Address , Block is 32Samples (512-bit:64B)
		*
		* use struct device_ioctrl_data
		* @param offset		ADC 0-A 1-B
		* @param data		data
		* @param cmd		0- set Read only register
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_ACTUAL_ADDRESS:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_offset >0)    tmp_offset = 1;
			if(tmp_offset <=0) tmp_offset = 0;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + (SFMC_ADCA_ACTUAL_ADDRESS_REG + tmp_offset) *4 ;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 +  (SFMC_ADCA_ACTUAL_ADDRESS_REG + tmp_offset)*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = (rd_data_32 & 0xFFFFFF)*64;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		/**
		* get Event Counter
		*
		* use struct device_ioctrl_data
		* @param offset		not used
		* @param data		data
		* @param cmd		0- set Read only register
		* @param reserved	for all SFMC ioctrl used as FMC number 0-FMC1 1-FMC2, get: return Armend
		* @return      0 or negative error code
		**/
		case SFMC_EVENT_COUNTER:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // channel
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; 
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_EVENT_COUNTER_REG *4 ;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 +  SFMC_ADC_EVENT_COUNTER_REG*4;
			}
			
			rd_data_32       = ioread32(address );
			smp_rmb();
			data.data = (rd_data_32 & 0xFFFFFF);
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
			
		case SFMC_HMC_SPI_RW:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // spi address
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; //   IOCTRL_R   IOCTRL_W
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_HMC_SPI_REG *4 ;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 +  SFMC_HMC_SPI_REG*4;
			}
			//printk (KERN_ALERT "========SFMC_HMC_SPI_RW COMMAND %i: SFMC %i:  DATA %X: ADDRESS %X\n", 
			//		tmp_cmd, tmp_reserved, tmp_data, address);
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  ((tmp_offset & 0x3fff) << 8) + (tmp_data & 0xff);
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(10);
			}else{
				tmp_data_32 =  ((tmp_offset & 0x3fff) << 8) + 0x800000 ;
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(10);
				rd_data_32       = ioread32(address );
				smp_rmb();
				data.data = (rd_data_32 & 0xFF);
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}
			udelay(20);
			//printk (KERN_ALERT "========SFMC_HMC_SPI_RW DONE\n");
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		case SFMC_ADC_SPI_RW:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // spi address
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; //   IOCTRL_R   IOCTRL_W
			tmp_reserved = data.reserved;
			
			if(tmp_reserved >= 1){ 
				tmp_reserved = 1;
				address += SIS8160_FMC2_SPACE_REG*4 + SFMC_ADC_SPI_REG *4 ;
			}
			if(tmp_reserved <= 0){ 
				tmp_reserved = 0;
				address += SIS8160_FMC1_SPACE_REG*4 +  SFMC_ADC_SPI_REG*4;
			}
			
			//printk (KERN_ALERT "========SFMC_ADC_SPI_RW COMMAND %i: SFMC %i:  DATA %X: ADDRESS %X\n", 
			//		tmp_cmd, tmp_reserved, tmp_data, address);
			
			if(tmp_cmd==IOCTRL_W){
				tmp_data_32 =  ((tmp_offset & 0x3fff) << 8) + (tmp_data & 0xff);
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(100);
			}else{
				tmp_data_32 =  ((tmp_offset & 0x3fff) << 8) + 0x800000 ;
				iowrite32(tmp_data_32, ((void*)(address )));
				smp_wmb();
				udelay(100);
				rd_data_32       = ioread32(address );
				smp_rmb();
				data.data = rd_data_32 ;
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}
			udelay(20);
			//printk (KERN_ALERT "========SFMC_ADC_SPI_RW DONE\n");
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		case SIS8160_SI5326_SPI_RW:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // spi address
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; //   IOCTRL_R   IOCTRL_W
			tmp_reserved = data.reserved;
			
			address += SIS8160_CLOCK_MULTIPLIER_SPI_REG*4 ;
			
			//printk (KERN_ALERT "========SIS8160_SI5326_SPI_RW COMMAND %i:  DATA %X: ADDRESS %X\n", 
			//		tmp_cmd, tmp_data, address);
			
			if(tmp_cmd==IOCTRL_W){
				// write address
				write_data = 0x0000 + (tmp_offset & 0xff) ; // write ADDR Instruction + register addr
				iowrite32(write_data, ((void*)(address )));
				smp_wmb();
				udelay(10);
				poll_counter = 0 ;
				do{
					poll_counter++;
					read_data       = ioread32(address );
					smp_rmb();
				}
				while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5326_SPI_POLL_COUNTER_MAX)) ;
				if (poll_counter == SI5326_SPI_POLL_COUNTER_MAX){
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
				udelay(10) ;
				
				// write data
				write_data = 0x4000 + (tmp_data & 0xff) ; // write Instruction + data
				iowrite32(write_data, ((void*)(address )));
				smp_wmb();
				udelay(10) ;

				poll_counter = 0 ;
				do{
					poll_counter++;
					read_data       = ioread32(address );
					smp_rmb();
				}
				while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5326_SPI_POLL_COUNTER_MAX)) ;
				if (poll_counter == SI5326_SPI_POLL_COUNTER_MAX){
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}else{
				// read address
				write_data = 0x0000 + (tmp_offset & 0xff) ; // read ADDR Instruction + register addr
				iowrite32(write_data, ((void*)(address )));
				smp_wmb();
				udelay(10) ;

				poll_counter = 0 ;
				do{
					poll_counter++;
					read_data       = ioread32(address );
					smp_rmb();
				}
				while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5326_SPI_POLL_COUNTER_MAX)) ;
				if (poll_counter == SI5326_SPI_POLL_COUNTER_MAX){
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
				udelay(10) ;
				// read data
				write_data = 0x8000  ; // read Instruction + data
				iowrite32(write_data, ((void*)(address )));
				smp_wmb();
				udelay(10) ;

				poll_counter = 0 ;
				do{
					poll_counter++;
					read_data       = ioread32(address );
					smp_rmb();
				}
				while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5326_SPI_POLL_COUNTER_MAX)) ;
				
				// "read data" cmd a second time -> observe abnormal behavior of the si5326
				write_data = 0x8000  ; // read Instruction + data
				iowrite32(write_data, ((void*)(address )));
				smp_wmb();
				udelay(10) ;

				poll_counter = 0 ;
				do{
					poll_counter++;
					read_data       = ioread32(address );
					smp_rmb();
				}
				while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5326_SPI_POLL_COUNTER_MAX)) ;

				if (poll_counter == SI5326_SPI_POLL_COUNTER_MAX){
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
				udelay(1000) ;
				data.data = read_data;
				if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
			}
			udelay(20);
			//printk (KERN_ALERT "========SIS8160_SI5326_SPI_RW DONE\n");
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		case SIS8160_SI5326_CALIBRATION:
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset; // spi address
			tmp_data     = data.data;   //data
			tmp_cmd      = data.cmd; //   IOCTRL_R   IOCTRL_W
			tmp_reserved = data.reserved;
			
			address += SIS8160_CLOCK_MULTIPLIER_SPI_REG*4 ;
			
			//printk (KERN_ALERT "========SIS8160_SI5326_CALIBRATION COMMAND %i:  DATA %X: ADDRESS %X\n", 
			//		tmp_cmd, tmp_data, address);
			// write address
			write_data = 0x0000 + 136 ; // write ADDR Instruction + register addr
			iowrite32(write_data, ((void*)(address )));
			smp_wmb();
			udelay(10) ;

			poll_counter = 0 ;
			do{
				poll_counter++;
				read_data       = ioread32(address );
				smp_rmb();
			}
			while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5326_SPI_POLL_COUNTER_MAX)) ;
			if (poll_counter == SI5326_SPI_POLL_COUNTER_MAX){
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			udelay(10) ;
			// write data
			write_data = 0x4000 + 0x40 ; // write Instruction + data
			iowrite32(write_data, ((void*)(address )));
			smp_wmb();
			udelay(10) ;

			poll_counter = 0 ;
			do{
				poll_counter++;
				read_data       = ioread32(address );
				smp_rmb();
			}
			while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5326_SPI_POLL_COUNTER_MAX)) ;
			if (poll_counter == SI5326_SPI_POLL_COUNTER_MAX){
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			udelay(10) ;
			// poll until Calibration is ready
			cal_poll_counter = 0 ;
			do{
				cal_poll_counter++;
				// read data
				write_data = 0x8000  ; // read Instruction + data
				iowrite32(write_data, ((void*)(address )));
				smp_wmb();
				udelay(10) ;

				poll_counter = 0 ;
				do{
					poll_counter++;
					read_data       = ioread32(address );
					smp_rmb();
				}
				while (((read_data & 0x80000000) == 0x80000000) && (poll_counter < SI5326_SPI_POLL_COUNTER_MAX)) ;
				if (poll_counter == SI5326_SPI_POLL_COUNTER_MAX){
					retval = -EFAULT;
					mutex_unlock(&dev->dev_mut);
					return retval;
				}
				udelay(10) ;

			}
			while (((read_data & 0x40) == 0x40) && (cal_poll_counter < SI5326_SPI_CALIBRATION_READY_POLL_COUNTER_MAX)) ;
			if (cal_poll_counter == SI5326_SPI_CALIBRATION_READY_POLL_COUNTER_MAX){
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			//printk (KERN_ALERT "========SIS8160_SI5326_CALIBRATION DONE\n");
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
			
		case SIS8160_MASTER_RESET:
			retval = 0;
			iowrite32(0x1, ((void*)(address + SIS8160_MASTER_RESET_REG*4)));
			smp_wmb();
			udelay(2);
			mutex_unlock(&dev->dev_mut);
			return retval;
			break;
		case PCIEDEV_GET_DMA_TIME:
		case SIS8160_GET_DMA_TIME:
			retval = 0;
			if (copy_from_user(&time_data, (device_ioctrl_time*)arg, (size_t)time_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			time_data.start_time = sis8160dev->dma_start_time;
			time_data.stop_time  = sis8160dev->dma_stop_time;
			if (copy_to_user((device_ioctrl_time*)arg, &time_data, (size_t)time_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			break;
		 case PCIEDEV_READ_DMA:
		 case SIS8160_READ_DMA:
			retval = 0;
			if (copy_from_user(&dma_data, (device_ioctrl_dma*)arg, (size_t)io_dma_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				printk (KERN_ALERT "SIS8160_READ_DMA: COULD NOT COPY FROM USER\n");
				return retval;
			}
			tmp_dma_size          = dma_data.dma_size;
			tmp_dma_offset        = dma_data.dma_offset;
			tmp_reserved            = dma_data.dma_reserved1;
			if(tmp_reserved){
				tmp_reserved = 0x1;
			}else{
				tmp_reserved =0x0;
			}
			sis8160dev->dev_dma_size     = tmp_dma_size;
			 if(tmp_dma_size <= 0){
				 printk (KERN_ALERT "SIS8160_READ_DMA: SIZE 0 tmp_dma_size %d\n", tmp_dma_size);
				 mutex_unlock(&dev->dev_mut);
				 return EFAULT;
			}

			tmp_data_32       = ioread32(address + 0); // be safe all writes are done
			tmp_dma_trns_size    = tmp_dma_size;
			if((tmp_dma_size%SIS8160_DMA_SYZE)){
				tmp_dma_trns_size    = tmp_dma_size + (tmp_dma_size%SIS8160_DMA_SYZE);
			}
			value  = 10000*HZ/200000; //value is given in jiffies
			//value    = HZ/1;                // value is given in jiffies
			length   = tmp_dma_size;
			tmp_order = get_order(tmp_dma_trns_size);
			sis8160dev->dma_page_order = tmp_order;
			pWriteBuf = (void *)__get_free_pages(GFP_KERNEL | __GFP_DMA, tmp_order);
			pTmpDmaHandle      = pci_map_single(pdev, pWriteBuf, tmp_dma_trns_size, PCI_DMA_FROMDEVICE);

			tmp_source_address = tmp_dma_offset;
			dma_sys_addr       = (u32)(pTmpDmaHandle & 0xFFFFFFFF);
			iowrite32(tmp_source_address, ((void*)(address + SIS8160_DMA_READ_SRC_ADR_LO32*4)));

			tmp_data_32         = dma_sys_addr;
			iowrite32(tmp_data_32, ((void*)(address + SIS8160_DMA_READ_DST_ADR_LO32*4)));
			smp_wmb();
			dma_sys_addr       = (u32)((pTmpDmaHandle >> 32) & 0xFFFFFFFF);
			tmp_data_32         = dma_sys_addr;
			iowrite32(tmp_data_32, ((void*)(address + SIS8160_DMA_READ_DST_ADR_HI32*4)));
			smp_wmb();
			iowrite32(tmp_dma_trns_size, ((void*)(address + SIS8160_DMA_READ_LEN*4)));
			smp_wmb();
			iowrite32(tmp_reserved, ((void*)(address + SIS8160_DMA_READ_SPACE_SELECT*4)));
			smp_wmb();
			tmp_data_32       = ioread32(address + SIS8160_IRQ_ENABLE_REG*4);
			smp_rmb();
			tmp_data_32      &= 0x8000;
			tmp_data_32       += 1;
			iowrite32(tmp_data_32, ((void*)(address + SIS8160_IRQ_ENABLE_REG*4)));
			smp_wmb();
			tmp_data_32       = ioread32(address + 0); // be safe all writes are done
			smp_rmb();
			do_gettimeofday(&(sis8160dev->dma_start_time));
			sis8160dev->waitFlag = 0;
			iowrite32(1, ((void*)(address + SIS8160_DMA_READ_CTRL*4)));
			timeDMAwait = wait_event_interruptible_timeout( sis8160dev->waitDMA, sis8160dev->waitFlag != 0, value );
			do_gettimeofday(&(sis8160dev->dma_stop_time));
			if(!sis8160dev->waitFlag){
				printk (KERN_ALERT "SIS8160_READ_DMA:SLOT NUM %i NO INTERRUPT 0 \n", dev->slot_num);
				tmp_data_32       = ioread32((void*)(address + SIS8160_DMA_READ_CTRL*4)); 
				smp_rmb();
				 for(dma_done_count = 0; dma_done_count < 2000; ++dma_done_count){
					 printk (KERN_ALERT "SIS8160_READ_DMA:SLOT NUM %i NO INTERRUPT COUNT %i \n", dev->slot_num, dma_done_count);
					 udelay(20);
					tmp_data_32 = 0;
					tmp_data_32       = ioread32((void*)(address + SIS8160_DMA_READ_CTRL*4)); 
					smp_rmb();
					if(!(tmp_data_32 & 0x1)) break;
				}
				if(tmp_data_32 & 0x1){
					printk (KERN_ALERT "SIS8160_READ_DMA:SLOT NUM %i NO INTERRUPT \n", dev->slot_num);
					sis8160dev->waitFlag = 1;
					iowrite32(0xFFFFFFFF, ((void*)(address + SIS8160_IRQ_CLEAR_REG*4)));
					smp_wmb();
					udelay(5);
					pci_unmap_single(pdev, pTmpDmaHandle, tmp_dma_trns_size, PCI_DMA_FROMDEVICE);
					free_pages((ulong)pWriteBuf, (ulong)sis8160dev->dma_page_order);
					mutex_unlock(&dev->dev_mut);
					return EFAULT;
				}    
			}
			pci_unmap_single(pdev, pTmpDmaHandle, tmp_dma_trns_size, PCI_DMA_FROMDEVICE);
			if (copy_to_user ((void *)arg, pWriteBuf, tmp_dma_size)) {
				retval = -EFAULT;
				printk (KERN_ALERT "SIS8160_READ_DMA: SLOT NUM %i COULD NOT COPY TO USER\n", dev->slot_num);
			}
			free_pages((ulong)pWriteBuf, (ulong)sis8160dev->dma_page_order);
			iowrite32(0xFFFFFFFF, ((void*)(address + SIS8160_IRQ_CLEAR_REG*4)));
			smp_wmb();
			tmp_data_32       = ioread32(address + SIS8160_IRQ_ENABLE_REG*4);
			smp_rmb();
			tmp_data_32      &= 0x8000;
			iowrite32(tmp_data_32, ((void*)(address + SIS8160_IRQ_ENABLE_REG*4)));
			smp_wmb();
			udelay(5);
			break;
		case PCIEDEV_WRITE_DMA:
		case SIS8160_WRITE_DMA:
			retval = 0;
			if (copy_from_user(&dma_data, (device_ioctrl_dma*)arg, (size_t)io_dma_size)) {
				retval = -EFAULT;
				mutex_unlock(&dev->dev_mut);
				printk (KERN_ALERT "SIS8160_WRITE_DMA: COULD NOT COPY FROM USER\n");
				return retval;
			}
			// value  = 10000*HZ/20000; // value is given in jiffies
			value    = HZ/1; // value is given in jiffies
			tmp_dma_size           = dma_data.dma_size;
			tmp_dma_offset        = dma_data.dma_offset;
			tmp_reserved            = dma_data.dma_reserved1;
			if(tmp_reserved){
				tmp_reserved = 0x1;
			}else{
				tmp_reserved =0x0;
			}
			if(tmp_dma_size <= 0){
				 printk (KERN_ALERT "SIS8160_WRITE_DMA: tmp_dma_size %d\n", tmp_dma_size);
				 mutex_unlock(&dev->dev_mut);
				 return EFAULT;
			}
			
			tmp_offset                    = 0;
			tmp_dma_trns_size       = tmp_dma_size;
			if((tmp_dma_size%SIS8160_DMA_SYZE)){
				tmp_dma_trns_size    = tmp_dma_size + (tmp_dma_size%SIS8160_DMA_SYZE);
			}
			tmp_data_32       = ioread32(address + 0); // be safe all writes are done
			length   = tmp_dma_size;
			tmp_order = get_order(tmp_dma_trns_size);
			sis8160dev->dma_page_order = tmp_order;
			pWriteBuf = (void *)__get_free_pages(GFP_KERNEL | __GFP_DMA, tmp_order);
			tmp_source_address = tmp_dma_offset;
			if (copy_from_user(pWriteBuf, ((u_int*)arg + DMA_DATA_OFFSET), (size_t)length)) {
				retval = -EFAULT;
				free_pages((ulong)pWriteBuf, (ulong)sis8160dev->dma_page_order);
				mutex_unlock(&dev->dev_mut);
				return retval;
			}
			pTmpDmaHandle       = pci_map_single(pdev, pWriteBuf, tmp_dma_trns_size, PCI_DMA_TODEVICE);
			tmp_source_address = tmp_dma_offset;
			iowrite32(tmp_source_address, ((void*)(address + SIS8160_DMA_WRITE_DST_ADR_LO32*4)));
			dma_sys_addr       = (u32)(pTmpDmaHandle & 0xFFFFFFFF);
			iowrite32(dma_sys_addr, ((void*)(address + SIS8160_DMA_WRITE_SRC_ADR_LO32*4)));
			dma_sys_addr       = (u32)((pTmpDmaHandle >> 32) & 0xFFFFFFFF);
			iowrite32(dma_sys_addr, ((void*)(address + SIS8160_DMA_WRITE_SRC_ADR_HI32*4)));
			iowrite32(tmp_dma_trns_size, ((void*)(address + SIS8160_DMA_WRITE_LEN*4)));
			iowrite32(tmp_reserved, ((void*)(address + SIS8160_DMA_WRITE_SPACE_SELECT*4)));
			tmp_data_32       = ioread32(address + SIS8160_IRQ_ENABLE_REG*4);
			smp_rmb();
			tmp_data_32      &= 0x8000;
			tmp_data_32       += 2;
			iowrite32(tmp_data_32, ((void*)(address + SIS8160_IRQ_ENABLE_REG*4)));
			smp_wmb();
			tmp_data_32       = ioread32(address + 0); // be safe all writes are done
			smp_rmb();
			do_gettimeofday(&(sis8160dev->dma_start_time));
			sis8160dev->waitFlag = 0;
			iowrite32(1, ((void*)(address + SIS8160_DMA_WRITE_CTRL*4)));
			timeDMAwait = wait_event_interruptible_timeout( sis8160dev->waitDMA, sis8160dev->waitFlag != 0, value );
			do_gettimeofday(&(sis8160dev->dma_stop_time));
			if(!sis8160dev->waitFlag){
				printk (KERN_ALERT "SIS8160_WRITE_DMA: NO INTERRUPT\n");
				tmp_data_32       = ioread32((void*)(address + SIS8160_DMA_WRITE_CTRL*4)); 
				smp_rmb();
				for(dma_done_count = 0; dma_done_count < 2000; ++dma_done_count){
					printk (KERN_ALERT "SIS8160_WRITE_DMA:SLOT NUM %i NO INTERRUPT COUNT %i \n", dev->slot_num, dma_done_count);
					udelay(20);
					tmp_data_32 = 0;
					tmp_data_32       = ioread32((void*)(address + SIS8160_DMA_WRITE_CTRL*4)); 
					smp_rmb();
					if(!(tmp_data_32 & 0x1)) break;
				}
				if(tmp_data_32 & 0x1){
					printk (KERN_ALERT "SIS8160_WRITE_DMA:SLOT NUM %i NO INTERRUPT \n", dev->slot_num);
					sis8160dev->waitFlag = 1;
					iowrite32(0xFFFFFFFF, ((void*)(address + SIS8160_IRQ_CLEAR_REG*4)));
					smp_wmb();
					udelay(5);
					pci_unmap_single(pdev, pTmpDmaHandle, tmp_dma_trns_size, PCI_DMA_TODEVICE);
					free_pages((ulong)pWriteBuf, (ulong)sis8160dev->dma_page_order);
					mutex_unlock(&dev->dev_mut);
					return EFAULT;
				}  
			}
			pci_unmap_single(pdev, pTmpDmaHandle, tmp_dma_trns_size, PCI_DMA_TODEVICE);
			free_pages((ulong)pWriteBuf, (ulong)sis8160dev->dma_page_order);
			iowrite32(0xFFFFFFFF, ((void*)(address + SIS8160_IRQ_CLEAR_REG*4)));
			smp_wmb();
			tmp_data_32       = ioread32(address + SIS8160_IRQ_ENABLE_REG*4);
			smp_rmb();
			tmp_data_32      &= 0x8000;
			iowrite32(tmp_data_32, ((void*)(address + SIS8160_IRQ_ENABLE_REG*4)));
			smp_wmb();
			udelay(2);
			break;
		default:
			mutex_unlock(&dev->dev_mut);
		return -ENOTTY;
		break;
	}
	mutex_unlock(&dev->dev_mut);

	return retval;
}
