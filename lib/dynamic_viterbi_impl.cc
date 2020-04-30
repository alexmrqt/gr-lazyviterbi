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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "dynamic_viterbi_impl.h"

namespace gr {
  namespace lazyviterbi {

    dynamic_viterbi::sptr
    dynamic_viterbi::make(const gr::trellis::fsm &FSM, int K, int S0, int SK, float thres)
    {
      return gnuradio::get_initial_sptr
        (new dynamic_viterbi_impl(FSM, K, S0, SK, thres));
    }

    /*
     * The private constructor
     */
    dynamic_viterbi_impl::dynamic_viterbi_impl(const gr::trellis::fsm &FSM, int K, int S0, int SK, float thres)
      : gr::block("dynamic_viterbi",
              gr::io_signature::make(1, -1, sizeof(float)),
              gr::io_signature::make(1, -1, sizeof(char))),
        d_FSM(FSM), d_K(K), d_S0(S0), d_SK(SK), d_is_lazy(true), d_thres(thres),
        d_lazy_block(FSM, K, S0, SK), d_viterbi_block(FSM, K, S0, SK)
    {
      set_relative_rate(1.0 / ((double)d_FSM.O()));
      set_output_multiple(d_K);
    }

    void
    dynamic_viterbi_impl::set_S0(int S0)
    {
      gr::thread::scoped_lock guard(d_setlock);

      d_S0 = S0;
      d_lazy_block.set_S0(S0);
      d_viterbi_block.set_S0(S0);
    }

    void
    dynamic_viterbi_impl::set_SK(int SK)
    {
      gr::thread::scoped_lock guard(d_setlock);

      d_SK = SK;
      d_viterbi_block.set_SK(SK);
    }

    void
    dynamic_viterbi_impl::set_thres(float thres)
    {
      gr::thread::scoped_lock guard(d_setlock);

      d_thres = thres;
    }

    void
    dynamic_viterbi_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      int input_required =  d_FSM.O() * noutput_items;
      unsigned ninputs = ninput_items_required.size();
      for(unsigned int i = 0; i < ninputs; i++) {
        ninput_items_required[i] = input_required;
      }
    }

    int
    dynamic_viterbi_impl::general_work (int noutput_items,
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
          choose_algo(&(in[n*d_K*d_FSM.O()]), d_K, d_FSM.O());

          if(d_is_lazy) {
            d_lazy_block.lazy_viterbi_algorithm(d_FSM.I(), d_FSM.S(), d_FSM.O(),
                d_FSM.NS(), d_FSM.OS(), d_K, d_S0, d_SK, &(in[n*d_K*d_FSM.O()]),
                &(out[n*d_K]));
          }
          else {
            d_viterbi_block.viterbi_algorithm(d_FSM.I(), d_FSM.S(), d_FSM.O(),
                d_FSM.NS(), d_FSM.OS(), d_FSM.PS(), d_FSM.PI(), d_K, d_S0, d_SK,
                &(in[n*d_K*d_FSM.O()]), &(out[n*d_K]));
          }
        }
      }

      consume_each (d_FSM.O() * noutput_items);
      return noutput_items;
    }

    void
    dynamic_viterbi_impl::choose_algo(const float *metrics, int K, int O)
    {
      float acc_max=0.0, acc_min=0.0;
      const float* metrics_end = metrics + K*O;

      while(metrics < metrics_end) {
        //Find min_element
        acc_min += *std::min_element(metrics, metrics+O);

        //Find max_element
        acc_max += *std::max_element(metrics, metrics+O);

        //Update pointer on metrics
        metrics += O;
      }

      if(acc_max/acc_min > d_thres) {
        d_is_lazy = true;
      }
      else {
        d_is_lazy = false;
      }
    }

  } /* namespace lazyviterbi */
} /* namespace gr */

