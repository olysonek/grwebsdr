AM_LDFLAGS = -lgnuradio-osmosdr -lboost_system  -lgnuradio-pmt \
	-lgnuradio-filter -lgnuradio-audio -lgnuradio-analog \
	-lgnuradio-runtime -lgnuradio-blocks \
	-lvorbisenc -lvorbis -logg -lwebsockets \
	-ljson-c -lsqlite3

bin_PROGRAMS = grwebsdr
grwebsdr_SOURCES = am_demod.cpp auth.cpp config_load.cpp fm_demod.cpp http.cpp \
	main.cpp ogg_sink.cpp receiver.cpp ssb_demod.cpp utils.cpp websocket.cpp
