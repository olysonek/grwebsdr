.PHONY: all
all: main.cpp receiver.cpp receiver.h ogg_sink.h ogg_sink.cpp
	g++ -g -O0 -Wall -Wextra main.cpp receiver.cpp ogg_sink.cpp \
		-o main \
		-lgnuradio-osmosdr -lboost_system  -lgnuradio-pmt \
		-lgnuradio-filter -lgnuradio-audio -lgnuradio-analog \
		-lgnuradio-runtime -lgnuradio-blocks -lmicrohttpd \
		-lvorbisenc -lvorbis -logg

clean:
	rm -f main
