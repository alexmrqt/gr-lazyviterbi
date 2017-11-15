title: LAZY-VITERBI Algorithm
brief: A quicker viterbi algorithm.
tags: # Tags are arbitrary, but look at CGRAN what other authors are using
  - sdr, Viterbi
author:
  - Alexandre Marquet.
copyright_owner:
  - Free Software Foundation, Inc.
license:
#repo:
#website:
#icon:
---
This module contains a single block which is a drop-in replacement for the Viterbi
block of gr-trellis.
It implements the Viterbi algorithm for decoding or equalization in the way described
in Feldman, Jon & Abou-Faycal, Ibrahim & Frigo, Matteo. (2002).
A Fast Maximum-Likelihood Decoder for Convolutional Codes. Vehicular Technology Conference, 1988, IEEE 38th.
10.1109/VETECF.2002.1040367. 

This implementation provides a significant speedup at moderate SNRs, but is slower
at low SNR.
