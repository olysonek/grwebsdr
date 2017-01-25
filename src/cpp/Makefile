.PHONY: all
all: main.cpp receiver.cpp receiver.h ogg_sink.h ogg_sink.cpp stuff.h \
	websocket.cpp utils.cpp utils.h fm_demod.cpp fm_demod.h \
	am_demod.cpp am_demod.h
	g++ -g -O0 -Wall -Wextra main.cpp receiver.cpp ogg_sink.cpp \
		websocket.cpp utils.cpp fm_demod.cpp  am_demod.cpp \
		-o main \
		-lgnuradio-osmosdr -lboost_system  -lgnuradio-pmt \
		-lgnuradio-filter -lgnuradio-audio -lgnuradio-analog \
		-lgnuradio-runtime -lgnuradio-blocks -lmicrohttpd \
		-lvorbisenc -lvorbis -logg -pthread -lwebsockets \
		-ljson-c

clean:
	rm -f main
