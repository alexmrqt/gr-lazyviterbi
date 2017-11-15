# GNURadio implementation of the Lazy Viterbi algorithm.

This module contains a single block which is a drop-in replacement for the Viterbi
block of gr-trellis.
It implements the Viterbi algorithm for decoding or equalization in the way described
in Feldman, Jon & Abou-Faycal, Ibrahim & Frigo, Matteo. (2002).
A Fast Maximum-Likelihood Decoder for Convolutional Codes. Vehicular Technology Conference, 1988, IEEE 38th.
10.1109/VETECF.2002.1040367. 

This implementation provides a significant speedup at moderate SNRs, but is slower
at low SNR.

One GRC example is provided in the examples/ directory.
There are also two python scripts :
* `ber_vs_ebn0_75_awgn.py` lets you compare the BER of this algorithm
and the gr-trellis's implementation of the classical Viterbi algorithm for a
transmission AWGN channel with the (7,5) convolutional code (there
should be a slight difference of performance du to metrics quantization from our
implementation) ;
* `bitrate_vs_ebn0.py` compare the speed of this algorithm and the gr-trellis's
implementation of the classical Viterbi algorithm for a transmission AWGN
channel with the (229,159) convolutional code.
The convolutional code used in this script can easily be changed by supplying its
treillis in a FSM file (see gr-trellis documentation), to see what kind of speedup
can be expected for a particular use case.
