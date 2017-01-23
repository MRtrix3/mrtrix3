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

#ifndef __dwi_tractography_mapping_gaussian_voxel_h__
#define __dwi_tractography_mapping_gaussian_voxel_h__


#include "dwi/tractography/mapping/voxel.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {
        namespace Gaussian {




          // Base class to handle case where the factor contributed by the streamline varies along its length
          //   (currently only occurs when the track-wise statistic is Gaussian)
          class VoxelAddon
          { NOMEMALIGN
            public:
              VoxelAddon() : sum_factors (0.0) { }
              VoxelAddon (const float v) : sum_factors (v) { }
              float get_factor() const { return sum_factors; }
            protected:
              void operator+= (const float f) const { sum_factors += f; }
              void operator=  (const float f) { sum_factors = f; }
              void operator=  (const VoxelAddon& that) { sum_factors = that.sum_factors; }
              void normalize (const float l) const { sum_factors /= l; }
            private:
              mutable float sum_factors;
          };




          class Voxel : public Mapping::Voxel, public VoxelAddon
          { MEMALIGN(Voxel)
            typedef Mapping::Voxel Base;
            public:
            Voxel (const int x, const int y, const int z) : Base (x, y, z) , VoxelAddon () { }
            Voxel (const Eigen::Vector3i& that) : Base (that), VoxelAddon() { }
            Voxel (const Eigen::Vector3i& v, const float l) : Base (v, l), VoxelAddon () { }
            Voxel (const Eigen::Vector3i& v, const float l, const float f) : Base (v, l), VoxelAddon (f) { }
            Voxel () : Base (), VoxelAddon () { }

            Voxel& operator= (const Voxel& V) { Base::operator= (V); VoxelAddon::operator= (V); return *this; }
            void operator+= (const float l) const { Base::operator+= (l); };
            bool operator== (const Voxel& V) const { return Base::operator== (V); }
            bool operator<  (const Voxel& V) const { return Base::operator<  (V); }

            void add (const float l, const float f) const { Base::operator+= (l); VoxelAddon::operator+= (f); }
            void normalize() const { VoxelAddon::normalize (get_length()); Base::normalize(); }

          };


          class VoxelDEC : public Mapping::VoxelDEC, public VoxelAddon
          { MEMALIGN(VoxelDEC)
            typedef Mapping::VoxelDEC Base;
            public:
            VoxelDEC () : Base (), VoxelAddon () { }
            VoxelDEC (const Eigen::Vector3i& V) : Base (V), VoxelAddon () { }
            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3f& d) : Base (V, d), VoxelAddon () { }
            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3f& d, const float l) : Base (V, d, l), VoxelAddon () { }
            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3f& d, const float l, const float f) : Base (V, d, l), VoxelAddon (f) { }

            VoxelDEC& operator=  (const VoxelDEC& V)   { Base::operator= (V); VoxelAddon::operator= (V); return (*this); }
            void operator+= (const float) const { assert (0); }
            void operator+= (const Eigen::Vector3f&) const { assert (0); };
            bool operator== (const VoxelDEC& V) const { return Base::operator== (V); }
            bool operator<  (const VoxelDEC& V) const { return Base::operator<  (V); }

            void add (const Eigen::Vector3f&, const float) const { assert (0); };
            void add (const Eigen::Vector3f& i, const float l, const float f) const { Base::add (i, l); VoxelAddon::operator+= (f); }
            void normalize() const { VoxelAddon::normalize (get_length()); Base::normalize(); }

          };


          class Dixel : public Mapping::Dixel, public VoxelAddon
          { MEMALIGN(Dixel)
            typedef Mapping::Dixel Base;
            public:
            Dixel () : Base (), VoxelAddon () { }
            Dixel (const Eigen::Vector3i& V) : Base (V), VoxelAddon () { }
            Dixel (const Eigen::Vector3i& V, const size_t b) : Base (V, b), VoxelAddon () { }
            Dixel (const Eigen::Vector3i& V, const size_t b, const float l) : Base (V, b, l), VoxelAddon () { }
            Dixel (const Eigen::Vector3i& V, const size_t b, const float l, const float f) : Base (V, b, l), VoxelAddon (f) { }

            Dixel& operator=  (const Dixel& V)       { Base::operator= (V); VoxelAddon::operator= (V); return *this; }
            bool   operator== (const Dixel& V) const { return Base::operator== (V); }
            bool   operator<  (const Dixel& V) const { return Base::operator<  (V); }
            void   operator+= (const float)  const { assert (0); };

            void add (const float l, const float f) const { Base::operator+= (l); VoxelAddon::operator+= (f); }
            void normalize() const { VoxelAddon::normalize (get_length()); Base::normalize(); }

          };


          class VoxelTOD : public Mapping::VoxelTOD, public VoxelAddon
          { MEMALIGN(VoxelTOD)
            typedef Mapping::VoxelTOD Base;
            public:
            VoxelTOD () : Base (), VoxelAddon () { }
            VoxelTOD (const Eigen::Vector3i& V) : Base (V), VoxelAddon () { }
            VoxelTOD (const Eigen::Vector3i& V, const Eigen::VectorXf& t) : Base (V, t), VoxelAddon () { }
            VoxelTOD (const Eigen::Vector3i& V, const Eigen::VectorXf& t, const float l) : Base (V, t, l), VoxelAddon () { }
            VoxelTOD (const Eigen::Vector3i& V, const Eigen::VectorXf& t, const float l, const float f) : Base (V, t, l), VoxelAddon (f) { }

            VoxelTOD& operator=  (const VoxelTOD& V)   { Base::operator= (V); VoxelAddon::operator= (V); return (*this); }
            bool      operator== (const VoxelTOD& V) const { return Base::operator== (V); }
            bool      operator<  (const VoxelTOD& V) const { return Base::operator< (V); }
            void      operator+= (const Eigen::VectorXf&) const { assert (0); };

            void add (const Eigen::VectorXf&, const float) const { assert (0); };
            void add (const Eigen::VectorXf& i, const float l, const float f) const { Base::add (i, l); VoxelAddon::operator+= (f); }
            void normalize() const { VoxelAddon::normalize (get_length()); Base::normalize(); }

          };






          // Unlike standard TWI, with Gaussian smoothing the TWI factor is stored per point rather than per streamline
          // However, it's handy from a code perspective to still use the same base class
          /*
             class SetVoxelExtras
             { MEMALIGN(SetVoxelExtras)
             public:
             size_t index;
             float weight;
             };
             */





          class SetVoxel : public std::set<Voxel>, public Mapping::SetVoxelExtras
          { MEMALIGN(SetVoxel)
            public:
              typedef Voxel VoxType;
              inline void insert (const Eigen::Vector3i& v, const float l, const float f)
              {
                const Voxel temp (v, l, f);
                iterator existing = std::set<Voxel>::find (temp);
                if (existing == std::set<Voxel>::end())
                  std::set<Voxel>::insert (temp);
                else
                  (*existing).add (l, f);
              }
          };
          class SetVoxelDEC : public std::set<VoxelDEC>, public Mapping::SetVoxelExtras
          { MEMALIGN(SetVoxelDEC)
            public:
              typedef VoxelDEC VoxType;
              inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3f& d, const float l, const float f)
              {
                const VoxelDEC temp (v, d, l, f);
                iterator existing = std::set<VoxelDEC>::find (temp);
                if (existing == std::set<VoxelDEC>::end())
                  std::set<VoxelDEC>::insert (temp);
                else
                  (*existing).add (d, l, f);
              }
          };
          class SetDixel : public std::set<Dixel>, public Mapping::SetVoxelExtras
          { MEMALIGN(SetDixel)
            public:
              typedef Dixel VoxType;
              inline void insert (const Eigen::Vector3i& v, const size_t d, const float l, const float f)
              {
                const Dixel temp (v, d, l, f);
                iterator existing = std::set<Dixel>::find (temp);
                if (existing == std::set<Dixel>::end())
                  std::set<Dixel>::insert (temp);
                else
                  (*existing).add (l, f);
              }
          };
          class SetVoxelTOD : public std::set<VoxelTOD>, public Mapping::SetVoxelExtras
          { MEMALIGN(SetVoxelTOD)
            public:
              typedef VoxelTOD VoxType;
              inline void insert (const Eigen::Vector3i& v, const Eigen::VectorXf& t, const float l, const float f)
              {
                const VoxelTOD temp (v, t, l, f);
                iterator existing = std::set<VoxelTOD>::find (temp);
                if (existing == std::set<VoxelTOD>::end())
                  std::set<VoxelTOD>::insert (temp);
                else
                  (*existing).add (t, l, f);
              }
          };



        }
      }
    }
  }
}

#endif



