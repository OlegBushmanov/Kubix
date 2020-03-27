obj-m += kubix.o
kubix-objs := kubix_main.o \
			  kbx_storage.o \
		 	  kbx_channel.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
