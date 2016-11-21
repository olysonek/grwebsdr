from gnuradio import audio

class sink:
    def __init__(self, source, top_block):
        self._source = source
        self._top_block = top_block
        self._sink = audio.sink(source.get_output_rate(), '', True)
        top_block.connect((source._output, 0), (self._sink, 0))
