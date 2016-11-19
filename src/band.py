from gnuradio import filter
from gnuradio.filter import firdes

class band:
    def __init__(self, source, top_block, center_freq, cutoff):
        self._source = source
        self._top_block = top_block
        self._center_freq = center_freq
        self._cutoff = cutoff
        self._decimation = 8
        taps = firdes.low_pass(1.0, source._output.get_sample_rate(), cutoff,
                cutoff / 3, firdes.WIN_HAMMING, 6.76)
        self._output = filter.freq_xlating_fir_filter_ccc(self._decimation,
                taps, center_freq, source._output.get_sample_rate())
        top_block.connect((source._output, 0), (self._output, 0))

    def get_output_rate(self):
        return self._source._output.get_sample_rate() / self._decimation
