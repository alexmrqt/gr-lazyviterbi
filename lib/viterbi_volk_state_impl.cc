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
#include "viterbi_volk_state_impl.h"

namespace gr {
  namespace lazyviterbi {

    viterbi_volk_state::sptr
    viterbi_volk_state::make(const gr::trellis::fsm &FSM, int K, int S0, int SK)
    {
      return gnuradio::get_initial_sptr
        (new viterbi_volk_state_impl(FSM, K, S0, SK));
    }

    /*
     * The private constructor
     */
    viterbi_volk_state_impl::viterbi_volk_state_impl(const gr::trellis::fsm &FSM, int K, int S0, int SK)
      : gr::block("viterbi_volk_state",
              gr::io_signature::make(1, -1, sizeof(float)),
              gr::io_signature::make(1, -1, sizeof(char))),
        d_FSM(FSM), d_K(K), d_ordered_OS(FSM.S()*FSM.I()), d_ordered_PS(FSM.S()*FSM.I())
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

      d_max_size_PS_s = 0;
      std::vector<int>::iterator ordered_OS_it = d_ordered_OS.begin();
      std::vector<int>::iterator ordered_PS_it = d_ordered_PS.begin();

      for(int s=0 ; s < S ; ++s) {
        if ((PS[s]).size() > d_max_size_PS_s) {
          d_max_size_PS_s = (PS[s]).size();
        }
      }

      for(size_t i=0 ; i<d_max_size_PS_s ; ++i) {
        for(int s=0 ; s < S ; ++s) {
          if (i < PS[s].size()) {
            *(ordered_OS_it++) = OS[PS[s][i]*I + PI[s][i]];
            *(ordered_PS_it++) = PS[s][i];
          }
        }
      }

      set_relative_rate(1.0 / ((double)d_FSM.O()));
      set_output_multiple(d_K);
    }

    void
    viterbi_volk_state_impl::set_FSM(const gr::trellis::fsm &FSM)
    {
      gr::thread::scoped_lock guard(d_setlock);
      d_FSM = FSM;
      set_relative_rate(1.0 / ((double)d_FSM.O()));
    }

    void
    viterbi_volk_state_impl::set_K(int K)
    {
      gr::thread::scoped_lock guard(d_setlock);
      d_K = K;
      set_output_multiple(d_K);
    }

    void
    viterbi_volk_state_impl::set_S0(int S0)
    {
      gr::thread::scoped_lock guard(d_setlock);
      d_S0 = S0;
    }

    void
    viterbi_volk_state_impl::set_SK(int SK)
    {
      gr::thread::scoped_lock guard(d_setlock);
      d_SK = SK;
    }

    void
    viterbi_volk_state_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
    {
      int input_required =  d_FSM.O() * noutput_items;
      unsigned ninputs = ninput_items_required.size();
      for(unsigned int i = 0; i < ninputs; i++) {
        ninput_items_required[i] = input_required;
      }
    }

