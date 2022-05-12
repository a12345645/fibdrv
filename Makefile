CONFIG_MODULE_SIG = n
TARGET_MODULE := fibdrv

obj-m := $(TARGET_MODULE).o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

GIT_HOOKS := .git/hooks/applied

all: $(GIT_HOOKS) client
	$(MAKE) -C $(KDIR) M=$(PWD) modules

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) client out
load:
	sudo insmod $(TARGET_MODULE).ko
unload:
	sudo rmmod $(TARGET_MODULE) || true >/dev/null

replace:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	sudo rmmod $(TARGET_MODULE) || true >/dev/null
	sudo insmod $(TARGET_MODULE).ko

client: client.c
	$(CC) -o $@ $^

gnuplot:
	make client
	sudo taskset 0x1 ./client > out_nn
	gnuplot gnuplot_srcipt.gp

PRINTF = env printf
PASS_COLOR = \e[32;01m
NO_COLOR = \e[0m
pass = $(PRINTF) "$(PASS_COLOR)$1 Passed [-]$(NO_COLOR)\n"

check: all
	$(MAKE) unload
	$(MAKE) load
	sudo taskset 0x1 ./client > out
	$(MAKE) unload
	scripts/verify.py
	make gnuplot
