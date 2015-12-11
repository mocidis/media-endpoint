include custom.mk

.PHONY: all clean
#APP:=pjsua_stream
#APP:=test-codecs
APP:=list-devices
#APP:=confbridge
#APP:=confsample
#APP:=adjust-volume
#APP:=pjsua-receiver
#STREAMER:=app-streamer
#RECEIVER:=app-receiver
#STREAMER2:=app-streamer-2

STREAMER:=test-streamer
RECEIVER:=test-receiver

SRC_DIR:=.
STREAMER_SRCS:=$(STREAMER).c
STREAMER_SRCS2:=$(STREAMER2).c
RECEIVER_SRCS:=$(RECEIVER).c
APP_SRCS:=$(APP).c
SRCS:=endpoint.c

C_DIR:=../common
C_SRCS:=ansi-utils.c my-pjlib-utils.c

USERVER_DIR:=../userver

PROTOCOL_DIR:=../group-man/protocols
GM_P:=gm-proto.u
GMC_P:=gmc-proto.u
ADV_P:=adv-proto.u
GB_P:=gb-proto.u

NODE_DIR:=../group-man
NODE_SRCS:=node.c gb-receiver.c

ICS_DIR:=../ics
ICS_SRCS:=ics.c ics-event.c ics-command.c

Q_DIR:=../concurrent_queue
Q_SRCS:=queue.c

O_DIR:=../object-pool
O_SRCS:=object-pool.c

EP_DIR:=../media-endpoint
EP_SRCS:=endpoint.c

GEN_DIR:=gen
GEN_SRCS:=gm-client.c gmc-server.c adv-server.c gb-server.c

JSONC_DIR:=../json-c/output

HT_DIR:=../hash-table
HT_SRCS:=hash-table.c

CFLAGS:=-std=c99 $(shell pkg-config --cflags libpjproject) -fms-extensions
CFLAGS+=-I$(ICS_DIR)/include -I$(Q_DIR)/include -I$(O_DIR)/include
CFLAGS+=-I$(C_DIR)/include
CFLAGS+=-I../json-c/output/include/json-c
CFLAGS+=-I$(PROTOCOLS_DIR)/include
CFLAGS+=-I$(GEN_DIR)
CFLAGS+=-I$(NODE_DIR)/include
CFLAGS+=-I$(EP_DIR)/include
CFLAGS+=-I$(HT_DIR)/include
CFLAGS+=-D__ICS_INTEL__

LIBS:=$(shell pkg-config --libs libpjproject) ../json-c/output/lib/libjson-c.a -lpthread

#LIBS:=-L../libs/linux-armv7l/lib -lpjsua2-arm-none-linux-gnueabi -lstdc++ -lpjsua-arm-none-linux-gnueabi -lpjsip-ua-arm-none-linux-gnueabi -lpjsip-simple-arm-none-linux-gnueabi -lpjsip-arm-none-linux-gnueabi -lpjmedia-codec-arm-none-linux-gnueabi -lpjmedia-arm-none-linux-gnueabi -lpjmedia-videodev-arm-none-linux-gnueabi -lpjmedia-audiodev-arm-none-linux-gnueabi -lpjmedia-arm-none-linux-gnueabi -lpjnath-arm-none-linux-gnueabi -lpjlib-util-arm-none-linux-gnueabi -lsrtp-arm-none-linux-gnueabi -lresample-arm-none-linux-gnueabi -lgsmcodec-arm-none-linux-gnueabi -lspeex-arm-none-linux-gnueabi -lilbccodec-arm-none-linux-gnueabi -lg7221codec-arm-none-linux-gnueabi -lportaudio-arm-none-linux-gnueabi -lpj-arm-none-linux-gnueabi -lm -lrt -lpthread -lasound


all: gen-gm gen-gmc gen-adv gen-gb $(STREAMER) $(STREAMER2) $(RECEIVER) $(APP)

gen-gm: $(PROTOCOL_DIR)/$(GM_P)
	mkdir -p gen
	awk -v base_dir=$(USERVER_DIR) -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

gen-gmc: $(PROTOCOL_DIR)/$(GMC_P)
	mkdir -p gen
	awk -v base_dir=$(USERVER_DIR) -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

gen-adv: $(PROTOCOL_DIR)/$(ADV_P)
	mkdir -p gen
	awk -v base_dir=$(USERVER_DIR) -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

gen-gb: $(PROTOCOL_DIR)/$(GB_P)
	mkdir -p gen
	awk -v base_dir=$(USERVER_DIR) -f $(USERVER_DIR)/gen-tools/gen.awk $<
	touch $@

$(STREAMER): $(STREAMER_SRCS:.c=.o) $(C_SRCS:.c=.o) $(SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
$(STREAMER2): $(STREAMER_SRCS2:.c=.o) $(C_SRCS:.c=.o) $(SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
$(RECEIVER): $(RECEIVER_SRCS:.c=.o) $(C_SRCS:.c=.o) $(SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
$(APP): $(NODE_SRCS:.c=.o) $(GEN_SRCS:.c=.o) $(C_SRCS:.c=.o) $(APP_SRCS:.c=.o) $(ICS_SRCS:.c=.o) $(O_SRCS:.c=.o) $(Q_SRCS:.c=.o) $(EP_SRCS:.c=.o) $(HT_SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
$(APP_SRCS:.c=.o): %.o: $(SRC_DIR)/test/%.c
	gcc -c -o $@ $< $(CFLAGS)
$(STREAMER_SRCS:.c=.o): %.o: $(SRC_DIR)/test/%.c
	gcc -c -o $@ $< $(CFLAGS)
$(STREAMER_SRCS2:.c=.o): %.o: $(SRC_DIR)/test/%.c
	gcc -c -o $@ $< $(CFLAGS)
$(RECEIVER_SRCS:.c=.o): %.o: $(SRC_DIR)/test/%.c
	gcc -c -o $@ $< $(CFLAGS)
$(SRCS:.c=.o): %.o: src/%.c
	gcc -c -o $@ $< $(CFLAGS)
$(ICS_SRCS:.c=.o): %.o: $(ICS_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(O_SRCS:.c=.o): %.o: $(O_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(Q_SRCS:.c=.o): %.o: $(Q_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(NODE_SRCS:.c=.o): %.o: $(NODE_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(GEN_SRCS:.c=.o): %.o: $(GEN_DIR)/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(C_SRCS:.c=.o): %.o: $(C_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(ANSI_O_SRCS:.c=.o): %.o: $(ANSI_O_DIR)/src/%.c
	gcc -c -o $@ $^ $(CFLAGS)
$(HT_SRCS:.c=.o) : %.o : $(HT_DIR)/src/%.c
	gcc -o $@ -c $< $(CFLAGS)

clean:
	rm -fr *.o $(STREAMER) $(STREAMER2) $(RECEIVER) $(APP)

