/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __adapter_extract_h__
#define __adapter_extract_h__

#include "adapter/base.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
      class Extract1D :
        public Base<Extract1D<ImageType>,ImageType>
    { MEMALIGN (Extract1D<ImageType>)
      public:

        using base_type = Base<Extract1D<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        using base_type::ndim;
        using base_type::spacing;
        using base_type::parent;


        Extract1D (const ImageType& original, const size_t axis, const vector<uint32_t>& indices) :
          base_type (original),
          extract_axis (axis),
          indices (indices),
          nsize (indices.size()),
          trans (original.transform()) {
            reset();

            if (extract_axis < 3) {
              Eigen::Vector3d a (0.0, 0.0, 0.0);
              a[extract_axis] = indices[0] * spacing (extract_axis);
              trans.translation() = trans * a;
            }

            this->indices.push_back (indices.back());
          }


        void reset () {
          for (size_t n = 0; n < ndim(); ++n)
            parent().index(n) = ( n == extract_axis ? indices[0] : 0 );
          current_pos = 0;
        }

        ssize_t size (size_t axis) const {
          return ( axis == extract_axis ? nsize : base_type::size (axis) );
        }

        const transform_type& transform () const { return trans; }

        ssize_t get_index (size_t axis) const { return ( axis == extract_axis ? current_pos : parent().index(axis) ); }
        void move_index (size_t axis, ssize_t increment) {
          if (axis == extract_axis) {
            ssize_t prev_pos = current_pos < nsize ? indices[current_pos] : 0;
            current_pos += increment;
            if (current_pos < nsize)
              parent().index(axis) += indices[current_pos] - prev_pos;
            else
              parent().index(axis) = 0;
          }
          else
            parent().index(axis) += increment;
        }


        friend std::ostream& operator<< (std::ostream& stream, const Extract1D& V) {
          stream << "Extract1D adapter for image \"" << V.name() << "\", position [ ";
          for (size_t n = 0; n < V.ndim(); ++n)
            stream << V.index(n) << " ";
          stream << "], value = " << V.value();
          return stream;
        }

      private:
        const size_t extract_axis;
        vector<uint32_t> indices;
        const ssize_t nsize;
        transform_type trans;
        ssize_t current_pos;

    };










    template <class ImageType>
      class Extract :
        public Base<Extract<ImageType>,ImageType>
    { MEMALIGN (Extract<ImageType>)
      public:

        using base_type = Base<Extract<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        using base_type::ndim;
        using base_type::spacing;
        using base_type::parent;


        Extract (const ImageType& original, const vector<vector<uint32_t>>& indices) :
          base_type (original),
          current_pos (ndim()),
          indices (indices),
          trans (original.transform()) {
            reset();
            trans.translation() = trans * Eigen::Vector3d (
                indices[0][0] * spacing (0),
                indices[1][0] * spacing (1),
                indices[2][0] * spacing (2)
                );

            for (const auto& i : indices)
              sizes.push_back (i.size());
          }


        ssize_t size (size_t axis) const { return sizes[axis]; }

        const transform_type& transform () const { return trans; }

        void reset () {
          for (size_t n = 0; n < ndim(); ++n) {
            current_pos[n] = 0;
            parent().index(n) = indices[n][0];
          }
        }

        ssize_t get_index (size_t axis) const { return current_pos[axis]; }
        void move_index (size_t axis, ssize_t increment) {
          current_pos[axis] += increment;
          if (current_pos[axis] < 0)
            parent().index (axis) = -1;
          else if (current_pos[axis] >= sizes[axis])
            parent().index (axis) = parent().size (axis);
          else
            parent().index (axis) = indices[axis][current_pos[axis]];
        }

      private:
        vector<ssize_t> current_pos;
        vector<vector<uint32_t> > indices;
        vector<ssize_t> sizes;
        transform_type trans;
    };

  }
}

#endif


