sis8160-objs := sis8160_drv.o sis8160_ioctl_dma.o
obj-m := sis8160.o

KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules
clean:
	test ! -d /lib/modules/$(KVERSION) || make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) clean

EXTRA_CFLAGS += -I/usr/include
#KBUILD_EXTRA_SYMBOLS += /home/petros/doocs/source/unixdriver/utca/linux/upciedev/Upciedev.symvers
KBUILD_EXTRA_SYMBOLS = /lib/modules/$(KVERSION)/upciedev/Upciedev.symvers
