CFILES = GpioInit.c WiPiLike.c
MODULE = WiPiLikegpio_mod

obj-m += $(MODULE).o
$(MODULE)-objs := $(CFILES:.c=.o)

KERNEL_SRC = /home/pi/linux

all:
	make -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean
