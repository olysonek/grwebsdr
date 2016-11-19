from gnuradio import gr
from osmosdr_src import osmosdr_src
from band import band
from raw_audio import raw_audio
from sink import sink

tb = gr.top_block()
src = osmosdr_src('numchan=1')
b = band(src, tb, 0, 75000)
ra = raw_audio(b, tb, 'FM')
a = sink(ra, tb)

tb.start()
try:
    raw_input('')
except EOFError:
    pass
tb.stop()
tb.wait()
