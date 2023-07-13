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
          {
            public:
              VoxelAddon() : sum_factors (0.0) { }
              VoxelAddon (const default_type v) : sum_factors (v) { }
              default_type get_factor() const { return sum_factors; }
            protected:
              void operator+= (const default_type f) const { sum_factors += f; }
              void operator=  (const default_type f) { sum_factors = f; }
              void operator=  (const VoxelAddon& that) { sum_factors = that.sum_factors; }
              void normalize  (const default_type l) const { sum_factors /= l; }
            private:
              mutable default_type sum_factors;
          };




          class Voxel : public Mapping::Voxel, public VoxelAddon
          {

            using Base = Mapping::Voxel;

            public:
            Voxel () = delete;
            Voxel (const Eigen::Vector3i& v, const default_type l, const default_type f) : Base (v, l), VoxelAddon (l * f) { }

            Voxel& operator= (const Voxel& V) { Base::operator= (V); VoxelAddon::operator= (V); return *this; }
            const Voxel& operator+= (const Voxel& V) const { assert (V == *this); IntersectionLength::operator+= (V); VoxelAddon::operator+= (V.get_factor() * V.get_length()); return *this; }
            bool operator== (const Voxel& V) const { return Base::operator== (V); }
            bool operator<  (const Voxel& V) const { return Base::operator<  (V); }

            void normalize() const { VoxelAddon::normalize (get_length()); IntersectionLength::normalize(); }

          };


          class VoxelDEC : public Mapping::VoxelDEC, public VoxelAddon
          {

            using Base = Mapping::VoxelDEC;

            public:
            VoxelDEC () = delete;
            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3d& d, const default_type l, const default_type f) : Base (V, d, l), VoxelAddon (l * f) { }

            VoxelDEC& operator=  (const VoxelDEC& V)   { Base::operator= (V); VoxelAddon::operator= (V); return (*this); }
            const VoxelDEC& operator+= (const VoxelDEC& V) const { assert (V == *this); Base::operator+= (V); VoxelAddon::operator+= (V.get_length() * V.get_factor()); return *this; }
            bool operator== (const VoxelDEC& V) const { return Base::operator== (V); }
            bool operator<  (const VoxelDEC& V) const { return Base::operator<  (V); }

            void normalize() const { VoxelAddon::normalize (get_length()); IntersectionLength::normalize(); }

          };


          class Dixel : public Mapping::Dixel, public VoxelAddon
          {

            using Base = Mapping::Dixel;

            public:
            using index_type = Math::Sphere::Set::index_type;

            Dixel() = delete;
            Dixel (const Eigen::Vector3i&) = delete;
            Dixel (const Eigen::Vector3i& V, const dir_index_type b, const default_type l, const default_type f) : Base (V, b, l), VoxelAddon (l * f) { }

            Dixel& operator=  (const Dixel& V)       { Base::operator= (V); VoxelAddon::operator= (V); return *this; }
            bool   operator== (const Dixel& V) const { return Base::operator== (V); }
            bool   operator<  (const Dixel& V) const { return Base::operator<  (V); }
            const Dixel& operator+= (const Dixel& V) const { assert (V == *this); Base::operator+= (V); VoxelAddon::operator+= (V.get_length() * V.get_factor()); return *this; }

            void normalize() const { VoxelAddon::normalize (get_length()); IntersectionLength::normalize(); }

          };


          class VoxelTOD : public Mapping::VoxelTOD, public VoxelAddon
          {

            using Base = Mapping::VoxelTOD;

            public:
            using vector_type = Eigen::Matrix<default_type, Eigen::Dynamic, 1>;

            VoxelTOD () = delete;
            VoxelTOD (const Eigen::Vector3i& V) : Base (V), VoxelAddon () { }
            VoxelTOD (const Eigen::Vector3i& V, const vector_type& t, const default_type l, const default_type f) : Base (V, t, l), VoxelAddon (l * f) { }

            VoxelTOD& operator=  (const VoxelTOD& V)   { Base::operator= (V); VoxelAddon::operator= (V); return (*this); }
            bool      operator== (const VoxelTOD& V) const { return Base::operator== (V); }
            bool      operator<  (const VoxelTOD& V) const { return Base::operator< (V); }
            const VoxelTOD& operator+= (const VoxelTOD& V) const { assert (V == *this); Base::operator+= (V); VoxelAddon::operator+= (V.get_length() * V.get_factor()); return *this; }

            void normalize() const { VoxelAddon::normalize (get_length()); IntersectionLength::normalize(); }

          };



          class Fixel : public Mapping::Fixel, public VoxelAddon
          {

            using Base = Mapping::Fixel;
            using index_type = Base::index_type;

            public:
            Fixel() = delete;
            Fixel (const index_type F) : Base (F) , VoxelAddon () { }
            Fixel (const index_type F, const default_type l, const default_type f) : Base (F, l), VoxelAddon (l * f) { }

            Fixel& operator= (const Fixel& F) { Base::operator= (F); VoxelAddon::operator= (F); return *this; }
            bool operator== (const Fixel& F) const { return Base::operator== (F); }
            bool operator<  (const Fixel& F) const { return Base::operator<  (F); }
            const Fixel& operator+= (const Fixel& F) const { assert (F == *this); Base::operator+= (F); VoxelAddon::operator+= (F.get_length() * F.get_factor()); return *this; }

            void normalize() const { VoxelAddon::normalize (get_length()); IntersectionLength::normalize(); }

          };






          // Unlike standard TWI, with Gaussian smoothing the TWI factor is stored per point rather than per streamline
          // However, it's handy from a code perspective to still use the same base class
          /*
             class SetVoxelExtras
             {
             public:
             size_t index;
             float weight;
             };
             */





          class SetVoxel : public std::set<Voxel>, public Mapping::SetVoxelExtras
          {
            public:

              using VoxType = Voxel;

              inline void insert (const Eigen::Vector3i& v, const default_type l, const default_type f)
              {
                const Voxel temp (v, l, f);
                iterator existing = std::set<Voxel>::find (temp);
                if (existing == std::set<Voxel>::end())
                  std::set<Voxel>::insert (temp);
                else
                  (*existing) += temp;
              }
          };


          class SetVoxelDEC : public std::set<VoxelDEC>, public Mapping::SetVoxelExtras
          {
            public:

              using VoxType = VoxelDEC;

              inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l, const default_type f)
              {
                const VoxelDEC temp (v, d, l, f);
                iterator existing = std::set<VoxelDEC>::find (temp);
                if (existing == std::set<VoxelDEC>::end())
                  std::set<VoxelDEC>::insert (temp);
                else
                  (*existing) += temp;
              }
          };


          class SetDixel : public std::set<Dixel>, public Mapping::SetVoxelExtras
          {
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
                  (*existing) += temp;
              }
          };


          class SetVoxelTOD : public std::set<VoxelTOD>, public Mapping::SetVoxelExtras
          {
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
                  (*existing) += temp;
              }
          };



          class SetFixel : public std::set<Fixel>, public Mapping::SetVoxelExtras
          {
            public:

              using VoxType = Fixel;

              inline void insert (const MR::Fixel::index_type& F, const default_type l, const default_type f)
              {
                const Fixel temp (F, l, f);
                iterator existing = std::set<Fixel>::find (temp);
                if (existing == std::set<Fixel>::end())
                  std::set<Fixel>::insert (temp);
                else
                  (*existing) += temp;
              }
          };



        }
      }
    }
  }
}

#endif



