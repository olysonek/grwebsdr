.PHONY: all
all: main.cpp
	g++ -g -O0 -Wall -Wextra main.cpp -o main -lgnuradio-osmosdr -lboost_system \
		-lgnuradio-filter -lgnuradio-audio -lgnuradio-analog \
		-lgnuradio-runtime -lgnuradio-blocks -lmicrohttpd

clean:
	rm -f main
