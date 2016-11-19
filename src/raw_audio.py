from gnuradio import filter
from gnuradio import analog

class raw_audio:
    def __init__(self, source, top_block, demod):
        self._source = source
        self._top_block = top_block
        self._demod = demod

        if demod == "FM":
            self._demodulator = analog.wfm_rcv(quad_rate=source.get_output_rate(),
                    audio_decimation=1)
        else:
            self._demodulator = None

        self._output = filter.rational_resampler_fff(interpolation=48,
                decimation=250, taps=None, fractional_bw=None)

        top_block.connect((source._output, 0), (self._demodulator, 0))
        top_block.connect((self._demodulator, 0), (self._output, 0))

    def get_output_rate(self):
        return 48000
