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
        d_FSM(FSM), d_K(K)
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

      int S = d_FSM.S();
      std::vector< std::vector<int> > PS = d_FSM.PS();
      d_max_size_PS_s = 0;

      for(int s=0 ; s < S ; ++s) {
        if ((PS[s]).size() > d_max_size_PS_s) {
          d_max_size_PS_s = (PS[s]).size();
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
    viterbi_volk_state_impl::order_alpha_prev_in(int i, const float *alpha_prev,
        const float *in_k, float *alpha_prev_ord, float *in_k_ord)
    {
          for(int s=0 ; s < d_FSM.S() ; ++s) {
            if (i >= d_FSM.PS()[s].size()) {
                alpha_prev_ord[s] = std::numeric_limits<float>::max();
            }
            else{
                alpha_prev_ord[s] = alpha_prev[d_FSM.PS()[s][i]];
			    in_k_ord[s] = in_k[d_FSM.OS()[d_FSM.PS()[s][i]*d_FSM.I() + d_FSM.PI()[s][i]]];
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

      std::vector<int> trace(K*S, 0);

      //Variables to be allocated by volk (for best alignment)
      float *can_metrics = (float*)volk_malloc(S*sizeof(float),
              volk_get_alignment());
      float *alpha_curr = (float*)volk_malloc(S*sizeof(float),
              volk_get_alignment());
      float *alpha_prev = (float*)volk_malloc(S*sizeof(float),
              volk_get_alignment());
      float *in_tmp = (float*)volk_malloc(S*sizeof(float),
              volk_get_alignment());

      std::vector<int>::iterator trace_it = trace.begin();

      //If initial state was specified
      if(S0 != -1) {
        for (float* alpha_prev_it = alpha_prev ;
              alpha_prev_it < alpha_prev + S ; ++alpha_prev_it) {
          *alpha_prev_it = std::numeric_limits<float>::max();
        }
        alpha_prev[S0] = 0.0;
      }
      else {
        for (float* alpha_prev_it = alpha_prev ;
              alpha_prev_it < alpha_prev + S ; ++alpha_prev_it) {
          *alpha_prev_it = 0.0;
        }
      }

      for(float* in_k=(float*)in ; in_k < (float*)in + K*O ; in_k += O) {
		order_alpha_prev_in(0, alpha_prev, in_k, alpha_curr, in_tmp);
        volk_32f_x2_add_32f(alpha_curr, alpha_curr, in_tmp, S);

        min_metric = *std::min_element(alpha_curr, alpha_curr + S);

        //Loop
        for(size_t i=1 ; i < d_max_size_PS_s ; ++i) {
          //ACS for state s
		  order_alpha_prev_in(i, alpha_prev, in_k, can_metrics, in_tmp);
          volk_32f_x2_add_32f(can_metrics, can_metrics, in_tmp, S);

          for(int s=0 ; s < S ; ++s) {
            //COMPARE
            if(can_metrics[s] < alpha_curr[s]) {
              alpha_curr[s] = can_metrics[s];
              //SELECT
              trace_it[s] = i;
            }
          }
          min_metric = *std::min_element(alpha_curr, alpha_curr + S);
        }

        //Metrics normalization
        std::transform(alpha_curr, alpha_curr + S, alpha_curr,
            std::bind2nd(std::minus<double>(), min_metric));

        //At this point, current path metrics becomes previous path metrics
        std::swap(alpha_prev, alpha_curr);

        //Update iterators
        trace_it += S;
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
      trace_it = trace.end() - S; //place trace_it at the last time index

      for(unsigned char* out_k = out+K-1 ; out_k >= out ; --out_k) {
        //Retrieve previous input index from trace
        pidx=*(trace_it + tb_state);
        //Update trace_it for next output symbol
        trace_it -= S;

        //Output previous input
        *out_k = (unsigned char) PI[tb_state][pidx];

        //Update tb_state with the previous state on the shortest path
        tb_state = PS[tb_state][pidx];
      }

      volk_free(alpha_prev);
      volk_free(alpha_curr);
      volk_free(can_metrics);
      volk_free(in_tmp);
    }


  } /* namespace lazyviterbi */
} /* namespace gr */

