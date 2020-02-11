/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
              VoxelAddon (const default_type v) : sum_factors (v) { }
              default_type get_factor() const { return sum_factors; }
            protected:
              void operator+= (const default_type f) const { sum_factors += f; }
              void operator=  (const default_type f) { sum_factors = f; }
              void operator=  (const VoxelAddon& that) { sum_factors = that.sum_factors; }
              void normalize (const default_type l) const { sum_factors /= l; }
            private:
              mutable default_type sum_factors;
          };




          class Voxel : public Mapping::Voxel, public VoxelAddon
          { MEMALIGN(Voxel)

            using Base = Mapping::Voxel;

            public:
            Voxel (const int x, const int y, const int z) : Base (x, y, z) , VoxelAddon () { }
            Voxel (const Eigen::Vector3i& that) : Base (that), VoxelAddon() { }
            Voxel (const Eigen::Vector3i& v, const default_type l) : Base (v, l), VoxelAddon () { }
            Voxel (const Eigen::Vector3i& v, const default_type l, const default_type f) : Base (v, l), VoxelAddon (f) { }
            Voxel () : Base (), VoxelAddon () { }

            Voxel& operator= (const Voxel& V) { Base::operator= (V); VoxelAddon::operator= (V); return *this; }
            void operator+= (const default_type l) const { Base::operator+= (l); }
            bool operator== (const Voxel& V) const { return Base::operator== (V); }
            bool operator<  (const Voxel& V) const { return Base::operator<  (V); }

            void add (const default_type l, const default_type f) const { Base::operator+= (l); VoxelAddon::operator+= (f); }
            void normalize() const { VoxelAddon::normalize (get_length()); Base::normalize(); }

          };


          class VoxelDEC : public Mapping::VoxelDEC, public VoxelAddon
          { MEMALIGN(VoxelDEC)

            using Base = Mapping::VoxelDEC;

            public:
            VoxelDEC () : Base (), VoxelAddon () { }
            VoxelDEC (const Eigen::Vector3i& V) : Base (V), VoxelAddon () { }
            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3& d) : Base (V, d), VoxelAddon () { }
            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3& d, const default_type l) : Base (V, d, l), VoxelAddon () { }
            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3& d, const default_type l, const default_type f) : Base (V, d, l), VoxelAddon (f) { }

            VoxelDEC& operator=  (const VoxelDEC& V)   { Base::operator= (V); VoxelAddon::operator= (V); return (*this); }
            void operator+= (const default_type) const { assert (0); }
            void operator+= (const Eigen::Vector3&) const { assert (0); }
            bool operator== (const VoxelDEC& V) const { return Base::operator== (V); }
            bool operator<  (const VoxelDEC& V) const { return Base::operator<  (V); }

            void add (const Eigen::Vector3&, const default_type) const { assert (0); }
            void add (const Eigen::Vector3& i, const default_type l, const default_type f) const { Base::add (i, l); VoxelAddon::operator+= (f); }
            void normalize() const { VoxelAddon::normalize (get_length()); Base::normalize(); }

          };


          class Dixel : public Mapping::Dixel, public VoxelAddon
          { MEMALIGN(Dixel)

            using Base = Mapping::Dixel;

            public:
            using index_type = DWI::Directions::index_type;

            Dixel () : Base (), VoxelAddon () { }
            Dixel (const Eigen::Vector3i& V) : Base (V), VoxelAddon () { }
            Dixel (const Eigen::Vector3i& V, const dir_index_type b) : Base (V, b), VoxelAddon () { }
            Dixel (const Eigen::Vector3i& V, const dir_index_type b, const default_type l) : Base (V, b, l), VoxelAddon () { }
            Dixel (const Eigen::Vector3i& V, const dir_index_type b, const default_type l, const default_type f) : Base (V, b, l), VoxelAddon (f) { }

            Dixel& operator=  (const Dixel& V)       { Base::operator= (V); VoxelAddon::operator= (V); return *this; }
            bool   operator== (const Dixel& V) const { return Base::operator== (V); }
            bool   operator<  (const Dixel& V) const { return Base::operator<  (V); }
            void   operator+= (const default_type)  const { assert (0); }

            void add (const default_type l, const default_type f) const { Base::operator+= (l); VoxelAddon::operator+= (f); }
            void normalize() const { VoxelAddon::normalize (get_length()); Base::normalize(); }

          };


          class VoxelTOD : public Mapping::VoxelTOD, public VoxelAddon
          { MEMALIGN(VoxelTOD)

            using Base = Mapping::VoxelTOD;

            public:
            using vector_type = Eigen::Matrix<default_type, Eigen::Dynamic, 1>;

            VoxelTOD () : Base (), VoxelAddon () { }
            VoxelTOD (const Eigen::Vector3i& V) : Base (V), VoxelAddon () { }
            VoxelTOD (const Eigen::Vector3i& V, const vector_type& t) : Base (V, t), VoxelAddon () { }
            VoxelTOD (const Eigen::Vector3i& V, const vector_type& t, const default_type l) : Base (V, t, l), VoxelAddon () { }
            VoxelTOD (const Eigen::Vector3i& V, const vector_type& t, const default_type l, const default_type f) : Base (V, t, l), VoxelAddon (f) { }

            VoxelTOD& operator=  (const VoxelTOD& V)   { Base::operator= (V); VoxelAddon::operator= (V); return (*this); }
            bool      operator== (const VoxelTOD& V) const { return Base::operator== (V); }
            bool      operator<  (const VoxelTOD& V) const { return Base::operator< (V); }
            void      operator+= (const vector_type&) const { assert (0); }

            void add (const vector_type&, const default_type) const { assert (0); }
            void add (const vector_type& i, const default_type l, const default_type f) const { Base::add (i, l); VoxelAddon::operator+= (f); }
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

              using VoxType = Voxel;

              inline void insert (const Eigen::Vector3i& v, const default_type l, const default_type f)
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

              using VoxType = VoxelDEC;

              inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3& d, const default_type l, const default_type f)
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

              using VoxType = Dixel;
              using dir_index_type = Dixel::dir_index_type;

              inline void insert (const Eigen::Vector3i& v, const dir_index_type d, const default_type l, const default_type f)
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

              using VoxType = VoxelTOD;
              using vector_type = VoxelTOD::vector_type;

              inline void insert (const Eigen::Vector3i& v, const vector_type& t, const default_type l, const default_type f)
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



