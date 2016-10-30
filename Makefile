.PHONY: all
all: main.cpp
	g++ -g -O2 -Wall -Wextra main.cpp -o main -lgnuradio-osmosdr -lboost_system \
		-lgnuradio-filter -lgnuradio-audio -lgnuradio-analog \
		-lgnuradio-runtime

clean:
	rm -f main
