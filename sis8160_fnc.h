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


#ifndef _SIS8160_FNC_H_
#define _SIS8160_FNC_H_

#include "pciedev_io.h"
#include "pciedev_ufn.h"

#ifndef SIS8160_NR_DEVS
#define SIS8160_NR_DEVS 15  /* sis83000 through sis830011 */
#endif

#define SIS8160DEVNAME "sis8160"	                    /* name of device */
#define SIS8160_VENDOR_ID 0x1796	/* FZJZEL vendor ID */
#define SIS8160_DEVICE_ID 0x0028	/* SIS8160 dev board device ID */

struct sis8160_dev {
    int                              brd_num;
    struct timeval          dma_start_time;
    struct timeval          dma_stop_time;
    int                              waitFlag;
    u32                            dev_dma_size;
    u32                            dma_page_num;
    int                              dma_offset;
    int                              dma_page_order;
    wait_queue_head_t  waitDMA;
    struct pciedev_dev  *parent_dev;
};
typedef struct sis8160_dev sis8160_dev;

long sis8160_ioctl_dma(struct file *, unsigned int* , unsigned long* );

#endif /* _SIS8160_FNC_H_ */
