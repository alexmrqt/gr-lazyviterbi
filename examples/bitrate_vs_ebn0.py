#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from gnuradio import analog
from gnuradio import blocks
from gnuradio import digital
from gnuradio import gr
from gnuradio import trellis
import lazyviterbi as lv

import os
import numpy
import time
import matplotlib.pyplot as plt


class transmitter_channel_metrics(gr.top_block):
    def __init__(self, pkt_len, nb_pkt, EbN0dB, fsm):
        gr.top_block.__init__(self, "Transmitter, channel and metrics computation")

        ##################################################
        # Variables
        ##################################################
        self.pkt_len = pkt_len
        Rc=0.5
        self.const = const = digital.constellation_bpsk().base()

        var_c=1
        Nb=1
        Eb=var_c/(2.0*Nb*Rc)
        N0=Eb * 10**(-EbN0dB/10.0)
        noisevar=2*N0

        ##################################################
        # Blocks
        ##################################################
        self.bits_src = blocks.vector_source_b(numpy.random.randint(0, 2, nb_pkt*pkt_len), False)
        self.trellis_encoder =  trellis.encoder_bb(trellis.fsm(fsm), 0, pkt_len)
        self.unpack = blocks.unpack_k_bits_bb(2)
        self.to_symbols = digital.chunks_to_symbols_bc((const.points()), 1)

        self.noise_src = analog.noise_source_c(analog.GR_GAUSSIAN, noisevar, 0)
        self.noise_adder = blocks.add_vcc(1)

        self.metrics_computer = trellis.metrics_c(4, 2, ([-1, -1, -1, 1, 1, -1, 1, 1]), digital.TRELLIS_EUCLIDEAN)

        self.dst = blocks.vector_sink_f()

        ##################################################
        # Connections
        ##################################################
        self.connect((self.bits_src, 0), (self.trellis_encoder, 0))
        self.connect((self.trellis_encoder, 0), (self.unpack, 0))
        self.connect((self.unpack, 0), (self.to_symbols, 0))

        self.connect((self.to_symbols, 0), (self.noise_adder, 0))
        self.connect((self.noise_src, 0), (self.noise_adder, 1))
        self.connect((self.noise_adder, 0), (self.metrics_computer, 0))

        self.connect((self.metrics_computer, 0), (self.dst, 0))

class ber_vs_ebn0_awgn(gr.top_block):

    def __init__(self, metrics, decoder):
        gr.top_block.__init__(self, "BER vs Eb/N0 (7,5) AWGN")

        ##################################################
        # Variables
        ##################################################
        self.metrics_source = blocks.vector_source_f(metrics, False)
        self.sink = blocks.null_sink(gr.sizeof_char)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.metrics_source, 0), (decoder, 0))
        self.connect((decoder, 0), (self.sink, 0))

