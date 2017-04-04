/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */




#ifndef __dwi_directions_set_h__
#define __dwi_directions_set_h__



#include <cstdint>
#include <vector>

#include "progressbar.h"
#include "dwi/directions/predefined.h"

namespace MR {
  namespace DWI {
    namespace Directions {



      typedef unsigned int dir_t;



      class Set {

        public:

          explicit Set (const std::string& path) :
              dir_mask_bytes (0),
              dir_mask_excess_bits (0),
              dir_mask_excess_bits_mask (0)
          {
            auto matrix = load_matrix (path);

            if (matrix.cols() != 2 && matrix.cols() != 3)
              throw Exception ("Text file \"" + path + "\"does not contain directions as either azimuth-elevation pairs or XYZ triplets");

            initialise (matrix);
          }

          explicit Set (const size_t d) :
              dir_mask_bytes (0),
              dir_mask_excess_bits (0),
              dir_mask_excess_bits_mask (0)
          {
            Eigen::MatrixXd az_el_pairs;
            load_predefined (az_el_pairs, d);
            initialise (az_el_pairs);
          }

          Set (const Set& that) = default;

          Set (Set&& that) :
              unit_vectors (std::move (that.unit_vectors)),
              adj_dirs (std::move (that.adj_dirs)),
              dir_mask_bytes (that.dir_mask_bytes),
              dir_mask_excess_bits (that.dir_mask_excess_bits),
              dir_mask_excess_bits_mask (that.dir_mask_excess_bits_mask)
          {
            that.dir_mask_bytes = that.dir_mask_excess_bits = that.dir_mask_excess_bits_mask = 0;
          }

          // TODO Want to generalise this template, but it was causing weird compilation issues
          template <class MatrixType>
          explicit Set (const Eigen::Matrix<MatrixType, Eigen::Dynamic, Eigen::Dynamic>& m) :
            dir_mask_bytes (0),
            dir_mask_excess_bits (0),
            dir_mask_excess_bits_mask (0)
          {
            initialise (m);
          }

          size_t size () const { return unit_vectors.size(); }
          const Eigen::Vector3f& get_dir (const size_t i) const { return unit_vectors[i]; }
          const std::vector<dir_t>& get_adj_dirs (const size_t i) const { return adj_dirs[i]; }

          bool dirs_are_adjacent (const dir_t one, const dir_t two) const {
            for (const auto& i : adj_dirs[one]) {
              if (i == two)
                return true;
            }
            return false;
          }

          dir_t get_min_linkage (const dir_t one, const dir_t two) const;

          const std::vector<Eigen::Vector3f>& get_dirs() const { return unit_vectors; }
          const Eigen::Vector3f& operator[] (const size_t i) const { return unit_vectors[i]; }


        protected:

          // TODO Change to double
          std::vector<Eigen::Vector3f> unit_vectors;
          std::vector< std::vector<dir_t> > adj_dirs; // Note: not self-inclusive


        private:

          size_t dir_mask_bytes, dir_mask_excess_bits;
          uint8_t dir_mask_excess_bits_mask;
          friend class Mask;

          Set ();

          void load_predefined (Eigen::MatrixXd& az_el_pairs, const size_t);
          template <class MatrixType>
          void initialise (const Eigen::Matrix<MatrixType, Eigen::Dynamic, Eigen::Dynamic>&);
          void initialise_adjacency();
          void initialise_mask();

      };



      template <class MatrixType>
      void Set::initialise (const Eigen::Matrix<MatrixType, Eigen::Dynamic, Eigen::Dynamic>& in)
      {
        unit_vectors.resize (in.rows());
        if (in.cols() == 2) {
          for (size_t i = 0; i != size(); ++i) {
            const float azimuth   = in(i, 0);
            const float elevation = in(i, 1);
            const float sin_elevation = std::sin (elevation);
            unit_vectors[i] = { std::cos (azimuth) * sin_elevation, std::sin (azimuth) * sin_elevation, std::cos (elevation) };
          }
        } else if (in.cols() == 3) {
          for (size_t i = 0; i != size(); ++i)
            unit_vectors[i] = { float(in(i,0)), float(in(i,1)), float(in(i,2)) };
        } else {
          assert (0);
        }
        initialise_adjacency();
        initialise_mask();
      }






      class FastLookupSet : public Set {

        public:

          FastLookupSet (const std::string& path) : 
              Set (path)
          {
            initialise();
          }

          FastLookupSet (const size_t d) :
              Set (d)
          {
            initialise();
          }

          FastLookupSet (FastLookupSet&& that) :
              Set (std::move (that)),
              grid_lookup (std::move (that.grid_lookup)),
              num_az_grids (that.num_az_grids),
              num_el_grids (that.num_el_grids),
              total_num_angle_grids (that.total_num_angle_grids),
              az_grid_step (that.az_grid_step),
              el_grid_step (that.el_grid_step),
              az_begin (that.az_begin),
              el_begin (that.el_begin) { }

          dir_t select_direction (const Eigen::Vector3f&) const;



        private:

          std::vector< std::vector<dir_t> > grid_lookup;
          unsigned int num_az_grids, num_el_grids, total_num_angle_grids;
          float az_grid_step, el_grid_step;
          float az_begin, el_begin;

          FastLookupSet ();

          dir_t select_direction_slow (const Eigen::Vector3f&) const;

          void initialise();

          size_t dir2gridindex (const Eigen::Vector3f&) const;

          void test_lookup() const;

      };



    }
  }
}

#endif

