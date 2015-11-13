.PHONY: all clean
#APP:=pjsua_stream
#APP:=test-codecs
#APP:=app-streamer
#APP:=app-receiver
APP:=list-devices
#APP:=confbridge
#APP:=adjust-volume
STREAMER:=app-streamer
RECEIVER:=app-receiver

SRC_DIR:=.
STREAMER_SRCS:=$(STREAMER).c
RECEIVER_SRCS:=$(RECEIVER).c
APP_SRCS:=$(APP).c
SRCS:=endpoint.c

C_DIR:=../common
C_SRCS:=my-pjlib-utils.c my-pjmedia-utils.c

CFLAGS:=$(shell pkg-config --cflags libpjproject) -Werror
CFLAGS+=-I$(C_DIR)/include
CFLAGS+=-I$(ICS_DIR)/include -I$(Q_DIR)/include -I$(O_DIR)/include
CFLAGS+=-Iinclude
LIBS:=$(shell pkg-config --libs libpjproject)

all: $(STREAMER) $(STREAMER2) $(RECEIVER) $(APP)

$(STREAMER): $(STREAMER_SRCS:.c=.o) $(C_SRCS:.c=.o) $(SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
$(RECEIVER): $(RECEIVER_SRCS:.c=.o) $(C_SRCS:.c=.o) $(SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
$(APP): $(APP_SRCS:.c=.o) $(C_SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
$(APP_SRCS:.c=.o): %.o: test/%.c
	gcc -c -o $@ $< $(CFLAGS)
$(STREAMER_SRCS:.c=.o): %.o: $(SRC_DIR)/test/%.c
	gcc -c -o $@ $< $(CFLAGS)
$(RECEIVER_SRCS:.c=.o): %.o: $(SRC_DIR)/test/%.c
	gcc -c -o $@ $< $(CFLAGS)
$(SRCS:.c=.o): %.o: src/%.c
	gcc -c -o $@ $< $(CFLAGS)
$(PLAY_SRCS:.c=.o): %.o: $(SRC_DIR)/test/%.c
	gcc -c -o $@ $< $(CFLAGS)

$(C_SRCS:.c=.o): %.o: $(C_DIR)/src/%.c
	gcc -c -o $@ $< $(CFLAGS)

clean:
	rm -fr *.o $(STREAMER) $(RECEIVER) $(APP)

