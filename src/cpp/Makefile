.PHONY: all
all: main.cpp receiver.cpp receiver.h ogg_sink.h ogg_sink.cpp stuff.h \
	websocket.cpp websocket.h utils.cpp utils.h fm_demod.cpp fm_demod.h \
	ssb_demod.cpp ssb_demod.h \
	am_demod.cpp am_demod.h \
	http.cpp http.h \
	config.cpp config.h
	g++ -g -O0 -Wall -Wextra main.cpp receiver.cpp ogg_sink.cpp \
		websocket.cpp utils.cpp fm_demod.cpp  am_demod.cpp \
		ssb_demod.cpp \
		http.cpp config.cpp \
		-o main \
		-lgnuradio-osmosdr -lboost_system  -lgnuradio-pmt \
		-lgnuradio-filter -lgnuradio-audio -lgnuradio-analog \
		-lgnuradio-runtime -lgnuradio-blocks \
		-lvorbisenc -lvorbis -logg -lwebsockets \
		-ljson-c

clean:
	rm -f main
