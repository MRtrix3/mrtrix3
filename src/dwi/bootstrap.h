/*
   Copyright 2010 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 15/10/10.

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

#ifndef __dwi_bootstrap_h__
#define __dwi_bootstrap_h__

#include "adapter/base.h"


namespace MR {
  namespace DWI {


    template <class ImageType, class Functor, size_t NUM_VOX_PER_CHUNK = 256> 
      class Bootstrap : public Adapter::Base<ImageType>
    {
      public:

        class IndexCompare {
          public:
            bool operator() (const Eigen::Vector3i& a, const Eigen::Vector3i& b) const {
              if (a[0] < b[0]) return true;
              if (a[1] < b[1]) return true;
              if (a[2] < b[2]) return true;
              return false;
            }
        };

        typedef typename ImageType::value_type value_type;
        using Adapter::Base<ImageType>::size;
        using Adapter::Base<ImageType>::index;

        Bootstrap (const ImageType& Image, const Functor& functor) :
          Adapter::Base<ImageType> (Image),
          func (functor),
          next_voxel (nullptr),
          last_voxel (nullptr) {
            assert (Adapter::Base<ImageType>::ndim() == 4);
          }

        value_type value () { 
          return get_voxel()[index(3)]; 
        }

        template <class VectorType>
        void get_values (VectorType& values) { 
          if (index(0) < 0 || index(0) >= size(0) ||
              index(1) < 0 || index(1) >= size(1) ||
              index(2) < 0 || index(2) >= size(2)) 
            values.setZero();
          else {
            auto p = get_voxel();
            for (ssize_t n = 0; n < size(3); ++n) 
              values[n] = p[n];
          }
        }

        void clear () 
        {
          voxels.clear(); 
          if (voxel_buffer.empty())
            voxel_buffer.push_back (std::vector<value_type> (NUM_VOX_PER_CHUNK * size(3)));
          next_voxel = &voxel_buffer[0][0];
          last_voxel = next_voxel + NUM_VOX_PER_CHUNK * size(3);
          current_chunk = 0;
        }

      protected:
        Functor func;
        std::map<Eigen::Vector3i,value_type*,IndexCompare> voxels;
        std::vector<std::vector<value_type>> voxel_buffer;
        value_type* next_voxel;
        value_type* last_voxel;
        size_t current_chunk;

        value_type* allocate_voxel () 
        {
          if (next_voxel == last_voxel) {
            ++current_chunk;
            if (current_chunk >= voxel_buffer.size()) 
              voxel_buffer.push_back (std::vector<value_type> (NUM_VOX_PER_CHUNK * size(3)));
            assert (current_chunk < voxel_buffer.size());
            next_voxel = &voxel_buffer.back()[0];
            last_voxel = next_voxel + NUM_VOX_PER_CHUNK * size(3);
          }
          value_type* retval = next_voxel;
          next_voxel += size(3);
          return retval;
        }

        value_type* get_voxel ()
        {
          value_type*& data (voxels.insert (std::make_pair (Eigen::Vector3i (index(0), index(1), index(2)), nullptr)).first->second);
          if (!data) {
            data = allocate_voxel ();
            ssize_t pos = index(3);
            for (auto l = Loop(3)(*this); l; ++l) 
              data[index(3)] = Adapter::Base<ImageType>::value();
            index(3) = pos;
            func (data);
          }
          return data;
        }
    };

  }
}

#endif




