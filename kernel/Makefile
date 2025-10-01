obj-m += sensorhub_driver.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	sudo insmod sensorhub_driver.ko

uninstall:
	sudo rmmod sensorhub_driver

.PHONY: all clean install uninstall
