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

#ifndef __dwi_tractography_mapping_voxel_h__
#define __dwi_tractography_mapping_voxel_h__



#include <set>

#include "image.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {



        // Helper functions; note that int[3] rather than Voxel is always used during the mapping itself
        inline Eigen::Vector3i round (const Eigen::Vector3f& p)
        { 
          assert (p.allFinite());
          return { int(std::lround (p[0])), int(std::lround (p[1])), int(std::lround (p[2])) };
        }

        template <class HeaderType>
          inline bool check (const Eigen::Vector3i& V, const HeaderType& H)
          {
            return (V[0] >= 0 && V[0] < H.size(0) && V[1] >= 0 && V[1] < H.size(1) && V[2] >= 0 && V[2] < H.size(2));
          }

        inline Eigen::Vector3f vec2DEC (const Eigen::Vector3f& d)
        {
          return { std::abs(d[0]), std::abs(d[1]), std::abs(d[2]) };
        }




        class Voxel : public Eigen::Vector3i
        { MEMALIGN(Voxel)
          public:
            Voxel (const int x, const int y, const int z) : Eigen::Vector3i (x,y,z), length (1.0f) { }
            Voxel (const Eigen::Vector3i& that) : Eigen::Vector3i (that), length (1.0f) { }
            Voxel (const Eigen::Vector3i& v, const float l) : Eigen::Vector3i (v), length (l) { }
            Voxel () : length (0.0f) { setZero(); }
            bool operator< (const Voxel& V) const { return (((*this)[2] == V[2]) ? (((*this)[1] == V[1]) ? ((*this)[0] < V[0]) : ((*this)[1] < V[1])) : ((*this)[2] < V[2])); }
            Voxel& operator= (const Voxel& V) { Eigen::Vector3i::operator= (V); length = V.length; return *this; }
            void operator+= (const float l) const { length += l; }
            void normalize() const { length = 1.0f; }
            float get_length() const { return length; }
          private:
            mutable float length;
        };


        class VoxelDEC : public Voxel 
        { MEMALIGN(VoxelDEC)

          public:
            VoxelDEC () :
              Voxel (),
              colour (0.0f, 0.0f, 0.0f) { }

            VoxelDEC (const Eigen::Vector3i& V) :
              Voxel (V),
              colour (0.0f, 0.0f, 0.0f) { }

            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3f& d) :
              Voxel (V),
              colour (vec2DEC (d)) { }

            VoxelDEC (const Eigen::Vector3i& V, const Eigen::Vector3f& d, const float l) :
              Voxel (V, l),
              colour (vec2DEC (d)) { }

            VoxelDEC& operator=  (const VoxelDEC& V)   { Voxel::operator= (V); colour = V.colour; return *this; }
            VoxelDEC& operator=  (const Eigen::Vector3i& V) { Voxel::operator= (V); colour = { 0.0f, 0.0f, 0.0f }; return *this; }

            // For sorting / inserting, want to identify the same voxel, even if the colour is different
            bool      operator== (const VoxelDEC& V) const { return Voxel::operator== (V); }
            bool      operator<  (const VoxelDEC& V) const { return Voxel::operator< (V); }

            void normalize() const { colour.normalize(); Voxel::normalize(); }
            void set_dir (const Eigen::Vector3f& i) { colour = vec2DEC (i); }
            void add (const Eigen::Vector3f& i, const float l) const { Voxel::operator+= (l); colour += vec2DEC (i); }
            void operator+= (const Eigen::Vector3f& i) const { Voxel::operator+= (1.0f); colour += vec2DEC (i); }
            const Eigen::Vector3f& get_colour() const { return colour; }

          private:
            mutable Eigen::Vector3f colour;

        };


        // Temporary fix for fixel stats branch
        // Stores precise direction through voxel rather than mapping to a DEC colour or a dixel
        class VoxelDir : public Voxel
        { MEMALIGN(VoxelDir)

          public:
            VoxelDir () :
              Voxel (),
              dir (0.0f, 0.0f, 0.0f) { }

            VoxelDir (const Eigen::Vector3i& V) :
              Voxel (V),
              dir (0.0f, 0.0f, 0.0f) { }

            VoxelDir (const Eigen::Vector3i& V, const Eigen::Vector3f& d) :
              Voxel (V),
              dir (d) { }

            VoxelDir (const Eigen::Vector3i& V, const Eigen::Vector3f& d, const float l) :
              Voxel (V, l),
              dir (d) { }

            VoxelDir& operator=  (const VoxelDir& V)   { Voxel::operator= (V); dir = V.dir; return *this; }
            VoxelDir& operator=  (const Eigen::Vector3i& V) { Voxel::operator= (V); dir = { 0.0f, 0.0f, 0.0f }; return *this; }

            bool      operator== (const VoxelDir& V) const { return Voxel::operator== (V); }
            bool      operator<  (const VoxelDir& V) const { return Voxel::operator< (V); }

            void normalize() const { dir.normalize(); Voxel::normalize(); }
            void set_dir (const Eigen::Vector3f& i) { dir = i; }
            void add (const Eigen::Vector3f& i, const float l) const { Voxel::operator+= (l); dir += i * (dir.dot(i) < 0.0f ? -1.0f : 1.0f); }
            void operator+= (const Eigen::Vector3f& i) const { Voxel::operator+= (1.0f); dir += i * (dir.dot(i) < 0.0f ? -1.0f : 1.0f); }
            const Eigen::Vector3f& get_dir() const { return dir; }

          private:
            mutable Eigen::Vector3f dir;

        };




        // Assumes tangent has been mapped to a hemisphere basis direction set
        class Dixel : public Voxel
        { MEMALIGN(Dixel)

          public:
            Dixel () :
              Voxel (),
              dir (invalid) { }

            Dixel (const Eigen::Vector3i& V) :
              Voxel (V),
              dir (invalid) { }

            Dixel (const Eigen::Vector3i& V, const size_t b) :
              Voxel (V),
              dir (b) { }

            Dixel (const Eigen::Vector3i& V, const size_t b, const float l) :
              Voxel (V, l),
              dir (b) { }

            void set_dir   (const size_t b) { dir = b; }

            bool   valid()     const { return (dir != invalid); }
            size_t get_dir()   const { return dir; }

            Dixel& operator=  (const Dixel& V)       { Voxel::operator= (V); dir = V.dir; return *this; }
            Dixel& operator=  (const Eigen::Vector3i& V)  { Voxel::operator= (V); dir = invalid; return *this; }
            bool   operator== (const Dixel& V) const { return (Voxel::operator== (V) ? (dir == V.dir) : false); }
            bool   operator<  (const Dixel& V) const { return (Voxel::operator== (V) ? (dir <  V.dir) : Voxel::operator< (V)); }
            void   operator+= (const float l)  const { Voxel::operator+= (l); }

          private:
            size_t dir;

            static const size_t invalid;

        };



        // TOD class: tore the SH coefficients in the voxel class so that aPSF generation can be multi-threaded
        // Provide a normalize() function to remove any length dependence, and have unary contribution per streamline
        class VoxelTOD : public Voxel
        { MEMALIGN(VoxelTOD)

          public:
            VoxelTOD () :
              Voxel (),
              sh_coefs () { }

            VoxelTOD (const Eigen::Vector3i& V) :
              Voxel (V),
              sh_coefs () { }

            VoxelTOD (const Eigen::Vector3i& V, const Eigen::VectorXf& t) :
              Voxel (V),
              sh_coefs (t) { }

            VoxelTOD (const Eigen::Vector3i& V, const Eigen::VectorXf& t, const float l) :
              Voxel (V, l),
              sh_coefs (t) { }

            VoxelTOD& operator=  (const VoxelTOD& V)   { Voxel::operator= (V); sh_coefs = V.sh_coefs; return (*this); }
            VoxelTOD& operator=  (const Eigen::Vector3i& V) { Voxel::operator= (V); sh_coefs.resize(0,0); return (*this); }

            // For sorting / inserting, want to identify the same voxel, even if the TOD is different
            bool      operator== (const VoxelTOD& V) const { return Voxel::operator== (V); }
            bool      operator<  (const VoxelTOD& V) const { return Voxel::operator< (V); }

            void normalize() const
            {
              const float multiplier = 1.0f / get_length();
              sh_coefs *= multiplier;
              Voxel::normalize();
            }
            void set_tod (const Eigen::VectorXf& i) { sh_coefs = i; }
            void add (const Eigen::VectorXf& i, const float l) const
            {
              assert (i.size() == sh_coefs.size());
              for (ssize_t index = 0; index != sh_coefs.size(); ++index)
                sh_coefs[index] += l * i[index];
              Voxel::operator+= (l);
            }
            void operator+= (const Eigen::VectorXf& i) const
            {
              assert (i.size() == sh_coefs.size());
              sh_coefs += i;
              Voxel::operator+= (1.0f);
            }
            const Eigen::VectorXf& get_tod() const { return sh_coefs; }

          private:
            mutable Eigen::VectorXf sh_coefs;

        };







        class SetVoxelExtras
        { NOMEMALIGN
          public:
            float factor; // For TWI, when contribution to the map is uniform along the length of the track
            size_t index; // Index of the track
            float weight; // Cross-sectional multiplier for the track
        };






        // Set classes that give sensible behaviour to the insert() function depending on the base voxel class

        class SetVoxel : public std::set<Voxel>, public SetVoxelExtras
        { NOMEMALIGN
          public:
            typedef Voxel VoxType;
            inline void insert (const Voxel& v)
            {
              iterator existing = std::set<Voxel>::find (v);
              if (existing == std::set<Voxel>::end())
                std::set<Voxel>::insert (v);
              else
                (*existing) += v.get_length();
            }
            inline void insert (const Eigen::Vector3i& v, const float l)
            {
              const Voxel temp (v, l);
              insert (temp);
            }
        };





        class SetVoxelDEC : public std::set<VoxelDEC>, public SetVoxelExtras
        { NOMEMALIGN
          public:
            typedef VoxelDEC VoxType;
            inline void insert (const VoxelDEC& v)
            {
              iterator existing = std::set<VoxelDEC>::find (v);
              if (existing == std::set<VoxelDEC>::end())
                std::set<VoxelDEC>::insert (v);
              else
                existing->add (v.get_colour(), v.get_length());
            }
            inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3f& d)
            {
              const VoxelDEC temp (v, d);
              insert (temp);
            }
            inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3f& d, const float l)
            {
              const VoxelDEC temp (v, d, l);
              insert (temp);
            }
        };




        class SetVoxelDir : public std::set<VoxelDir>, public SetVoxelExtras
        { NOMEMALIGN
          public:
            typedef VoxelDir VoxType;
            inline void insert (const VoxelDir& v)
            {
              iterator existing = std::set<VoxelDir>::find (v);
              if (existing == std::set<VoxelDir>::end())
                std::set<VoxelDir>::insert (v);
              else
                existing->add (v.get_dir(), v.get_length());
            }
            inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3f& d)
            {
              const VoxelDir temp (v, d);
              insert (temp);
            }
            inline void insert (const Eigen::Vector3i& v, const Eigen::Vector3f& d, const float l)
            {
              const VoxelDir temp (v, d, l);
              insert (temp);
            }
        };


        class SetDixel : public std::set<Dixel>, public SetVoxelExtras
        { NOMEMALIGN
          public:
            typedef Dixel VoxType;
            inline void insert (const Dixel& v)
            {
              iterator existing = std::set<Dixel>::find (v);
              if (existing == std::set<Dixel>::end())
                std::set<Dixel>::insert (v);
              else
                (*existing) += v.get_length();
            }
            inline void insert (const Eigen::Vector3i& v, const size_t d)
            {
              const Dixel temp (v, d);
              insert (temp);
            }
            inline void insert (const Eigen::Vector3i& v, const size_t d, const float l)
            {
              const Dixel temp (v, d, l);
              insert (temp);
            }
        };





        class SetVoxelTOD : public std::set<VoxelTOD>, public SetVoxelExtras
        { NOMEMALIGN
          public:
            typedef VoxelTOD VoxType;
            inline void insert (const VoxelTOD& v)
            {
              iterator existing = std::set<VoxelTOD>::find (v);
              if (existing == std::set<VoxelTOD>::end())
                std::set<VoxelTOD>::insert (v);
              else
                (*existing) += v.get_tod();
            }
            inline void insert (const Eigen::Vector3i& v, const Eigen::VectorXf& t)
            {
              const VoxelTOD temp (v, t);
              insert (temp);
            }
            inline void insert (const Eigen::Vector3i& v, const Eigen::VectorXf& t, const float l)
            {
              const VoxelTOD temp (v, t, l);
              insert (temp);
            }
        };



      }
    }
  }
}

#endif



