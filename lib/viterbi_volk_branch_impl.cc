/* -*- c++ -*- */
/*
 * Copyright 2019 Alexandre Marquet.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "viterbi_volk_branch_impl.h"

namespace gr {
  namespace lazyviterbi {

    viterbi_volk_branch::sptr
    viterbi_volk_branch::make(const gr::trellis::fsm &FSM, int K, int S0, int SK)
    {
      return gnuradio::get_initial_sptr
        (new viterbi_volk_branch_impl(FSM, K, S0, SK));
    }

    /*
     * The private constructor
     */
    viterbi_volk_branch_impl::viterbi_volk_branch_impl(const gr::trellis::fsm &FSM, int K, int S0, int SK)
      : gr::block("viterbi_volk_branch",
              gr::io_signature::make(1, -1, sizeof(float)),
              gr::io_signature::make(1, -1, sizeof(char))),
        d_FSM(FSM), d_K(K), d_ordered_OS(FSM.S()*FSM.I()),
        d_ordered_PS(FSM.S()*FSM.I())
    {
      //S0 and SK must represent a state of the trellis
      if(S0 >= 0 || S0 < d_FSM.S()) {
        d_S0 = S0;
      }
      else {
        d_S0 = -1;
      }

      if(SK >= K || SK < d_FSM.S()) {
        d_SK = SK;
      }
      else {
        d_SK = -1;
      }

      int I = d_FSM.I();
      int S = d_FSM.S();
      std::vector< std::vector<int> > PS = d_FSM.PS();
      std::vector< std::vector<int> > PI = d_FSM.PI();
      std::vector<int> OS = d_FSM.OS();

      //Compute ordered_OS and max_size_PS_s
      std::vector<int>::iterator ordered_OS_it = d_ordered_OS.begin();
      std::vector<int>::iterator ordered_PS_it = d_ordered_PS.begin();

      d_n_metrics = 0;
      for(int s=0 ; s < S ; ++s) {
        for(size_t i=0 ; i<(PS[s]).size() ; ++i) {
          *(ordered_OS_it++) = OS[PS[s][i]*I + PI[s][i]];
          *(ordered_PS_it++) = PS[s][i];
          ++d_n_metrics;
        }
      }

      //Memory reservations
      d_alpha_curr = (float*)volk_malloc(S*sizeof(float), volk_get_alignment());

      d_alpha_prev = (float*)volk_malloc(S*sizeof(float), volk_get_alignment());

      d_can_metrics = (float*)volk_malloc(d_n_metrics*sizeof(float),
          volk_get_alignment());

      d_ordered_in_k = (float*)volk_malloc(d_n_metrics * sizeof(float),
          volk_get_alignment());

      d_trace = (uint32_t*)volk_malloc(K*S*sizeof(uint32_t),
          volk_get_alignment());

      set_relative_rate(1.0 / ((double)d_FSM.O()));
      set_output_multiple(d_K);
    }

    viterbi_volk_branch_impl::~viterbi_volk_branch_impl()
    {
      volk_free(d_alpha_prev);
      volk_free(d_alpha_curr);
      volk_free(d_can_metrics);
      volk_free(d_ordered_in_k);
      volk_free(d_trace);
    }

    void
    viterbi_volk_branch_impl::set_S0(int S0)
    {
      gr::thread::scoped_lock guard(d_setlock);
      d_S0 = S0;
    }

    void
    viterbi_volk_branch_impl::set_SK(int SK)
    {
      gr::thread::scoped_lock guard(d_setlock);
      d_SK = SK;
    }

    void
    viterbi_volk_branch_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
      int input_required =  d_FSM.O() * noutput_items;
      unsigned ninputs = ninput_items_required.size();
      for(unsigned int i = 0; i < ninputs; i++) {
        ninput_items_required[i] = input_required;
      }
    }

    int
    viterbi_volk_branch_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      gr::thread::scoped_lock guard(d_setlock);
      int nstreams = input_items.size();
      int nblocks = noutput_items / d_K;

      for(int m = 0; m < nstreams; m++) {
        const float *in = (const float*)input_items[m];
        unsigned char *out = (unsigned char*)output_items[m];

        for(int n = 0; n < nblocks; n++) {
          viterbi_algorithm_volk_branch(d_FSM.I(), d_FSM.S(), d_FSM.O(),
              d_FSM.NS(), d_ordered_OS, d_FSM.PS(), d_FSM.PI(), d_K, d_S0, d_SK,
              &(in[n*d_K*d_FSM.O()]), &(out[n*d_K]));
        }
      }

      consume_each(d_FSM.O() * noutput_items);
      return noutput_items;
    }

    void
    viterbi_volk_branch_impl::compute_all_metrics(const float *alpha_prev,
        const float *in_k, float *can_metrics)
    {
      std::vector<int>::const_iterator ordered_OS_it = d_ordered_OS.begin();
      std::vector<int>::const_iterator ordered_PS_it = d_ordered_PS.begin();

      float *ordered_in_k_it = d_ordered_in_k;
      float *can_metrics_it = can_metrics;

      for(size_t i=0 ; i < d_n_metrics ; ++i) {
        *(can_metrics_it++) = alpha_prev[*(ordered_PS_it++)];
        *(ordered_in_k_it++) = in_k[*(ordered_OS_it++)];
      }

      volk_32f_x2_subtract_32f(can_metrics, can_metrics, d_ordered_in_k, d_n_metrics);
    }

    //Volk optimized implementation adapted when the number of branch between
    //pairs of states is greater than the number of states.
    void
    viterbi_volk_branch_impl::viterbi_algorithm_volk_branch(int I, int S, int O,
        const std::vector<int> &NS, const std::vector<int> &ordered_OS,
        const std::vector< std::vector<int> > &PS,
        const std::vector< std::vector<int> > &PI, int K, int S0, int SK,
        const float *in, unsigned char *out)
    {
      int tb_state, pidx;
      size_t n_branch_state = 0;

      uint32_t *max_idx = (uint32_t*)volk_malloc(sizeof(uint32_t),
          volk_get_alignment());

      std::vector< std::vector<int> >::const_iterator PS_s;

      float *alpha_curr_it;
      float *can_metrics_it = d_can_metrics;
      uint32_t *trace_it = d_trace;

      //If initial state was specified
      if(S0 != -1) {
        std::fill(d_alpha_prev, d_alpha_prev + S,
            -std::numeric_limits<float>::max());
        d_alpha_prev[S0] = 0.0;
      }
      else {
        std::fill(d_alpha_prev, d_alpha_prev + S, 0.0);
      }

      for(float* in_k=(float*)in ; in_k < (float*)in + K*O ; in_k += O) {
        //ADD
        compute_all_metrics(d_alpha_prev, in_k, d_can_metrics);

        alpha_curr_it = d_alpha_curr;
        PS_s = PS.begin();
        //COMPARE
        for(int s = 0 ; s < S ; ++s) {
          n_branch_state = (*PS_s++).size();

          volk_32f_index_max_32u(trace_it, can_metrics_it, n_branch_state);

          //SELECT
          *(alpha_curr_it++) = can_metrics_it[*(trace_it++)];

          //Update pointer
          can_metrics_it += n_branch_state;
        }

        //At this point, current path metrics becomes previous path metrics
        std::swap(d_alpha_prev, d_alpha_curr);

        //Metrics normalization
        volk_32f_index_max_32u(max_idx, d_alpha_prev, S);
        std::transform(d_alpha_prev, d_alpha_prev + S, d_alpha_prev,
            std::bind2nd(std::minus<float>(), d_alpha_prev[*max_idx]));

        //Update iterators
        can_metrics_it = d_can_metrics;
      }

      //If final state was specified
      if(SK != -1) {
        tb_state = SK;
      }
      else{
        //at this point, d_alpha_prev contains the path metrics of states after time K
        tb_state = (int)(*max_idx);
      }

      //Traceback
      trace_it -= S; //place trace at the last time index

      for(unsigned char* out_k = out+K-1 ; out_k >= out ; --out_k) {
        //Retrieve previous input index from d_trace
        pidx=*(trace_it + tb_state);
        //Update d_trace_it for next output symbol
        trace_it -= S;

        //Output previous input
        *out_k = (unsigned char) PI[tb_state][pidx];

        //Update tb_state with the previous state on the shortest path
        tb_state = PS[tb_state][pidx];
      }

      //Dealocate max_idx
      volk_free(max_idx);
    }

  } /* namespace lazyviterbi */
} /* namespace gr */

