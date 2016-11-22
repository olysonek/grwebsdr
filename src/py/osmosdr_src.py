import osmosdr

class osmosdr_src:
    def __init__(self, id):
        self._id = id
        self._output = osmosdr.source(args=id)
        self._output.set_sample_rate(2e6)
        self._output.set_center_freq(926e5)
        self._output.set_freq_corr(0)
        self._output.set_dc_offset_mode(0)
        self._output.set_iq_balance_mode(0)
        self._output.set_gain_mode(False)
        self._output.set_gain(10)
        self._output.set_if_gain(20)
        self._output.set_bb_gain(20)
        self._output.set_bandwidth(0)
