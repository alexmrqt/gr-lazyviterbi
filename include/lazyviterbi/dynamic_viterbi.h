/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
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


#ifndef INCLUDED_LAZYVITERBI_DYNAMIC_VITERBI_H
#define INCLUDED_LAZYVITERBI_DYNAMIC_VITERBI_H

#include <lazyviterbi/api.h>
#include <gnuradio/block.h>
#include <gnuradio/trellis/fsm.h>

namespace gr {
  namespace lazyviterbi {

    /*!
     * \brief Switch between the Viterbi and the Lazy Viterbi algorithm depending
     * on channel conditions.
     *
     * The decision wether to choose between the two is based on the ratio of
     * the mean of maximum branch metrics and minimum branch metrics:
     * \f[
     * Q = \frac{\sum_{k=0}^{K-1} \max_{o\in [0 : O-1]}\{\text{metrics}_k[o]\}}
     * {\sum_{k=0}^{K-1} \min_{o\in [0 : O-1]}\{\text{metrics}_k[o]\}},
     * \f]
     * where:
     *  - \f$ K \f$ is the length of the data block;
     *  - \f$ O \f$ is the number of possible output symbols of the FSM under
     *  consideration;
     *  - \f$ \text{metrics}_k[o] \f$ is the branch metric at time index
     *  \f$ k \f$ for output symbol \f$ o \f$.
     *
     *  Then, if \f$ Q > \text{threshold} \f$, then the Lazy Viterbi is chosen,
     *  otherwise the classical Viterbi algorithm is chosen.
     *
     */
    class LAZYVITERBI_API dynamic_viterbi : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<dynamic_viterbi> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of lazyviterbi::dynamic_viterbi.
       *
       * To avoid accidental use of raw pointers, lazyviterbi::dynamic_viterbi's
       * constructor is in a private implementation
       * class. lazyviterbi::dynamic_viterbi::make is the public interface for
       * creating new instances.
       *
       * \param FSM Trellis of the code.
       * \param K Length of a block of data.
       * \param S0 Initial state of the encoder (set to -1 if unknown).
       * \param SK Final state of the encoder (set to -1 if unknown).
       * \param thres Threshold for choosing the Lazy Viterbi algorithm over the
       * classical Viterbi algorithm.
       */
      static sptr make(const gr::trellis::fsm &FSM, int K, int S0, int SK, float thres=15.0);

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
       * \return The threshold use to choose the algorithm.
       */
      virtual float thres()  const = 0;
      /*!
       * \return True if the Lazy Viterbi algorithm is currently used.
       */
      virtual bool is_lazy()  const = 0;

      /*!
       * Gives the initial state of the encoder to the decoder (set to -1 if unknown).
       */
      virtual void set_S0(int S0) = 0;
      /*!
       * Gives the final state of the encoder to the decoder (set to -1 if unknown).
       */
      virtual void set_SK(int SK) = 0;
      /*!
       * Set the threshold value.
       */
      virtual void set_thres(float thres) = 0;
    };

  } // namespace lazyviterbi
} // namespace gr

#endif /* INCLUDED_LAZYVITERBI_DYNAMIC_VITERBI_H */

