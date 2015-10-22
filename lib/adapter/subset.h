/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/10/09.

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

#ifndef __adapter_subset_h__
#define __adapter_subset_h__

#include "image.h"
#include "adapter/base.h"

namespace MR
{
  namespace Adapter {

    template <class ImageType>
      class Subset : public Base<ImageType>
    {
      public:
        typedef typename ImageType::value_type value_type;

        using Base<ImageType>::name;
        using Base<ImageType>::spacing;

        template <class VectorType>
        Subset (const ImageType& original, const VectorType& from, const VectorType& dimensions) :
            Base<ImageType> (original),
            from_ (container_cast<decltype(from_)>(from)),
            size_ (container_cast<decltype(size_)>(dimensions)),
            header_ (original.header(), from, dimensions) { }

        void reset () {
          for (size_t n = 0; n < parent().ndim(); ++n)
            set_pos (n, 0);
        }

        const MR::Header& header() const { return header_; }

        ssize_t size (const size_t axis) const { return size_[axis]; }
        ssize_t index (size_t axis) const { return parent().index(axis)-from_[axis]; }
        auto index (size_t axis) -> decltype(Helper::index(*this, axis)) { return { *this, axis }; } 
        void move_index (size_t axis, ssize_t increment) { parent().index(axis) += increment; }

      protected:
        using Base<ImageType>::parent;
        const std::vector<ssize_t> from_, size_;

        class Header : public MR::Header
        {
          public:
            template <class VectorType>
            Header (const MR::Header& original, const VectorType& from, const VectorType& dimensions) :
                MR::Header (original)
            {
              for (size_t n = 0; n < MR::Header::ndim(); ++n) {
                if (ssize_t(from[n] + dimensions[n]) > MR::Header::size(n))
                  throw Exception ("FIXME: dimensions requested for Subset adapter are out of bounds!");
                MR::Header::size(n) = dimensions[n];
              }
              for (size_t j = 0; j < 3; ++j) {
                for (size_t i = 0; i < 3; ++i)
                  MR::Header::transform_(i,3) += from[j] * MR::Header::spacing(j) * MR::Header::transform_(i,j);
              }
            }
        } header_;
    };

  }
}

#endif


