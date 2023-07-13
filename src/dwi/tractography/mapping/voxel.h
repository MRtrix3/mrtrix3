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

#ifndef __dwi_tractography_mapping_voxel_h__
#define __dwi_tractography_mapping_voxel_h__



#include <set>

#include "image.h"
#include "fixel/fixel.h"
#include "math/sphere/set/set.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {



        // Helper functions; note that int[3] rather than Voxel is always used during the mapping itself
        template <typename T>
        inline Eigen::Vector3i round (const Eigen::Matrix<T, 3, 1>& p)
        {
          assert (p.allFinite());
          return { int(std::round (p[0])), int(std::round (p[1])), int(std::round (p[2])) };
        }

        template <class HeaderType>
          inline bool check (const Eigen::Vector3i& V, const HeaderType& H)
          {
            return (V[0] >= 0 && V[0] < H.size(0) && V[1] >= 0 && V[1] < H.size(1) && V[2] >= 0 && V[2] < H.size(2));
          }

        inline Eigen::Vector3d vec2DEC (const Eigen::Vector3d& d)
        {
          return { abs(d[0]), abs(d[1]), abs(d[2]) };
        }



        class IntersectionLength
        {
          public:
            IntersectionLength() : length (0.0) { }
            IntersectionLength (const default_type l) : length (l) { }
            IntersectionLength operator+= (const default_type l) const { length += l; return *this; }
            IntersectionLength operator+= (const IntersectionLength& that) const { length += that.get_length(); return *this; }
            void normalize() const { length = 1.0; }
            void set_length (const default_type l) { length = l; }
            default_type get_length() const { return length; }
          private:
            mutable default_type length;
        };




        class Voxel : public Eigen::Vector3i, public IntersectionLength
        {
          public:
            using vox_type = Eigen::Vector3i;
            using length_type = IntersectionLength;
            Voxel (const int x, const int y, const int z) : vox_type (x,y,z), length_type (1.0f) { }
            Voxel (const vox_type& that) : vox_type (that), length_type (1.0f) { }
            Voxel (const vox_type& v, const default_type l) : Eigen::Vector3i (v), length_type (l) { }
            Voxel () { vox_type::setZero(); }
            bool operator< (const Voxel& V) const { return (((*this)[2] == V[2]) ? (((*this)[1] == V[1]) ? ((*this)[0] < V[0]) : ((*this)[1] < V[1])) : ((*this)[2] < V[2])); }
            Voxel& operator= (const Voxel& V) { vox_type::operator= (V); set_length (V.get_length()); return *this; }
            bool operator== (const Voxel& V) const { return vox_type::operator== (V); }
            const Voxel& operator+= (const Voxel& that) const { IntersectionLength::operator+= (that); return *this; }
        };



        class VoxelDEC : public Voxel
        {

          public:
            VoxelDEC () :
              Voxel (),
              colour (0.0, 0.0, 0.0) { }

            VoxelDEC (const Eigen::Vector3i& V) :
              Voxel (V),
              colour (0.0, 0.0, 0.0) { }

            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3d& d) :
              Voxel (V),
              colour (vec2DEC (d)) { }

            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3d& d, const float l) :
              Voxel (V, l),
              colour (vec2DEC (d)) { }

            VoxelDEC& operator=  (const VoxelDEC& V)   { Voxel::operator= (V); colour = V.colour; return *this; }
            VoxelDEC& operator=  (const Eigen::Vector3i& V) { Voxel::operator= (V); colour = { 0.0, 0.0, 0.0 }; return *this; }

            // For sorting / inserting, want to identify the same voxel, even if the colour is different
            bool      operator== (const VoxelDEC& V) const { return Voxel::operator== (V); }
            bool      operator<  (const VoxelDEC& V) const { return Voxel::operator< (V); }

            void normalize() const { colour.normalize(); IntersectionLength::normalize(); }
            void set_dir (const Eigen::Vector3d& i) { colour = vec2DEC (i); }
            void add (const Eigen::Vector3d& i, const default_type l) const { IntersectionLength::operator+= (l); colour += vec2DEC (i); }
            const VoxelDEC& operator+= (const VoxelDEC& that) const { assert (that == *this); IntersectionLength::operator+= (that); colour += that.colour; return *this; }
            const Eigen::Vector3d& get_colour() const { return colour; }

          private:
            mutable Eigen::Vector3d colour;

        };



        // Stores precise direction through voxel rather than mapping to a DEC colour or a dixel
        class VoxelDir : public Voxel
        {

          public:
            VoxelDir () :
              Voxel (),
              dir (0.0, 0.0, 0.0) { }

            VoxelDir (const Eigen::Vector3i& V) :
              Voxel (V),
              dir (0.0, 0.0, 0.0) { }

            VoxelDir (const Eigen::Vector3i& V, const Eigen::Vector3d& d) :
              Voxel (V),
              dir (d) { }

            VoxelDir (const Eigen::Vector3i& V, const Eigen::Vector3d& d, const default_type l) :
              Voxel (V, l),
              dir (d) { }

            VoxelDir& operator=  (const VoxelDir& V)   { Voxel::operator= (V); dir = V.dir; return *this; }
            VoxelDir& operator=  (const Eigen::Vector3i& V) { Voxel::operator= (V); dir = { 0.0, 0.0, 0.0 }; return *this; }

            bool      operator== (const VoxelDir& V) const { return Voxel::operator== (V); }
            bool      operator<  (const VoxelDir& V) const { return Voxel::operator< (V); }

            void normalize() const { dir.normalize(); IntersectionLength::normalize(); }
            void set_dir (const Eigen::Vector3d& i) { dir = i; }
            void add (const Eigen::Vector3d& i, const default_type l) const { IntersectionLength::operator+= (l); dir += i * (dir.dot(i) < 0.0 ? -1.0 : 1.0); }
            const VoxelDir& operator+= (const VoxelDir& that) const { assert (that == *this); dir += that.get_length() * that.dir * (dir.dot(that.dir) < 0.0 ? -1.0 : 1.0); IntersectionLength::operator+= (that.get_length()); return *this; }
            const Eigen::Vector3d& get_dir() const { return dir; }

          private:
            mutable Eigen::Vector3d dir;

        };



        // Assumes tangent has been mapped to a hemisphere basis direction set
        class Dixel : public Voxel
        {

          public:

            using dir_index_type = Math::Sphere::Set::index_type;

            Dixel () = delete;
            Dixel (const Eigen::Vector3i& V) = delete;

            Dixel (const Eigen::Vector3i& V, const dir_index_type b) :
              Voxel (V),
              dir (b) { }

            Dixel (const Eigen::Vector3i& V, const dir_index_type b, const default_type l) :
              Voxel (V, l),
              dir (b) { }

            void set_dir (const size_t b) { dir = b; }

            dir_index_type get_dir() const { return dir; }

            Dixel& operator=  (const Dixel& V) { Voxel::operator= (V); dir = V.dir; return *this; }
            Dixel& operator=  (const Eigen::Vector3i& V) { Voxel::operator= (V); /*dir = invalid;*/ return *this; }
            bool   operator== (const Dixel& V) const { return (Voxel::operator== (V) ? (dir == V.dir) : false); }
            bool   operator<  (const Dixel& V) const { return (Voxel::operator== (V) ? (dir <  V.dir) : Voxel::operator< (V)); }
            const Dixel& operator+= (const Dixel& that) const { assert (that == *this); IntersectionLength::operator+= (that); return *this; }

          private:
            dir_index_type dir;

        };



        // TOD class: Store the SH coefficients in the voxel class so that aPSF generation can be multi-threaded
        // Provide a normalize() function to remove any length dependence, and have unary contribution per streamline
        class VoxelTOD : public Voxel
        {

          public:

            using vector_type = Eigen::Matrix<default_type, Eigen::Dynamic, 1>;

            VoxelTOD () :
              Voxel (),
              sh_coefs () { }

            VoxelTOD (const Eigen::Vector3i& V) :
              Voxel (V),
              sh_coefs () { }

            VoxelTOD (const Eigen::Vector3i& V, const vector_type& t) :
              Voxel (V),
              sh_coefs (t) { }

            VoxelTOD (const Eigen::Vector3i& V, const vector_type& t, const default_type l) :
              Voxel (V, l),
              sh_coefs (t) { }

            VoxelTOD& operator=  (const VoxelTOD& V)        { Voxel::operator= (V); sh_coefs = V.sh_coefs; return (*this); }
            VoxelTOD& operator=  (const Eigen::Vector3i& V) { Voxel::operator= (V); sh_coefs.resize(0); return (*this); }

            // For sorting / inserting, want to identify the same voxel, even if the TOD is different
            bool      operator== (const VoxelTOD& V) const { return Voxel::operator== (V); }
            bool      operator<  (const VoxelTOD& V) const { return Voxel::operator< (V); }

            void normalize() const
            {
              const default_type multiplier = 1.0 / get_length();
              sh_coefs *= multiplier;
              IntersectionLength::normalize();
            }
            void set_tod (const vector_type& i) { sh_coefs = i; }
            const VoxelTOD& operator+= (const VoxelTOD& that) const
            {
              assert (that == *this);
              assert (that.sh_coefs.size() == sh_coefs.size());
              sh_coefs += that.sh_coefs;
              IntersectionLength::operator+= (that);
              return *this;
            }
            const vector_type& get_tod() const { return sh_coefs; }

          private:
            mutable vector_type sh_coefs;

        };



        class Fixel : public IntersectionLength
        {
          public:
            using index_type = MR::Fixel::index_type;
            using length_type = IntersectionLength;
            Fixel() = delete;
            Fixel (const index_type f) : length_type (1.0), index (f) { }
            Fixel (const index_type f, const default_type l) : length_type (l), index (f) { }
            bool operator== (const Fixel& that) const { return index == that.index; }
            bool operator< (const Fixel& F) const { return (index < F.index); }
            Fixel& operator= (const Fixel& F) { index = F.index; set_length (F.get_length()); return *this; }
            const Fixel& operator+= (const Fixel& that) const { assert (that.index == index); IntersectionLength::operator+= (that); return *this; }
            operator index_type() const { return index; }
          private:
            index_type index;
        };



        std::ostream& operator<< (std::ostream&, const Voxel&);
        std::ostream& operator<< (std::ostream&, const VoxelDEC&);
        std::ostream& operator<< (std::ostream&, const VoxelDir&);
        std::ostream& operator<< (std::ostream&, const Dixel&);
        std::ostream& operator<< (std::ostream&, const VoxelTOD&);
        std::ostream& operator<< (std::ostream&, const Fixel&);



        class SetVoxelExtras
        {
          public:
            default_type factor; // For TWI, when contribution to the map is uniform along the length of the track
            size_t index; // Index of the track
            default_type weight; // Cross-sectional multiplier for the track
        };






        // Set classes that give sensible behaviour to the insert() function depending on the base voxel class

        class SetVoxel : public std::set<Voxel>, public SetVoxelExtras
        {
          public:
            using VoxType = Voxel;
            inline void insert (const Voxel& v)
            {
              iterator existing = std::set<Voxel>::find (v);
              if (existing == std::set<Voxel>::end())
                std::set<Voxel>::insert (v);
              else
                (*existing) += v;
            }
            inline void insert (const Eigen::Vector3i& v, const default_type l)
            {
              const Voxel temp (v, l);
              insert (temp);
            }
        };





        class SetVoxelDEC : public std::set<VoxelDEC>, public SetVoxelExtras
        {
          public:
            using VoxType = VoxelDEC;
            inline void insert (const VoxelDEC& v)
            {
              iterator existing = std::set<VoxelDEC>::find (v);
              if (existing == std::set<VoxelDEC>::end())
                std::set<VoxelDEC>::insert (v);
              else
                (*existing) += v;
            }
            inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3d& d)
            {
              const VoxelDEC temp (v, d);
              insert (temp);
            }
            inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l)
            {
              const VoxelDEC temp (v, d, l);
              insert (temp);
            }
        };




        class SetVoxelDir : public std::set<VoxelDir>, public SetVoxelExtras
        {
          public:
            using VoxType = VoxelDir;
            inline void insert (const VoxelDir& v)
            {
              iterator existing = std::set<VoxelDir>::find (v);
              if (existing == std::set<VoxelDir>::end())
                std::set<VoxelDir>::insert (v);
              else
                (*existing) += v;
            }
            inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3d& d)
            {
              const VoxelDir temp (v, d);
              insert (temp);
            }
            inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3d& d, const default_type l)
            {
              const VoxelDir temp (v, d, l);
              insert (temp);
            }
        };


        class SetDixel : public std::set<Dixel>, public SetVoxelExtras
        {
          public:

            using VoxType = Dixel;
            using dir_index_type = Dixel::dir_index_type;

            inline void insert (const Dixel& v)
            {
              iterator existing = std::set<Dixel>::find (v);
              if (existing == std::set<Dixel>::end())
                std::set<Dixel>::insert (v);
              else
                (*existing) += v;
            }
            inline void insert (const Eigen::Vector3i& v, const dir_index_type d)
            {
              const Dixel temp (v, d);
              insert (temp);
            }
            inline void insert (const Eigen::Vector3i& v, const dir_index_type d, const default_type l)
            {
              const Dixel temp (v, d, l);
              insert (temp);
            }
        };





        class SetVoxelTOD : public std::set<VoxelTOD>, public SetVoxelExtras
        {
          public:

            using VoxType = VoxelTOD;
            using vector_type = VoxelTOD::vector_type;

            inline void insert (const VoxelTOD& v)
            {
              iterator existing = std::set<VoxelTOD>::find (v);
              if (existing == std::set<VoxelTOD>::end())
                std::set<VoxelTOD>::insert (v);
              else
                (*existing) += v;
            }
            inline void insert (const Eigen::Vector3i& v, const vector_type& t)
            {
              const VoxelTOD temp (v, t);
              insert (temp);
            }
            inline void insert (const Eigen::Vector3i& v, const vector_type& t, const default_type l)
            {
              const VoxelTOD temp (v, t, l);
              insert (temp);
            }
        };



        class SetFixel : public std::set<Fixel>, public SetVoxelExtras
        {
          public:
            // TODO Rename
            using VoxType = Fixel;
            inline void insert (const Fixel& f)
            {
              iterator existing = std::set<Fixel>::find (f);
              if (existing == std::set<Fixel>::end())
                std::set<Fixel>::insert (f);
              else
                (*existing) += f;
            }
            inline void insert (const Fixel::index_type f, const default_type l)
            {
              const Fixel temp (f, l);
              insert (temp);
            }
        };



      }
    }
  }
}

#endif



