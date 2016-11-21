#!/usr/bin/python

from gnuradio import gr
from osmosdr_src import osmosdr_src
from band import band
from raw_audio import raw_audio
from sink import sink
from autobahn.twisted.websocket import WebSocketServerProtocol, \
    WebSocketServerFactory
from twisted.internet import reactor

class ws_protocol(WebSocketServerProtocol):
    def onMessage(self, payload, is_binary):
        freq = int(payload)
        b.set_center_freq(freq)

tb = gr.top_block()
src = osmosdr_src('numchan=1')
b = band(src, tb, 0, 75000)
ra = raw_audio(b, tb, 'FM')
a = sink(ra, tb)

tb.start()

factory = WebSocketServerFactory('ws://localhost:8000')
factory.protocol = ws_protocol
reactor.listenTCP(8000, factory)

try:
    reactor.run()
except KeyboardInterrupt:
    pass

tb.stop()
tb.wait()
