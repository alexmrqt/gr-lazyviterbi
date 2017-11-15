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

import os
import numpy
import time
import matplotlib.pyplot as plt


class transmitter_channel_metrics(gr.top_block):
    def __init__(self, pkt_len, nb_pkt, EbN0dB, fsm):
        gr.top_block.__init__(self, "Transmitter, channel and metrics computation");

        ##################################################
        # Variables
        ##################################################
        self.pkt_len = pkt_len;
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
        self.to_symbols = digital.chunks_to_symbols_bc((const.points()), 1);

        self.noise_src = analog.noise_source_c(analog.GR_GAUSSIAN, noisevar, 0);
        self.noise_adder = blocks.add_vcc(1);

        self.metrics_computer = trellis.metrics_c(4, 2, ([-1, -1, -1, 1, 1, -1, 1, 1]), digital.TRELLIS_EUCLIDEAN);

        self.dst = blocks.vector_sink_f();

        ##################################################
        # Connections
        ##################################################
        self.connect((self.bits_src, 0), (self.trellis_encoder, 0));
        self.connect((self.trellis_encoder, 0), (self.unpack, 0));
        self.connect((self.unpack, 0), (self.to_symbols, 0));

        self.connect((self.to_symbols, 0), (self.noise_adder, 0));
        self.connect((self.noise_src, 0), (self.noise_adder, 1));
        self.connect((self.noise_adder, 0), (self.metrics_computer, 0));

        self.connect((self.metrics_computer, 0), (self.dst, 0));

class ber_vs_ebn0_awgn(gr.top_block):

    def __init__(self, metrics, decoder):
        gr.top_block.__init__(self, "BER vs Eb/N0 (7,5) AWGN");

        ##################################################
        # Variables
        ##################################################
        self.metrics_source = blocks.vector_source_f(metrics, False);
        self.sink = blocks.null_sink(gr.sizeof_char);

        ##################################################
        # Connections
        ##################################################
        self.connect((self.metrics_source, 0), (decoder, 0));
        self.connect((decoder, 0), (self.sink, 0));

def main():
    pkt_len = 8192;
    #pkt_len = 32768;
    nb_pkt=100;
    EbN0dB = numpy.linspace(0, 10, 11);
    bitrate_viterbi = numpy.zeros(len(EbN0dB));
    bitrate_lazy = numpy.zeros(len(EbN0dB));

    prefix = os.getcwd();
    #fsm = fsm = trellis.fsm(prefix + "/57.fsm");
    fsm = fsm = trellis.fsm(prefix + "/awgn1o2_128.fsm");
    vit_dec = trellis.viterbi_b(trellis.fsm(fsm), pkt_len, 0, -1);
    lazy_dec = lv.lazy_viterbi(trellis.fsm(fsm), pkt_len, 0, -1);

    for i in range(0, len(EbN0dB)):
        print "Eb/N0=",
        print EbN0dB[i];

        print "Simulating transmission"
        tb = transmitter_channel_metrics(pkt_len, nb_pkt, EbN0dB[i], fsm);
        tb.run();

        metrics=tb.dst.data();

        #Viterbi
        tb = ber_vs_ebn0_awgn(metrics, vit_dec);

        start_time=time.time();
        tb.run();
        elapsed_time=time.time() - start_time;

        bitrate_viterbi[i] = pkt_len*nb_pkt/elapsed_time;
        print "Bitrate Viterbi:\t",
        print bitrate_viterbi[i]

        #Lazy Viterbi
        tb = ber_vs_ebn0_awgn(metrics, lazy_dec);

        start_time=time.time();
        tb.run();
        elapsed_time=time.time() - start_time;

        bitrate_lazy[i] = pkt_len*nb_pkt/elapsed_time;
        print "Bitrate Lazy:\t\t",
        print bitrate_lazy[i]

        print ""

    #Plot results
    viterbi_plot = plt.plot(EbN0dB, bitrate_viterbi, '-+', label="Viterbi");
    lazy_plot = plt.plot(EbN0dB, bitrate_lazy, '-x', label="Lazy");

    plt.grid(which='both');
    plt.ylabel('Bitrate (bps)');
    plt.xlabel('Eb/N0 (dB)');
    plt.legend();
    plt.show();

if __name__ == '__main__':
    main();

