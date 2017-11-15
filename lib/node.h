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

#ifndef INCLUDED_LAZYVITERBI_NODE_H
#define INCLUDED_LAZYVITERBI_NODE_H

#include <lazyviterbi/api.h>
#include <cfloat>

namespace gr {
namespace lazyviterbi {
	/*!
	 * \struct node "Structure for real nodes."
	 *
	 * Contains the previous state and previous
	 * input sequence (identifying the branch) leading to itself.
	 * The identifying information of the node itself in the trellis must be
	 * handled by its container.
	 */
    struct node
    {
      /*!
       * Index of the previous state on the shortest path.
       */
      unsigned int prev_state_idx;
      /*!
       * Previous input leading to this node.
       */
      int prev_input;

      /*!
       * Wether or note this nodes has been expanded.
       */
      bool expanded;
    };

	/*!
	 * \struct node "Structure for shadow nodes."
	 *
	 * Contains its identifying information (time and state indexes), as well as
	 * the previous state and previous input sequence (identifying the branch)
	 * leading to itself.
	 */
    struct shadow_node
    {
      /*!
       * Index of the previous state on the shortest path.
       */
      unsigned int prev_state_idx;
      /*!
       * Previous input leading to this node.
       */
      int prev_input;

      /*!
       * Time index of the node.
       */
      unsigned int time_idx;
      /*!
       * State identifier of the node.
       */
      unsigned int state_idx;
    };
  } // namespace lazyviterbi
} // namespace gr

#endif /* INCLUDED_LAZYVITERBI_NODE_H */