def main():
    #pkt_len = 8192
    pkt_len = 32768
    nb_pkt=100
    EbN0dB = numpy.linspace(0, 10, 11)
    bitrate_trellis_viterbi = numpy.zeros(len(EbN0dB))
    bitrate_viterbi = numpy.zeros(len(EbN0dB))
    bitrate_viterbi_vb = numpy.zeros(len(EbN0dB))
    bitrate_viterbi_vs = numpy.zeros(len(EbN0dB))
    bitrate_lazy = numpy.zeros(len(EbN0dB))
    bitrate_dynamic = numpy.zeros(len(EbN0dB))

    prefix = os.getcwd()
    #fsm = fsm = trellis.fsm(prefix + "/fsm/5_7.fsm")
    #fsm = fsm = trellis.fsm(prefix + "/fsm/229_159.fsm")
    #fsm = fsm = trellis.fsm(prefix + "/fsm/rsc_15_13.fsm")
    fsm = fsm = trellis.fsm(prefix + "/fsm/171_133.fsm")

    trellis_vit_dec = trellis.viterbi_b(trellis.fsm(fsm), pkt_len, 0, -1)
    vit_dec = lv.viterbi(trellis.fsm(fsm), pkt_len, 0, -1)
    vit_vb_dec = lv.viterbi_volk_branch(trellis.fsm(fsm), pkt_len, 0, -1)
    vit_vs_dec = lv.viterbi_volk_state(trellis.fsm(fsm), pkt_len, 0, -1)
    dyn_vit_dec = lv.dynamic_viterbi(trellis.fsm(fsm), pkt_len, 0, -1)
    lazy_dec = lv.lazy_viterbi(trellis.fsm(fsm), pkt_len, 0, -1)

    for i in range(0, len(EbN0dB)):
        print("Eb/N0=" + str(EbN0dB[i]))

        print("Simulating transmission")
        tb = transmitter_channel_metrics(pkt_len, nb_pkt, EbN0dB[i], fsm)
        tb.run()

        metrics=tb.dst.data()

        #Viterbi (gr-trellis)
        tb = ber_vs_ebn0_awgn(metrics, trellis_vit_dec)

        start_time=time.time()
        tb.run()
        elapsed_time=time.time() - start_time

        bitrate_trellis_viterbi[i] = pkt_len*nb_pkt/elapsed_time
        print("Bitrate Viterbi (gr-trellis):\t" + str(bitrate_trellis_viterbi[i]))

        #Viterbi (gr-lazyviterbi)
        tb = ber_vs_ebn0_awgn(metrics, vit_dec)

        start_time=time.time()
        tb.run()
        elapsed_time=time.time() - start_time

        bitrate_viterbi[i] = pkt_len*nb_pkt/elapsed_time
        print("Bitrate Viterbi (gr-lazyviterbi):\t" + str(bitrate_viterbi[i]))

        #Viterbi volk branch
        tb = ber_vs_ebn0_awgn(metrics, vit_vb_dec)

        start_time=time.time()
        tb.run()
        elapsed_time=time.time() - start_time

        bitrate_viterbi_vb[i] = pkt_len*nb_pkt/elapsed_time
        print("Bitrate Viterbi volk branch:\t" + str(bitrate_viterbi_vb[i]))

        #Viterbi volk state
        tb = ber_vs_ebn0_awgn(metrics, vit_vs_dec)

        start_time=time.time()
        tb.run()
        elapsed_time=time.time() - start_time

        bitrate_viterbi_vs[i] = pkt_len*nb_pkt/elapsed_time
        print("Bitrate Viterbi volk state:\t" + str(bitrate_viterbi_vs[i]))

        #Lazy Viterbi
        tb = ber_vs_ebn0_awgn(metrics, lazy_dec)

        start_time=time.time()
        tb.run()
        elapsed_time=time.time() - start_time

        bitrate_lazy[i] = pkt_len*nb_pkt/elapsed_time
        print("Bitrate Lazy:\t\t" + str(bitrate_lazy[i]))

        #Dynamic Viterbi
        tb = ber_vs_ebn0_awgn(metrics, dyn_vit_dec)

        start_time=time.time()
        tb.run()
        elapsed_time=time.time() - start_time

        bitrate_dynamic[i] = pkt_len*nb_pkt/elapsed_time
        print("Bitrate Dynamic:\t\t" + str(bitrate_dynamic[i]))

        print("")

    #Plot results
    plt.plot(EbN0dB, bitrate_trellis_viterbi, '-*', label="Viterbi (gr-trellis)")
    plt.plot(EbN0dB, bitrate_viterbi, '-+', label="Viterbi")
    plt.plot(EbN0dB, bitrate_viterbi_vb, '-+', label="Viterbi volk branch")
    plt.plot(EbN0dB, bitrate_viterbi_vs, '-d', label="Viterbi volk state")
    plt.plot(EbN0dB, bitrate_lazy, '-x', label="Lazy Viterbi")
    plt.plot(EbN0dB, bitrate_dynamic, '-o', label="Dynamique")

    plt.grid(which='both')
    plt.ylabel('Debit (bps)')
    plt.xlabel('Eb/N0 (dB)')
    plt.legend()
    plt.show()

if __name__ == '__main__':
    main()

