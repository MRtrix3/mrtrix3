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

#ifndef __dwi_bootstrap_h__
#define __dwi_bootstrap_h__

#include "adapter/base.h"


namespace MR {
  namespace DWI {


    template <class ImageType, class Functor, size_t NUM_VOX_PER_CHUNK = 256> 
      class Bootstrap : 
        public Adapter::Base<Bootstrap<ImageType,Functor,NUM_VOX_PER_CHUNK>,ImageType>
    { MEMALIGN (Bootstrap<ImageType,Functor,NUM_VOX_PER_CHUNK>)
      public:

        typedef Adapter::Base<Bootstrap<ImageType,Functor,NUM_VOX_PER_CHUNK>,ImageType> base_type;
        typedef typename ImageType::value_type value_type;

        using base_type::ndim;
        using base_type::size;
        using base_type::index;

        class IndexCompare { NOMEMALIGN
          public:
            bool operator() (const Eigen::Vector3i& a, const Eigen::Vector3i& b) const {
              if (a[0] < b[0]) return true;
              if (a[1] < b[1]) return true;
              if (a[2] < b[2]) return true;
              return false;
            }
        };

        Bootstrap (const ImageType& Image, const Functor& functor) :
          base_type (Image),
          func (functor),
          next_voxel (nullptr),
          last_voxel (nullptr) {
            assert (ndim() == 4);
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
            voxel_buffer.push_back (vector<value_type> (NUM_VOX_PER_CHUNK * size(3)));
          next_voxel = &voxel_buffer[0][0];
          last_voxel = next_voxel + NUM_VOX_PER_CHUNK * size(3);
          current_chunk = 0;
        }

      protected:
        Functor func;
        std::map<Eigen::Vector3i,value_type*,IndexCompare> voxels;
        vector<vector<value_type>> voxel_buffer;
        value_type* next_voxel;
        value_type* last_voxel;
        size_t current_chunk;

        value_type* allocate_voxel () 
        {
          if (next_voxel == last_voxel) {
            ++current_chunk;
            if (current_chunk >= voxel_buffer.size()) 
              voxel_buffer.push_back (vector<value_type> (NUM_VOX_PER_CHUNK * size(3)));
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
              data[index(3)] = base_type::value();
            index(3) = pos;
            func (data);
          }
          return data;
        }
    };

  }
}

#endif




