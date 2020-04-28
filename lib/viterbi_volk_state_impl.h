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

#ifndef INCLUDED_LAZYVITERBI_VITERBI_VOLK_STATE_IMPL_H
#define INCLUDED_LAZYVITERBI_VITERBI_VOLK_STATE_IMPL_H

#include <lazyviterbi/viterbi_volk_state.h>
#include <volk/volk.h>

namespace gr {
  namespace lazyviterbi {

    class viterbi_volk_state_impl : public viterbi_volk_state
    {
     private:
      gr::trellis::fsm d_FSM; //Trellis description
      int d_K;                //Number of trellis sections
      int d_S0;               //Initial state idx (-1 if unknown)
      int d_SK;               //Final state idx (-1 if unknown)

      //Same as d_FSM.OS(), but re-ordered in the following way:
      //d_ordered_OS[i*S+s] = d_FSM.OS()[d_FSM.PS()[s][i]*I + d_FSM.PI()[s][i]]
      std::vector<int> d_ordered_OS;
      //Same as d_FSM.PS(), but flattened:
      //d_ordered_PS[i*S+s] = d_FSM.PS()[s][i]
      std::vector<int> d_ordered_PS;
      //Input metrics, ordered as d_ordered_in_k[i] = in_k[d_ordered_OS[i]]
      float *d_ordered_in_k;
      //Max size of PS[s]
      size_t d_max_size_PS_s;

      //A vector of S zeros
      float *d_zeros;

      //Store current state metrics
      float *d_alpha_curr;
      //Store next state metrics
      float *d_alpha_prev;
      //Store next state candidate metrics
      float *d_can_metrics;
      //Traceback vector
      int *d_trace;

      protected:
        void compute_all_metrics(const float *alpha_prev, const float *in_k,
            float *can_metrics);

     public:
      viterbi_volk_state_impl(const gr::trellis::fsm &FSM, int K, int S0, int SK);
      ~viterbi_volk_state_impl();

      gr::trellis::fsm FSM() const  { return d_FSM; }
      int K()  const { return d_K; }
      int S0()  const { return d_S0; }
      int SK()  const { return d_SK; }

      void set_FSM(const gr::trellis::fsm &FSM);
      void set_K(int K);
      void set_S0(int S0);
      void set_SK(int SK);

      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items, gr_vector_int &ninput_items,
          gr_vector_const_void_star &input_items, gr_vector_void_star &output_items);

      void viterbi_algorithm_volk_state(int I, int S, int O, const std::vector<int> &NS,
          const std::vector<int> &OS, const std::vector< std::vector<int> > &PS,
          const std::vector< std::vector<int> > &PI, int K, int S0, int SK,
          const float *in, unsigned char *out);
    };

  } // namespace lazyviterbi
} // namespace gr

#endif /* INCLUDED_LAZYVITERBI_VITERBI_VOLK_STATE_IMPL_H */

