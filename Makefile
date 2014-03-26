KVER := $(shell uname -r)
MAJMIN := $(shell echo $(KVER) | cut -d . -f 1-2)

ifeq ($(MAJMIN),3.10)

ifneq ($(KERNELRELEASE),)

#
# Make rules for use from within 2.6 kbuild system
#

linect-objs := linect-usb.o linect-v4l.o linect-sysfs.o linect-buf.o linect-bayer.o linect-procfs.o

linect-objs += linect-cam.o
linect-objs += linect-motor.o

obj-m	+= linect.o

else

KDIR	:= /usr/lib/modules/$(shell uname -r)/build
#KDIR	:= /lib/modules/3.10.33-1-ARCH/build

PWD	:= $(shell pwd)
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install: all
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

clean:
	rm -rf .*.cmd  *.mod.c *.ko *.o .tmp_versions Module.symvers *~ core *.i *.order
endif

endif

