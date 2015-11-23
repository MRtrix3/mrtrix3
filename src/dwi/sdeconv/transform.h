/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 2014

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __dwi_sdeconv_transform_h__
#define __dwi_sdeconv_transform_h__

#include "math/SH.h"
#include "dwi/sdeconv/response.h"


namespace MR {
  namespace DWI {

    namespace {
        inline std::vector<int> get_lmax (const std::vector<size_t> & ncoefs) 
        {
          std::vector<int> lmax;
          for (size_t n = 0; n < ncoefs.size(); ++n)
            lmax.push_back (Math::SH::LforN (ncoefs[n]));
          return lmax;
        }
    }

    template <typename ValueType>
      inline Math::Matrix<ValueType> get_SH_to_DWI_mapping (
          const Image::Header& header, 
          Math::Matrix<ValueType>& grad,
          Math::Matrix<ValueType>& directions,
          std::vector<size_t>& dwis, 
          std::vector<size_t>& bzeros, 
          const std::vector< Response<ValueType> >& response,
          std::vector<size_t> & ncoefs,
          bool single_shell = false,
          bool lmax_from_command_line = true, 
          int lmax = -1,
          int default_lmax = 8, 
          ValueType bvalue_threshold = NAN)
      {
        if (response.size() < 1) 
          throw Exception ("must specify at least one tissue type to generate SH to DWI mapping");

        grad = get_valid_DW_scheme<ValueType> (header);
        normalise_grad (grad);

        App::Options opt = App::get_options ("shell");
        if (opt.size()) 
          single_shell = true;

        if (single_shell) {
          DWI::Shells shells (grad);
          shells.select_shells (true, true);
          if (shells.smallest().is_bzero())
            bzeros = shells.smallest().get_volumes();
          dwis = shells.largest().get_volumes();
        }

        if (lmax_from_command_line) {
          opt = App::get_options ("lmax");
          if (opt.size()) 
            lmax = to<int> (opt[0][0]);
        }

        if (lmax < 0) {
          lmax = Math::SH::LforN ((dwis.size() ? dwis.size() : grad.rows() ) - response.size() + 1);
          if (lmax > default_lmax)
            lmax = default_lmax;
        }


        directions = gen_direction_matrix (grad, dwis);

        size_t ncol = 0;
        for (size_t n = 0; n < response.size(); ++n) {
          ncoefs.push_back (Math::SH::NforL (std::min (response[n].lmax(), lmax)));
          ncol += ncoefs.back();
        }
        INFO ("computing SH transform using lmax = " + str (get_lmax (ncoefs)));

        VLA_MAX (AL, ValueType, lmax+1, 64);
        Math::Legendre::Plm_sph (AL, lmax, 0, ValueType (1.0));
        Math::Matrix<ValueType> SH (directions.rows(), ncol);
        size_t start_col = 0;
        for (size_t n = 0; n < response.size(); ++n) {
          int actual_lmax = std::min (response[n].lmax(), lmax);
          size_t end_col = start_col + ncoefs[n];
          typename Math::Matrix<ValueType>::View view (SH.sub(0, SH.rows(), start_col, end_col));
          Math::SH::init_transform (view, directions, actual_lmax);

          // apply response function:
          const size_t N = dwis.size() ? dwis.size() : grad.rows();
          for (size_t g = 0; g < N; ++g) {
            size_t row_num = dwis.size() ? dwis[g] : g;
            response[n].set_bval (grad (row_num, 3));
            size_t start = 0;
            typename Math::Vector<ValueType>::View row (view.row (g));
            for (int l = 0; l <= actual_lmax; l += 2) {
              size_t end = start + 2*l + 1;
              row.sub (start, end) *= response[n].value (l) / AL[l];
              start = end;
            }
          }

          start_col = end_col;
        }

        return SH;
      }



  }
}


#endif