    int
    viterbi_volk_state_impl::general_work (int noutput_items,
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
          viterbi_algorithm_volk_state(d_FSM.I(), d_FSM.S(), d_FSM.O(),
              d_FSM.NS(), d_FSM.OS(), d_FSM.PS(), d_FSM.PI(), d_K, d_S0, d_SK,
              &(in[n*d_K*d_FSM.O()]), &(out[n*d_K]));
        }
      }

      consume_each(d_FSM.O() * noutput_items);
      return noutput_items;
    }

    void
    viterbi_volk_state_impl::compute_all_metrics(const float *alpha_prev,
        const float *in_k, float *can_metrics)
    {
      std::vector<int>::const_iterator ordered_OS_it = d_ordered_OS.begin();
      std::vector<int>::const_iterator ordered_PS_it = d_ordered_PS.begin();

      for(size_t i=0 ; i < d_max_size_PS_s ; ++i) {
        for(int s=0 ; s < d_FSM.S() ; ++s) {
          if (i < d_FSM.PS()[s].size()) {
            *(can_metrics++) = alpha_prev[*(ordered_PS_it++)] + \
              in_k[*(ordered_OS_it++)];
          }
          else {
            *(can_metrics++) = std::numeric_limits<float>::max();
          }
        }
      }
    }

    //Volk optimized implementation adapted when the number of branch between
    //pairs of states is inferior to the number of states.
    void
    viterbi_volk_state_impl::viterbi_algorithm_volk_state(int I, int S, int O,
        const std::vector<int> &NS, const std::vector<int> &OS,
        const std::vector< std::vector<int> > &PS,
        const std::vector< std::vector<int> > &PI, int K, int S0, int SK,
        const float *in, unsigned char *out)
    {
      int tb_state, pidx;
      float min_metric = std::numeric_limits<float>::max();

      //TODO: reserve once at construction, use iterators here
      float *alpha_curr = (float*)malloc(S*sizeof(float));
      float *alpha_prev = (float*)malloc(S*sizeof(float));
      int *trace = (int*)malloc(K*S*sizeof(int));
      std::fill(trace, trace + K*S, 0);

      //Variables to be allocated by volk (for best alignment)
      float *can_metrics = (float*)volk_malloc(S*d_max_size_PS_s*sizeof(float),
              volk_get_alignment());

      //If initial state was specified
      if(S0 != -1) {
        std::fill(alpha_prev, alpha_prev + S, std::numeric_limits<float>::max());
        alpha_prev[S0] = 0.0;
      }
      else {
        std::fill(alpha_prev, alpha_prev + S, 0.0);
      }

      for(float* in_k=(float*)in ; in_k < (float*)in + K*O ; in_k += O) {
        //Reset minimum metric (used for normalization)
        min_metric = std::numeric_limits<float>::max();

        //ADD
        compute_all_metrics(alpha_prev, in_k, can_metrics);

        //Pre-loop
        for(int s=0 ; s < S ; ++s) {
          *(alpha_curr++) = *(can_metrics++);
        }
        alpha_curr -= S;

        //Loop
        for(size_t i=1 ; i < d_max_size_PS_s ; ++i) {
          //Todo try to volkize using (a-b)>0
          for(int s=0 ; s < S ; ++s) {
            //COMPARE
            if(*can_metrics < *alpha_curr) {
              //SELECT
              *alpha_curr = *can_metrics;
              if(*alpha_curr < min_metric) {
                min_metric = *alpha_curr;
              }
              *trace = i;
            }

            //Update iterators
            ++can_metrics;
            ++alpha_curr;
            ++trace;
          }

          //Reset iterators
          alpha_curr -= S;
          trace -= S;
        }
        //Reset iterators
        can_metrics -= S*d_max_size_PS_s;

        //Metrics normalization
        std::transform(alpha_curr, alpha_curr + S, alpha_curr,
            std::bind2nd(std::minus<double>(), min_metric));

        //At this point, current path metrics becomes previous path metrics
        std::swap(alpha_prev, alpha_curr);

        //Update iterators
        trace += S;
      }

      //If final state was specified
      if(SK != -1) {
        tb_state = SK;
      }
      else{
        //at this point, alpha_prev contains the path metrics of states after time K
        tb_state = (int)(std::min_element(alpha_prev, alpha_prev + S) - alpha_prev);
      }

      //Traceback
      trace -= S; //place trace at the last time index

      for(unsigned char* out_k = out+K-1 ; out_k >= out ; --out_k) {
        //Retrieve previous input index from trace
        pidx=*(trace + tb_state);
        //Update trace for next output symbol
        trace -= S;

        //Output previous input
        *out_k = (unsigned char) PI[tb_state][pidx];

        //Update tb_state with the previous state on the shortest path
        tb_state = PS[tb_state][pidx];
      }

      trace += S; //place trace at the first time index

      free(alpha_prev);
      free(alpha_curr);
      volk_free(can_metrics);
    }


  } /* namespace lazyviterbi */
} /* namespace gr */

