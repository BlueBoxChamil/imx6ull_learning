KERNELDIR := /home/bluebox/my_bluebox/my_zIamge
CURRENT_PATH := $(shell pwd)
obj-m := leddevice.o leddriver.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean