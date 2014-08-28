/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2012.

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

#ifndef __dwi_tractography_seeding_basic_h__
#define __dwi_tractography_seeding_basic_h__



#include "ptr.h"

#include "dwi/tractography/roi.h"

#include "dwi/tractography/seeding/base.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {


        class Sphere : public Base
        {

          public:
            Sphere (const std::string& in, const Math::RNG& rng) :
              Base (in, rng, "sphere", MAX_TRACKING_SEED_ATTEMPTS_RANDOM) {
                std::vector<float> F (parse_floats (in));
                if (F.size() != 4)
                  throw Exception ("Could not parse seed \"" + in + "\" as a spherical seed point; needs to be 4 comma-separated values (XYZ position, then radius)");
                pos.set (F[0], F[1], F[2]);
                rad = F[3];
                volume = 4.0*Math::pi*Math::pow3(rad)/3.0;
              }

            virtual bool get_seed (Point<float>& p);

          private:
            Point<float> pos;
            float rad;

        };


        class SeedMask : public Base
        {

          public:
            SeedMask (const std::string& in, const Math::RNG& rng) :
              Base (in, rng, "random seeding mask", MAX_TRACKING_SEED_ATTEMPTS_RANDOM) {
                mask = Tractography::get_mask (in);
                volume = get_count (*mask) * mask->vox(0) * mask->vox(1) * mask->vox(2);
              }

            virtual ~SeedMask();
            virtual bool get_seed (Point<float>& p);

          private:
            Mask* mask;

        };



        class Random_per_voxel : public Base
        {

          public:
            Random_per_voxel (const std::string& in, const Math::RNG& rng, const size_t num_per_voxel) :
              Base (in, rng, "random per voxel", MAX_TRACKING_SEED_ATTEMPTS_FIXED),
              num (num_per_voxel),
              vox (0, 0, -1),
              inc (0),
              expired (false) {
                mask = Tractography::get_mask (in);
                count = get_count (*mask) * num_per_voxel;
              }

            virtual ~Random_per_voxel();
            virtual bool get_seed (Point<float>& p);

          private:
            Mask* mask;
            const size_t num;
            Point<int> vox;
            uint32_t inc;
            bool expired;

        };



        class Grid_per_voxel : public Base
        {

          public:
            Grid_per_voxel (const std::string& in, const Math::RNG& rng, const size_t os_factor) :
              Base (in, rng, "grid per voxel", MAX_TRACKING_SEED_ATTEMPTS_FIXED),
              os (os_factor),
              vox (0, 0, -1),
              pos (os, os, os),
              offset (-0.5 + (1.0 / (2*os))),
              step (1.0 / os),
              expired (false) {
                mask = Tractography::get_mask (in);
                count = get_count (*mask) * Math::pow3 (os_factor);
              }

            virtual ~Grid_per_voxel();
            virtual bool get_seed (Point<float>& p);

          private:
            Mask* mask;
            const int os;
            Point<int> vox, pos;
            const float offset, step;
            bool expired;

        };




        class Rejection : public Base
        {

          private:
            class FloatImage : public Image::BufferScratch<float> {
              public:
                template <class InputVoxelType>
                  FloatImage (InputVoxelType& D, const Image::Info& info, const std::string& description) :
                    Image::BufferScratch<float> (info, description), transform (this->info()) {
                      Image::BufferScratch<float>::voxel_type this_vox (*this);
                      Image::copy (D, this_vox);
                    }
                Image::Transform transform;
            };


          public:
            Rejection (const std::string&, const Math::RNG&);

            virtual bool get_seed (Point<float>& p);

          private:
            RefPtr<FloatImage> image;
            float max;

        };






      }
    }
  }
}

#endif

