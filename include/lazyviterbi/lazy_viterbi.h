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


#ifndef INCLUDED_LAZYVITERBI_LAZY_VITERBI_H
#define INCLUDED_LAZYVITERBI_LAZY_VITERBI_H

#include <lazyviterbi/api.h>
#include <gnuradio/block.h>
#include <gnuradio/trellis/fsm.h>

namespace gr {
  namespace lazyviterbi {

    /*!
     * \brief A maximum likelihood decoder.
     *
     * This block implements the Viterbi algorithm for decoding or equalization
     * in the way described in \cite Feldman2002.
     *
     * It takes euclidean metrics as an input and produces decoded sequences.
     *
     * The idea is to see the Viterbi algorithm as an instance of a shortest path
     * algorithm for trellis. While the original Viterbi algorithm goes through
     * every path of the trellis to calculate the shortest path \cite Forney1973,
     * the Lazy Viterbi algorithm re-uses concepts from Dijkstra's algorithm to
     * speed-up shortest path calculation in the moderate and high SNR regions.
     *
     * This implementation provides a significant speedup at moderate SNRs,
     * but is slower at low SNR.
     * Finally, it shows slightly worse performance than gr-trellis's Viterbi
     * algorithm as it converts the metrics from float to 8-bit integers.
     */
    class LAZYVITERBI_API lazy_viterbi : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<lazy_viterbi> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of lazyviterbi::lazy_viterbi.
       *
       * To avoid accidental use of raw pointers, lazyviterbi::lazy_viterbi's
       * constructor is in a private implementation
       * class. lazyviterbi::lazy_viterbi::make is the public interface for
       * creating new instances.
       *
       * \param FSM Trellis of the code.
       * \param K Length of a block of data.
       * \param S0 Initial state of the encoder (set to -1 if unknown).
       * \param SK Final state of the encoder (set to -1 if unknown).
       */
      static sptr make(const gr::trellis::fsm &FSM, int K, int S0, int SK);

      /*!
       * \return The trellis used by the decoder.
       */
      virtual gr::trellis::fsm FSM() const  = 0;
      /*!
       * \return The data blocks length considered by the decoder.
       */
      virtual int K()  const = 0;
      /*!
       * \return The initial state of the encoder (as given to the decoder, -1
       * if unspecified).
       */
      virtual int S0()  const = 0;
      /*!
       * \return The final state of the encoder (as given to the decoder, -1 if
       * unspecified).
       */
      virtual int SK()  const = 0;

      /*!
       * Gives the initial state of the encoder to the decoder (set to -1 if unknown).
       */
      virtual void set_S0(int S0) = 0;
      /*!
       * Gives the final state of the encoder to the decoder (set to -1 if unknown).
       */
      virtual void set_SK(int SK) = 0;

      /*!
       * \brief Process the input metrics.
       *
       * In order to avoid the algorithm for exploring more paths than necessary
       * due to sporadic noise impulse (which affects equally every branches at
       * a given time index), each branch metrics at each time index are normalized
       * so that the lowest metric is 0.
       * This writes :
       * \f{equation}{
       *  \text{metrics}[k.O + j] = \text{in}[k.O + j] -
       *  \min_{i\in [0:O-1]} \{ \text{metrics}[k.O + i] \}.
       * \f}
       * whith \f$ k \f$ the time index, \f$ O \f$ the number of output symbols and \f$ i \in [0:O-1] \f$.
       *
       * \param *in Input branch metrics.
       * \param *metrics Processed branch metrics.
       * \param K Length of a block of data.
       * \param O number of possible output symbols of the FSM under consideration.
       */
      virtual void lazy_viteri_metrics_norm(const float *in, uint8_t* metrics,
          int K, int O) = 0;

      /*!
       * \brief Actual Lazy Viterbi algorithm implementation
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
       * \param out Output decoded sequence.
       *
       */
      virtual void lazy_viterbi_algorithm(int I, int S, int O,
          const std::vector<int> &NS, const std::vector<int> &OS, int K, int S0,
          int SK, const float *in, unsigned char *out) = 0;

    };

  } // namespace lazyviterbi
} // namespace gr

#endif /* INCLUDED_LAZYVITERBI_LAZY_VITERBI_H */

