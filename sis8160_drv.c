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

#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/signal.h>

#include "sis8160_fnc.h"
#include "sis8160_defs.h"
#include "sis8160_reg.h"

MODULE_AUTHOR("Ludwig Petrosyan");
MODULE_DESCRIPTION("SIS8160 board driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("Dual BSD/GPL");

pciedev_cdev     *sis8160_cdev_m = 0;
sis8160_dev       *sis8160_dev_p[PCIEDEV_NR_DEVS];
sis8160_dev       *sis8160_dev_pp;

static int        sis8160_open( struct inode *inode, struct file *filp );
static int        sis8160_release(struct inode *inode, struct file *filp);
static ssize_t sis8160_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t sis8160_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static long     sis8160_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int        sis8160_remap_mmap(struct file *filp, struct vm_area_struct *vma);

struct file_operations sis8160_fops = {
	.owner                   =  THIS_MODULE,
	.read                     =  sis8160_read,
	.write                    =  sis8160_write,
	.unlocked_ioctl    =  sis8160_ioctl,
	.open                    =  sis8160_open,
	.release                =  sis8160_release,
	.mmap                 = sis8160_remap_mmap,
};

static struct pci_device_id sis8160_ids[] = {
	{ PCI_DEVICE(SIS8160_VENDOR_ID, SIS8160_DEVICE_ID), },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, sis8160_ids);

/*
 * The top-half interrupt handler.
 */
#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
static irqreturn_t sis8160_interrupt(int irq, void *dev_id, struct pt_regs *regs)
#else
static irqreturn_t sis8160_interrupt(int irq, void *dev_id)
#endif
{
	uint32_t intreg = 0;
	u32                     tmp_data_32 = 0;
	void*                   address;

	struct pciedev_dev *pdev   = (pciedev_dev*)dev_id;
	struct sis8160_dev *dev     = (sis8160_dev *)(pdev->dev_str);

	address = pciedev_get_baraddress(BAR0, pdev);
	intreg    = ioread32(address + SIS8160_IRQ_STATUS_REG*4);
	smp_rmb();

	//printk(KERN_ALERT "SIS8160-PCIEDEV_GET INT IRQ_STATUS %X \n", intreg);
	if(intreg == 0){
		return IRQ_NONE;
	}
	
	tmp_data_32 = intreg &0x00008003;
	iowrite32(tmp_data_32, ((void*)(address + SIS8160_IRQ_CLEAR_REG*4)));
	smp_wmb();
	
	if(dev->waitFlag){
		return IRQ_HANDLED;
	}
	if(intreg & 0x3){
		dev->waitFlag = 1;
		wake_up_interruptible(&(dev->waitDMA));
	}
	
	return IRQ_HANDLED;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
    static int sis8160_probe(struct pci_dev *dev, const struct pci_device_id *id)
#else 
static int __devinit sis8160_probe(struct pci_dev *dev, const struct pci_device_id *id)
#endif
{
	int result                = 0;
	int tmp_brd_num  = -1;
	u32 tmp_info          = 0;
	pciedev_dev       *sis8160_pcie_dev;
	void*                   address;

	printk(KERN_ALERT "SIS8160-PCIEDEV_PROBE CALLED \n");
	result = pciedev_probe_exp(dev, id, &sis8160_fops, sis8160_cdev_m, SIS8160DEVNAME, &tmp_brd_num);
	printk(KERN_ALERT "SIS8160-PCIEDEV_PROBE_EXP CALLED  FOR BOARD %i result %i\n", tmp_brd_num, result);
	if(!(sis8160_cdev_m->pciedev_dev_m[tmp_brd_num]->pciedev_all_mems)){
		printk(KERN_ALERT "SIS8160-PCIEDEV_PROBE CALLED; NO BARs \n");
		result = pciedev_remove_exp(dev,  sis8160_cdev_m, SIS8160DEVNAME, &tmp_brd_num);
		printk(KERN_ALERT "SIS8160-PCIEDEV_REMOVE_EXP CALLED  FOR SLOT %i\n", tmp_brd_num);  
		return -ENOMEM;
	}
	/*if board has created we will create our structure and pass it to pcedev_dev*/
	if(!result){
		printk(KERN_ALERT "SIS8160-PCIEDEV_PROBE_EXP CREATING CURRENT STRUCTURE FOR BOARD %i\n", tmp_brd_num);
		sis8160_pcie_dev = sis8160_cdev_m->pciedev_dev_m[tmp_brd_num];
		sis8160_dev_pp = kzalloc(sizeof(sis8160_dev), GFP_KERNEL);
		if(!sis8160_dev_pp){
				return -ENOMEM;
		}
		printk(KERN_ALERT "SIS8160-PCIEDEV_PROBE CALLED; CURRENT STRUCTURE CREATED \n");
		sis8160_dev_p[tmp_brd_num] = sis8160_dev_pp;
		sis8160_dev_pp->brd_num      = tmp_brd_num;
		sis8160_dev_pp->parent_dev  = sis8160_cdev_m->pciedev_dev_m[tmp_brd_num];
		init_waitqueue_head(&sis8160_dev_pp->waitDMA);
		pciedev_set_drvdata(sis8160_cdev_m->pciedev_dev_m[tmp_brd_num], sis8160_dev_p[tmp_brd_num]);
		pciedev_setup_interrupt(sis8160_interrupt, sis8160_cdev_m->pciedev_dev_m[tmp_brd_num], SIS8160DEVNAME, 0); 

		/*****Switch ON USER_LED*****/
		address = pciedev_get_baraddress(BAR0, sis8160_pcie_dev);
		iowrite32(0x1, (address + SIS8160_USER_CONTROL_STATUS_REG*4));
		smp_wmb();
		/*****Switch ON FMC USER_LED*****/
		iowrite32(0x1, (address + SIS8160_FMC1_SPACE_REG*4 + SFMC_CONTROL1_REG*4));
		smp_wmb();
		iowrite32(0x1, (address + SIS8160_FMC2_SPACE_REG*4 + SFMC_CONTROL1_REG*4));
		smp_wmb();
		/*Collect INFO*/
		tmp_info = ioread32(address + 0x0);
		smp_rmb();
		sis8160_cdev_m->pciedev_dev_m[tmp_brd_num]->brd_info_list.PCIEDEV_BOARD_ID = tmp_info;
		sis8160_cdev_m->pciedev_dev_m[tmp_brd_num]->brd_info_list.PCIEDEV_BOARD_VERSION = tmp_info & 0xFF;
		tmp_info = ioread32(address + 0x4);
		smp_rmb();
		sis8160_cdev_m->pciedev_dev_m[tmp_brd_num]->brd_info_list.PCIEDEV_HW_VERSION = tmp_info & 0xFFFF;
	 }
	return result;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
static void sis8160_remove(struct pci_dev *dev)
#else
static void __devexit sis8160_remove(struct pci_dev *dev)
#endif
{
    int result               = 0;
    int tmp_slot_num = -1;
    int tmp_brd_num = -1;
    printk(KERN_ALERT "SIS8160-REMOVE CALLED\n");
    tmp_brd_num =pciedev_get_brdnum(dev);
    printk(KERN_ALERT "SIS8160-REMOVE CALLED FOR BOARD %i\n", tmp_brd_num);
    /* clean up any allocated resources and stuff here */
    kfree(sis8160_dev_p[tmp_brd_num]);
    /*now we can call pciedev_remove_exp to clean all standard allocated resources
    will clean all interrupts if it seted 
    */
    result = pciedev_remove_exp(dev,  sis8160_cdev_m, SIS8160DEVNAME, &tmp_slot_num);
    printk(KERN_ALERT "SIS8160-PCIEDEV_REMOVE_EXP CALLED  FOR SLOT %i\n", tmp_slot_num);  
}

/****************************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
static struct pci_driver pci_sis8160_driver = {
    .name       = SIS8160DEVNAME,
    .id_table   = sis8160_ids,
    .probe      = sis8160_probe,
    .remove   = sis8160_remove,
};
#else
static struct pci_driver pci_sis8160_driver = {
    .name       = SIS8160DEVNAME,
    .id_table   = sis8160_ids,
    .probe      = sis8160_probe,
    .remove   = __devexit_p(sis8160_remove),
};
#endif

static int sis8160_open( struct inode *inode, struct file *filp )
{
    int    result = 0;
    result = pciedev_open_exp( inode, filp );
    return result;
}

static int sis8160_release(struct inode *inode, struct file *filp)
{
     int result            = 0;
     result = pciedev_release_exp(inode, filp);
     return result;
} 

static ssize_t sis8160_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
     ssize_t    retval         = 0;
    retval  = pciedev_read_exp(filp, buf, count, f_pos);
    return retval;
}

static ssize_t sis8160_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t         retval = 0;
    retval = pciedev_write_exp(filp, buf, count, f_pos);
    return retval;
}

static int sis8160_remap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	ssize_t         retval = 0;
	retval =pciedev_remap_mmap_exp(filp, vma);
	return 0;
}

static long  sis8160_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    
    if (_IOC_TYPE(cmd) == PCIEDOOCS_IOC){
        if (_IOC_NR(cmd) <= PCIEDOOCS_IOC_MAXNR && _IOC_NR(cmd) >= PCIEDOOCS_IOC_MINNR) {
			if (_IOC_NR(cmd) <= PCIEDOOCS_IOC_DMA_MAXNR && _IOC_NR(cmd) >= PCIEDOOCS_IOC_DMA_MINNR) {
                result = sis8160_ioctl_dma(filp, &cmd, &arg);
            }else{
            result = pciedev_ioctl_exp(filp, &cmd, &arg, sis8160_cdev_m);
		  }
        }else{
                if (_IOC_NR(cmd) <= SIS8160_IOC_MAXNR && _IOC_NR(cmd) >= SIS8160_IOC_MINNR) {
                        result = sis8160_ioctl_dma(filp, &cmd, &arg);
                    }else{
                        return -ENOTTY;
                    }
            }
    }else{
         return -ENOTTY;
    }
    return result;
}

static void __exit sis8160_cleanup_module(void)
{
    printk(KERN_NOTICE "SIS8160_CLEANUP_MODULE: PCI DRIVER UNREGISTERED\n");
    pci_unregister_driver(&pci_sis8160_driver);
    printk(KERN_NOTICE "SIS8160_CLEANUP_MODULE CALLED\n");
    upciedev_cleanup_module_exp(&sis8160_cdev_m);
    
}

static int __init sis8160_init_module(void)
{
    int   result = 0;
    
    printk(KERN_WARNING "SIS8160_INIT_MODULE CALLED\n");
    result = upciedev_init_module_exp(&sis8160_cdev_m, &sis8160_fops, SIS8160DEVNAME);
    result = pci_register_driver(&pci_sis8160_driver);
    printk(KERN_ALERT "SIS8160_INIT_MODULE:REGISTERING PCI DRIVER RESUALT %d\n", result);
    return 0; /* succeed */
}

module_init(sis8160_init_module);
module_exit(sis8160_cleanup_module);

