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
        d_FSM(FSM), d_K(K), d_ordered_OS(FSM.S()*FSM.I())
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
      d_max_size_PS_s = 0;

      for(int s=0 ; s < S ; ++s) {
        for(size_t i=0 ; i<(PS[s]).size() ; ++i) {
          *(ordered_OS_it++) = OS[PS[s][i]*I + PI[s][i]];
        }

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
              d_FSM.NS(), d_ordered_OS, d_FSM.PS(), d_FSM.PI(), d_K, d_S0, d_SK,
              &(in[n*d_K*d_FSM.O()]), &(out[n*d_K]));
        }
      }

      consume_each(d_FSM.O() * noutput_items);
      return noutput_items;
    }

    //Volk optimized implementation adapted when the number of branch between
    //pairs of states is inferior to the number of states.
    void
    viterbi_volk_state_impl::viterbi_algorithm_volk_state(int I, int S, int O,
        const std::vector<int> &NS, const std::vector<int> &ordered_OS,
        const std::vector< std::vector<int> > &PS,
        const std::vector< std::vector<int> > &PI, int K, int S0, int SK,
        const float *in, unsigned char *out)
    {
    }


  } /* namespace lazyviterbi */
} /* namespace gr */

