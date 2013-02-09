#Source: http://blog.markloiseau.com/2012/04/hello-world-loadable-kernel-module-tutorial/

MODULE=lfrng
SOURCE=$(MODULE).c
KERNEL_OBJ=$(MODULE).ko
OUT=$(MODULE).o
obj-m=$(OUT)
KDIR=/lib/modules/$(shell uname -r)/build
PWD=$(shell pwd)

.PHONY: all clean load unload

#default target
all: $(OUT)

$(MODULE).o: $(SOURCE) clean
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean: unload
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load: unload $(MODULE).o
	/sbin/insmod $(MODULE).ko

#hyphen means don't halt on error. in this case, that would likely be when the
#module isn't loaded, but we want to make sure
unload:
	-/sbin/rmmod $(MODULE)
