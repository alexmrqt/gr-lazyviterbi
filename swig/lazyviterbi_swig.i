/* -*- c++ -*- */

#define LAZYVITERBI_API

%include "gnuradio.i"           // the common stuff

//load generated python docstrings
%include "lazyviterbi_swig_doc.i"

%{
#include "lazyviterbi/viterbi.h"
#include "lazyviterbi/lazy_viterbi.h"
#include "lazyviterbi/dynamic_viterbi.h"
#include "lazyviterbi/viterbi_volk_branch.h"
#include "lazyviterbi/viterbi_volk_state.h"
%}

%include "lazyviterbi/viterbi.h"
GR_SWIG_BLOCK_MAGIC2(lazyviterbi, viterbi);
%include "lazyviterbi/lazy_viterbi.h"
GR_SWIG_BLOCK_MAGIC2(lazyviterbi, lazy_viterbi);
%include "lazyviterbi/dynamic_viterbi.h"
GR_SWIG_BLOCK_MAGIC2(lazyviterbi, dynamic_viterbi);

%include "lazyviterbi/viterbi_volk_branch.h"
GR_SWIG_BLOCK_MAGIC2(lazyviterbi, viterbi_volk_branch);
%include "lazyviterbi/viterbi_volk_state.h"
GR_SWIG_BLOCK_MAGIC2(lazyviterbi, viterbi_volk_state);
