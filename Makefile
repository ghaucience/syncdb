ROOTDIR=$(shell pwd)
WORKDIR=$(ROOTDIR)/build

targets	 += dbsync_cli

.PHONY: targets

all : $(targets)


srcs				:= $(ROOTDIR)/main.cpp
srcs				+= $(ROOTDIR)/sync_tcp.cpp
srcs				+= $(ROOTDIR)/sync_cli.cpp
srcs				+= $(ROOTDIR)/lib/src/base64/base64.c
srcs				+= $(ROOTDIR)/lib/src/json/json_parser.c

srcs        := $(subst .cpp,.c,$(srcs))

objs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(srcs)))


-include $(ROOTDIR)/make/arch.mk
-include $(ROOTDIR)/make/rules.mk

$(eval $(call LinkApp,dbsync_cli,$(objs)))


scp:
	scp ./build/alink root@192.168.0.230:/root


run : 
	export LD_LIBRARY_PATH=./lib/lib;./build/dbsync_cli
