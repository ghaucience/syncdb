ROOTDIR=$(shell pwd)
WORKDIR=$(ROOTDIR)/build

#VERSION	:= .0.0.1

targets  += libdbsync.so$(VERSION)
#targets  += libdbsc.so$(VERSION)
#targets  += libdbss.so$(VERSION)
targets	 += dbsync_cli
targets	 += dbsync_svr

.PHONY: targets

all : $(targets)

srcs				+= $(ROOTDIR)/sync_tcp.cpp
srcs				+= $(ROOTDIR)/lib/src/base64/base64.c
srcs				+= $(ROOTDIR)/lib/src/json/json_parser.c
srcs	      := $(subst .cpp,.c,$(srcs))


dbscsrcs				+= $(ROOTDIR)/sync_cli.cpp
dbscsrcs				+= $(srcs)
dbscsrcs	      := $(subst .cpp,.c,$(dbscsrcs))

dbsssrcs				+= $(ROOTDIR)/sync_svr.cpp
dbsssrcs				+= $(srcs)
dbsssrcs	      := $(subst .cpp,.c,$(dbsssrcs))

dbsyncsrcs			+= $(ROOTDIR)/sync_cli.cpp
dbsyncsrcs			+= $(ROOTDIR)/sync_svr.cpp
dbsyncsrcs			+= $(srcs)
dbsyncsrcs	    := $(subst .cpp,.c,$(dbsyncsrcs))



clisrcs				:= $(ROOTDIR)/cli.cpp
#clisrcs				+= $(ROOTDIR)/sync_tcp.cpp
#clisrcs				+= $(ROOTDIR)/sync_cli.cpp
#clisrcs				+= $(ROOTDIR)/lib/src/base64/base64.c
#clisrcs				+= $(ROOTDIR)/lib/src/json/json_parser.c
clisrcs       := $(subst .cpp,.c,$(clisrcs))

svrsrcs				:= $(ROOTDIR)/svr.cpp
#svrsrcs				+= $(ROOTDIR)/sync_tcp.cpp
#svrsrcs				+= $(ROOTDIR)/sync_svr.cpp
#svrsrcs				+= $(ROOTDIR)/lib/src/base64/base64.c
#svrsrcs				+= $(ROOTDIR)/lib/src/json/json_parser.c
svrsrcs       := $(subst .cpp,.c,$(svrsrcs))



cliobjs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(clisrcs)))
svrobjs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(svrsrcs)))
objs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(srcs)))
#dbscobjs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(dbscsrcs)))
#dbssobjs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(dbsssrcs)))
dbsyncobjs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(dbsyncsrcs)))




-include $(ROOTDIR)/make/arch.mk
-include $(ROOTDIR)/make/rules.mk

#$(eval $(call LinkLio,libdbsc.so$(VERSION),$(dbscobjs)))
#$(eval $(call LinkLio,libdbss.so$(VERSION),$(dbssobjs)))
$(eval $(call LinkLio,libdbsync.so$(VERSION),$(dbsyncobjs)))
$(eval $(call LinkApp,dbsync_cli,$(cliobjs)))
$(eval $(call LinkApp,dbsync_svr,$(svrobjs)))


scp:
	scp ./build/alink root@192.168.0.230:/root


rcli : 
	#cp ./build/libdbsc.so$(VERSION) ./lib/lib/ -rf
	#cp ./build/libdbss.so$(VERSION) ./lib/lib/ -rf
	cp ./build/libdbsync.so$(VERSION) ./lib/lib/ -rf
	export LD_LIBRARY_PATH=./lib/lib;./build/dbsync_cli

rsvr : 
	#cp ./build/libdbsc.so$(VERSION) ./lib/lib/ -rf
	#cp ./build/libdbss.so$(VERSION) ./lib/lib/ -rf
	#cp ./build/libdbsync.so$(VERSION) ./lib/lib/ -rf
	export LD_LIBRARY_PATH=./lib/lib;./build/dbsync_svr

install: 
	sudo mkdir -p /usr/local/bin/sac
	sudo cp $(ROOTDIR)/build/dbsync_cli /usr/loca/bin/sac
	sudo cp $(ROOTDIR)/client.sh /usr/local/bin/sac/

	sudo mkdir -p /usr/local/lib/sac
	sudo cp $(ROOTDIR)/lib/lib/* /usr/local/lib/sac/ -rf
	sudo cp $(ROOTDIR)/build/libdbsync.so$(VERSION) /usr/local/lib/sac/ 

start:
	/usr/local/bin/sac/client.sh start &

stop:
	/usr/local/bin/sac/client.sh stop &
	
