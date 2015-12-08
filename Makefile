include custom.mk

.PHONY: all clean
#APP:=pjsua_stream
#APP:=test-codecs
APP:=list-devices
#APP:=confbridge
#APP:=adjust-volume
STREAMER:=app-streamer
RECEIVER:=app-receiver
STREAMER2:=app-streamer-2

SRC_DIR:=.
STREAMER_SRCS:=$(STREAMER).c
STREAMER_SRCS2:=$(STREAMER2).c
RECEIVER_SRCS:=$(RECEIVER).c
APP_SRCS:=$(APP).c
SRCS:=endpoint.c

C_DIR:=../common
C_SRCS:=my-pjlib-utils.c my-pjmedia-utils.c

CFLAGS:=$(shell pkg-config --cflags libpjproject) -Werror
#CFLAGS:=-DPJ_AUTOCONF=1	-O2 -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1 -I../libs/linux-armv7l/include
CFLAGS+=-I$(C_DIR)/include
CFLAGS+=-I$(ICS_DIR)/include -I$(Q_DIR)/include -I$(O_DIR)/include
CFLAGS+=-Iinclude
LIBS:=$(shell pkg-config --libs libpjproject)
#LIBS:=-L../libs/linux-armv7l/lib -lpjsua2-arm-none-linux-gnueabi -lstdc++ -lpjsua-arm-none-linux-gnueabi -lpjsip-ua-arm-none-linux-gnueabi -lpjsip-simple-arm-none-linux-gnueabi -lpjsip-arm-none-linux-gnueabi -lpjmedia-codec-arm-none-linux-gnueabi -lpjmedia-arm-none-linux-gnueabi -lpjmedia-videodev-arm-none-linux-gnueabi -lpjmedia-audiodev-arm-none-linux-gnueabi -lpjmedia-arm-none-linux-gnueabi -lpjnath-arm-none-linux-gnueabi -lpjlib-util-arm-none-linux-gnueabi -lsrtp-arm-none-linux-gnueabi -lresample-arm-none-linux-gnueabi -lgsmcodec-arm-none-linux-gnueabi -lspeex-arm-none-linux-gnueabi -lilbccodec-arm-none-linux-gnueabi -lg7221codec-arm-none-linux-gnueabi -lportaudio-arm-none-linux-gnueabi -lpj-arm-none-linux-gnueabi -lm -lrt -lpthread -lasound


all: $(STREAMER) $(STREAMER2) $(RECEIVER) $(APP)

$(STREAMER): $(STREAMER_SRCS:.c=.o) $(C_SRCS:.c=.o) $(SRCS:.c=.o)
	$(CC) -o $@ $^ $(LIBS)
$(STREAMER2): $(STREAMER_SRCS2:.c=.o) $(C_SRCS:.c=.o) $(SRCS:.c=.o)
	$(CC) -o $@ $^ $(LIBS)
$(RECEIVER): $(RECEIVER_SRCS:.c=.o) $(C_SRCS:.c=.o) $(SRCS:.c=.o)
	$(CC) -o $@ $^ $(LIBS)
$(APP): $(APP_SRCS:.c=.o) $(C_SRCS:.c=.o)
	$(CC) -o $@ $^ $(LIBS)
$(APP_SRCS:.c=.o): %.o: test/%.c
	$(CC) -c -o $@ $< $(CFLAGS)
$(STREAMER_SRCS:.c=.o): %.o: $(SRC_DIR)/test/%.c
	$(CC) -c -o $@ $< $(CFLAGS)
$(STREAMER_SRCS2:.c=.o): %.o: $(SRC_DIR)/test/%.c
	$(CC) -c -o $@ $< $(CFLAGS)
$(RECEIVER_SRCS:.c=.o): %.o: $(SRC_DIR)/test/%.c
	$(CC) -c -o $@ $< $(CFLAGS)
$(SRCS:.c=.o): %.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS)
$(C_SRCS:.c=.o): %.o: $(C_DIR)/src/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -fr *.o $(STREAMER) $(STREAMER2) $(RECEIVER) $(APP)

