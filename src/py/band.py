from gnuradio import filter
from gnuradio.filter import firdes

class band:
    def __init__(self, source, top_block, center_freq, cutoff):
        self._source = source
        self._top_block = top_block
        self._center_freq = center_freq
        self._cutoff = cutoff
        self._decimation = 8
        self._taps = firdes.low_pass(1.0, source._output.get_sample_rate(), cutoff,
                cutoff / 3, firdes.WIN_HAMMING, 6.76)
        self.setup_filter(center_freq)
        top_block.connect((source._output, 0), (self._output, 0))

    def setup_filter(self, freq):
        self._center_freq = freq
        self._output = filter.freq_xlating_fir_filter_ccc(self._decimation,
                self._taps, freq, self._source._output.get_sample_rate())

    def get_output_rate(self):
        return self._source._output.get_sample_rate() / self._decimation

    def set_center_freq(self, freq):
        print('Changing center frequency to ' + str(freq) + ' Hz')
        self._output.set_center_freq(freq)
