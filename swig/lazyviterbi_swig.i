/* -*- c++ -*- */

#define LAZYVITERBI_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "lazyviterbi_swig_doc.i"

%{
#include "lazyviterbi/lazy_viterbi.h"
%}


%include "lazyviterbi/lazy_viterbi.h"
GR_SWIG_BLOCK_MAGIC2(lazyviterbi, lazy_viterbi);
