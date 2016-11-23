.PHONY: all
all: main.cpp band.cpp band.h utils.cpp utils.h
	g++ -g -O2 -Wall -Wextra main.cpp band.cpp utils.cpp wfm_demod.cpp \
		wfm_demod.h \
		-o main \
		-lgnuradio-pmt \
		-lgnuradio-osmosdr -lboost_system \
		-lgnuradio-filter -lgnuradio-audio -lgnuradio-analog \
		-lgnuradio-runtime

clean:
	rm -f main
