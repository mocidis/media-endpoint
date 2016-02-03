.PHONY: all clean
APP:=test_pj_stream
all: build

build:
	gcc -o $(APP) $(APP).c -DPJ_AUTOCONF=1 -O2 -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1 -fms-extensions -I/root/workspace/ics-project/libs/linux-i686/include -L/root/workspace/ics-project/libs/linux-i686/lib -lpjsua2-i686-pc-linux-gnu -lstdc++ -lpjsua-i686-pc-linux-gnu -lpjsip-ua-i686-pc-linux-gnu -lpjsip-simple-i686-pc-linux-gnu -lpjsip-i686-pc-linux-gnu -lpjmedia-codec-i686-pc-linux-gnu -lpjmedia-i686-pc-linux-gnu -lpjmedia-videodev-i686-pc-linux-gnu -lpjmedia-audiodev-i686-pc-linux-gnu -lpjmedia-i686-pc-linux-gnu -lpjnath-i686-pc-linux-gnu -lpjlib-util-i686-pc-linux-gnu -lsrtp-i686-pc-linux-gnu -lresample-i686-pc-linux-gnu -lgsmcodec-i686-pc-linux-gnu -lspeex-i686-pc-linux-gnu -lilbccodec-i686-pc-linux-gnu -lg7221codec-i686-pc-linux-gnu -lportaudio-i686-pc-linux-gnu -lpj-i686-pc-linux-gnu -lm -lrt -lpthread -lasound -lpthread -pthread -lm -lsqlite3
clean:
	rm -rf $(APP)
