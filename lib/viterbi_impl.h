/* -*- c++ -*- */
/*
 * Copyright 2018 Free Software Foundation, Inc.
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

#ifndef INCLUDED_LAZYVITERBI_VITERBI_IMPL_H
#define INCLUDED_LAZYVITERBI_VITERBI_IMPL_H

#include <lazyviterbi/viterbi.h>

namespace gr {
  namespace lazyviterbi {

    class viterbi_impl : public viterbi
    {
      private:
        gr::trellis::fsm d_FSM; //Trellis description
        int d_K;                //Number of trellis sections
        int d_S0;               //Initial state idx (-1 if unknown)
        int d_SK;               //Final state idx (-1 if unknown)

        //Same as d_FSM.OS(), but re-ordered in the following way:
        //d_ordered_OS[s*I+i] = d_FSM.OS()[d_FSM.PS()[s][i]*I + d_FSM.PI()[s][i]]
        std::vector<int> d_ordered_OS;

        //Store current state metrics
        std::vector<float> d_alpha_prev;
        //Store next state metrics
        std::vector<float> d_alpha_curr;
        //Traceback vector
        std::vector<int> d_trace;

      public:
        viterbi_impl(const gr::trellis::fsm &FSM, int K, int S0, int SK);

        gr::trellis::fsm FSM() const  { return d_FSM; }
        int K()  const { return d_K; }
        int S0()  const { return d_S0; }
        int SK()  const { return d_SK; }

        void set_S0(int S0);
        void set_SK(int SK);

        void forecast (int noutput_items, gr_vector_int &ninput_items_required);

        int general_work(int noutput_items, gr_vector_int &ninput_items,
            gr_vector_const_void_star &input_items, gr_vector_void_star &output_items);

        void viterbi_algorithm(int I, int S, int O, const std::vector<int> &NS,
            const std::vector<int> &ordered_OS, const std::vector< std::vector<int> > &PS,
            const std::vector< std::vector<int> > &PI, int K, int S0, int SK,
            const float *in, unsigned char *out);
    };

  } // namespace lazyviterbi
} // namespace gr

#endif /* INCLUDED_LAZYVITERBI_VITERBI_IMPL_H */

