.PHONY: all
all: main.cpp receiver.cpp receiver.h
	g++ -g -O0 -Wall -Wextra main.cpp receiver.cpp \
		-o main \
		-lgnuradio-osmosdr -lboost_system  -lgnuradio-pmt \
		-lgnuradio-filter -lgnuradio-audio -lgnuradio-analog \
		-lgnuradio-runtime -lgnuradio-blocks -lmicrohttpd

clean:
	rm -f main
