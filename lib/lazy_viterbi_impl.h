/* -*- c++ -*- */
/*
 * Copyright 2017 Free Software Foundation, Inc.
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

#ifndef INCLUDED_LAZYVITERBI_LAZY_VITERBI_IMPL_H
#define INCLUDED_LAZYVITERBI_LAZY_VITERBI_IMPL_H

#include <boost/container/slist.hpp>
#include <boost/container/stable_vector.hpp>

#include <lazyviterbi/lazy_viterbi.h>
#include "node.h"

namespace gr {
  namespace lazyviterbi {

    class lazy_viterbi_impl : public lazy_viterbi
    {
     private:
      gr::trellis::fsm d_FSM;
      int d_K;
      int d_S0;
      int d_SK;
      /*
       * Real nodes, to be addressed by real_nodes[time_index*d_FSM.S() + state_index]
       */
      std::vector<node> real_nodes;
      /*
       * Shadow nodes. First dimension is used to make a circular buffer of 256
       * vectors (corresponding to the 256 possible values of branch metrics).
       * The vector nested stores shadow_nodes whose incoming branch have same
       * metrics.
       */
      std::vector<std::vector<shadow_node> > shadow_nodes;

     public:
      lazy_viterbi_impl(const gr::trellis::fsm &FSM, int K, int S0, int SK);

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

      /*
       * \Brief Process the input metrics.
       *
       * In order to avoid the algorithm for exploring more paths than necessary
       * due to sporadic noise impulse (which affects equally every branches at
       * a given time index), each branch metrics at fo each time index are normalized
       * so that the lowest metric is 0.
       * This writes :
       * \f{equation}{
       *  metrics[k.O + j] = in[k.O + j] - \min_{i\in [0:O-1]} metrics[k.O + j].
       * \f}
       * whith k the time index, O the number of output symbols and i \in [0:O-1].
       */
      void lazy_viteri_metrics_norm(const float *in, uint8_t* metrics, int K, int O);

      /*
       * \Brief Actual lazy viterbi algorithm implementation
       *
       * \param I The number of input sequences (e.g. 2 for binary codes).
       * \param S The number of states in the trellis.
       * \param O The number of output sequences (e.g. 4 for a binary code with a coding efficiency of 1/2).
       * \param NS Gives the next state ns of a branch defined by its initial state s
       * and its input symbol i : NS[s*I+i]=ns.
       * \param OS Gives the output symbol os of a branch defined by its initial state s
       * and its input symbol i : OS[s*I+i]=os.
       * \param K Length of a block of data.
       * \param S0 Initial state of the encoder (set to -1 if unknown).
       * \param SK Final state of the encoder (set to -1 if unknown).
       * \param in Input euclidean metrics for the algorithm.
       * \param in Output decoded sequence.
       * 
       */
      void lazy_viterbi_algorithm(int I, int S, int O, const std::vector<int> &NS,
          const std::vector<int> &OS, int K, int S0, int SK, const float *in,
          char *out);
    };

  } // namespace lazyviterbi
} // namespace gr

#endif /* INCLUDED_LAZYVITERBI_LAZY_VITERBI_IMPL_H */

