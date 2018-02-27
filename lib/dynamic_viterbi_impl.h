/* -*- c++ -*- */
/*
 * Copyright 2017-2018 Free Software Foundation, Inc.
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

#ifndef INCLUDED_LAZYVITERBI_DYNAMIC_VITERBI_IMPL_H
#define INCLUDED_LAZYVITERBI_DYNAMIC_VITERBI_IMPL_H

#include <numeric>
#include <lazyviterbi/dynamic_viterbi.h>
#include "lazy_viterbi_impl.h"
#include "viterbi_impl.h"

namespace gr {
  namespace lazyviterbi {

    class dynamic_viterbi_impl : public dynamic_viterbi
    {
     private:
      lazy_viterbi_impl d_lazy_block;
      viterbi_impl d_viterbi_block;
      bool d_is_lazy;
      float d_thres;

      gr::trellis::fsm d_FSM;
      int d_K;
      int d_S0;
      int d_SK;

     public:
      dynamic_viterbi_impl(const gr::trellis::fsm &FSM, int K, int S0, int SK, float thres);

      gr::trellis::fsm FSM() const  { return d_FSM; }
      int K()  const { return d_K; }
      int S0()  const { return d_S0; }
      int SK()  const { return d_SK; }
      float thres()  const { return d_thres; }
      bool is_lazy()  const { return d_is_lazy; }

      void set_FSM(const gr::trellis::fsm &FSM);
      void set_K(int K);
      void set_S0(int S0);
      void set_SK(int SK);
      void set_thres(float thres);

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

      void choose_algo(const float *metrics, int K, int O);
    };

  } // namespace lazyviterbi
} // namespace gr

#endif /* INCLUDED_LAZYVITERBI_DYNAMIC_VITERBI_IMPL_H */

