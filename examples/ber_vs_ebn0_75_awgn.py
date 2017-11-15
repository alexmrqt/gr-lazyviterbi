#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from gnuradio import analog
from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import qtgui
from gnuradio import trellis
from gnuradio import fec
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from grc_gnuradio import blks2 as grc_blks2
import lazyviterbi_swig as lv

import numpy
import matplotlib.pyplot as plt


class ber_vs_ebn0_awgn(gr.top_block):

    def __init__(self, pkt_len, nb_pkt, EbN0dB):
        gr.top_block.__init__(self, "BER vs Eb/N0 (7,5) AWGN");

        ##################################################
        # Variables
        ##################################################
        self.prefix = prefix = "/localdata/marqueal/Documents/sdr/lazy-viterbi/gr-lazyviterbi/examples/";
        self.noise_amp_dB = noise_amp_dB = -50;
        self.pkt_len = pkt_len;
        #self.fsm = fsm = trellis.fsm(prefix + "awgn1o2_128.fsm");
        self.fsm = fsm = trellis.fsm(prefix + "57.fsm");
        Rc=0.5;
        self.const = const = digital.constellation_bpsk().base();

        var_c=1;
        Nb=1;
        Eb=var_c/(2.0*Nb*Rc);
        N0=Eb * 10**(-EbN0dB/10.0);
        noisevar=2*N0;

        ##################################################
        # Blocks
        ##################################################
        self.bits_src = blocks.vector_source_b(map(int, numpy.random.randint(0, 2, nb_pkt*pkt_len)), False);
        self.trellis_encoder =  trellis.encoder_bb(trellis.fsm(fsm), 0, pkt_len);
        self.unpack = blocks.unpack_k_bits_bb(2);
        self.pack_src = blocks.pack_k_bits_bb(8);
        self.to_symbols = digital.chunks_to_symbols_bc((const.points()), 1);

        self.noise_src = analog.noise_source_c(analog.GR_GAUSSIAN, noisevar, 0);
        self.noise_adder = blocks.add_vcc(1);

        self.metrics_computer = trellis.metrics_c(4, 2, ([-1, -1, -1, 1, 1, -1, 1, 1]), digital.TRELLIS_EUCLIDEAN);

        self.viterbi = trellis.viterbi_b(trellis.fsm(fsm), pkt_len, 0, -1);
        #self.dijkstra = djbk.dijkstra_gr_block(trellis.fsm(fsm), pkt_len, 0, -1);
        self.lazy = lv.lazy_viterbi(trellis.fsm(fsm), pkt_len, 0, -1);

        self.pack_viterbi = blocks.pack_k_bits_bb(8);
        #self.pack_dijkstra = blocks.pack_k_bits_bb(8);
        self.pack_lazy = blocks.pack_k_bits_bb(8);

        self.ber_computer_viterbi = fec.ber_bf(False);
        #self.ber_computer_dijkstra = fec.ber_bf(False);
        self.ber_computer_lazy = fec.ber_bf(False);

        self.vector_sink_viterbi = blocks.vector_sink_f();
        #self.vector_sink_dijkstra = blocks.vector_sink_f();
        self.vector_sink_lazy = blocks.vector_sink_f();

        ##################################################
        # Connections
        ##################################################

        self.connect((self.bits_src, 0), (self.trellis_encoder, 0));
        self.connect((self.trellis_encoder, 0), (self.unpack, 0));
        self.connect((self.unpack, 0), (self.to_symbols, 0));

        self.connect((self.to_symbols, 0), (self.noise_adder, 0));
        self.connect((self.noise_src, 0), (self.noise_adder, 1));
        self.connect((self.noise_adder, 0), (self.metrics_computer, 0));

        self.connect((self.metrics_computer, 0), (self.viterbi, 0));
        #self.connect((self.metrics_computer, 0), (self.dijkstra, 0));
        self.connect((self.metrics_computer, 0), (self.lazy, 0));

        self.connect((self.bits_src, 0), (self.pack_src, 0));
        self.connect((self.viterbi, 0), (self.pack_viterbi, 0));
        #self.connect((self.dijkstra, 0), (self.pack_dijkstra, 0));
        self.connect((self.lazy, 0), (self.pack_lazy, 0));

        self.connect((self.pack_src, 0), (self.ber_computer_viterbi, 0));
        #self.connect((self.pack_src, 0), (self.ber_computer_dijkstra, 0));
        self.connect((self.pack_src, 0), (self.ber_computer_lazy, 0));
        self.connect((self.pack_viterbi, 0), (self.ber_computer_viterbi, 1));
        #self.connect((self.pack_dijkstra, 0), (self.ber_computer_dijkstra, 1));
        self.connect((self.pack_lazy, 0), (self.ber_computer_lazy, 1));

        #self.connect((self.ber_computer_dijkstra, 0), (self.vector_sink_dijkstra, 0));
        self.connect((self.ber_computer_viterbi, 0), (self.vector_sink_viterbi, 0));
        self.connect((self.ber_computer_lazy, 0), (self.vector_sink_lazy, 0));

def main():
    pkt_len = 16384;
    nb_pkt=100;
    #EbN0dB = [0];
    EbN0dB = numpy.linspace(0, 10, 11);
    BER_viterbi = numpy.zeros(len(EbN0dB));
    #BER_dijkstra = numpy.zeros(len(EbN0dB));
    BER_lazy = numpy.zeros(len(EbN0dB));

    for i in range(0, len(EbN0dB)):
        print "Eb/N0=",
        print EbN0dB[i];

        tb = ber_vs_ebn0_awgn(pkt_len, nb_pkt, EbN0dB[i]);
        tb.run();

        print "BER Viterbi:",
        print tb.vector_sink_viterbi.data();
        BER_viterbi[i] = 10**(numpy.mean(tb.vector_sink_viterbi.data()));
        tb.vector_sink_viterbi.reset();

        #print "BER Dijkstra:",
        #print tb.vector_sink_dijkstra.data();
        #BER_dijkstra[i] = 10**(numpy.mean(tb.vector_sink_dijkstra.data()));
        #tb.vector_sink_dijkstra.reset();

        print "BER Lazy:",
        print tb.vector_sink_lazy.data();
        BER_lazy[i] = 10**(numpy.mean(tb.vector_sink_lazy.data()));
        tb.vector_sink_lazy.reset();

        print ""

    #Plot results
    viterbi_plot = plt.semilogy(EbN0dB, BER_viterbi, '-+', label="Viterbi");
    #dijkstra_plot = plt.semilogy(EbN0dB, BER_dijkstra, '-*', label="Dijkstra");
    lazy_plot = plt.semilogy(EbN0dB, BER_lazy, '-x', label="Lazy");

    plt.grid(which='both');
    plt.ylabel('BER');
    plt.xlabel('Eb/N0 (dB)');
    plt.legend();
    plt.show();

if __name__ == '__main__':
    main();
