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
          Base (in, rng, "sphere")
        {
          std::vector<float> F (parse_floats (in));
          if (F.size() != 4)
            throw Exception ("Could not parse seed \"" + in + "\" as a spherical seed point; needs to be 4 comma-separated values (XYZ position, then radius)");
          pos.set (F[0], F[1], F[2]);
          rad = F[3];
          volume = 4.0*M_PI*Math::pow3(rad)/3.0;
        }

        bool get_seed (Point<float>& p)
        {
          do {
            p.set (2.0*rng.uniform()-1.0, 2.0*rng.uniform()-1.0, 2.0*rng.uniform()-1.0);
          } while (p.norm2() > 1.0);
          p = pos + rad*p;
          return true;
        }


        private:
        Point<float> pos;
        float rad;

      };


      class Default : public Base
      {

        public:
          Default (const std::string& in, const Math::RNG& rng) :
            Base (in, rng, "random")
          {
            mask = Tractography::get_mask (in);
            volume = get_count (*mask) * mask->vox(0) * mask->vox(1) * mask->vox(2);
          }

          ~Default()
          {
            delete mask;
            mask = NULL;
          }

          bool get_seed (Point<float>& p)
          {
            Mask::voxel_type seed (*mask);
            do {
              seed[0] = rng.uniform_int (mask->dim(0));
              seed[1] = rng.uniform_int (mask->dim(1));
              seed[2] = rng.uniform_int (mask->dim(2));
            } while (!seed.value());
            p.set (seed[0]+rng.uniform()-0.5, seed[1]+rng.uniform()-0.5, seed[2]+rng.uniform()-0.5);
            p = mask->transform.voxel2scanner (p);
            return true;
          }

        private:
          Mask* mask;

      };



      class Random_per_voxel : public Base
      {

        public:
          Random_per_voxel (const std::string& in, const Math::RNG& rng, const size_t num_per_voxel) :
            Base (in, rng, "random per voxel"),
            num (num_per_voxel),
            vox (0, 0, -1),
            inc (0),
            expired (false)
          {
            mask = Tractography::get_mask (in);
            count = get_count (*mask) * num_per_voxel;
          }

          ~Random_per_voxel()
          {
            delete mask;
            mask = NULL;
          }

          bool get_seed (Point<float>& p)
          {

            if (expired)
              return false;

            Thread::Mutex::Lock lock (mutex);

            if (vox[2] < 0 || ++inc == num) {
              inc = 0;
              ++vox[2];
              Mask::voxel_type v (*mask);
              Image::Nav::set_pos (v, vox);

              while (v[0] != v.dim(0) && !v.value()) {
                if (++v[2] == v.dim(2)) {
                  v[2] = 0;
                  if (++v[1] == v.dim(1)) {
                    v[1] = 0;
                    ++v[0];
                  }
                }
              }

              if (v[0] == v.dim(0)) {
                expired = true;
                return false;
              }
              vox[0] = v[0]; vox[1] = v[1], vox[2] = v[2];
            }

            p.set (vox[0]+rng.uniform()-0.5, vox[1]+rng.uniform()-0.5, vox[2]+rng.uniform()-0.5);
            p = mask->transform.voxel2scanner (p);
            return true;

          }


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
            Base (in, rng, "grid per voxel"),
            os (os_factor),
            vox (0, 0, -1),
            pos (os, os, os),
            offset (-0.5 + (1.0 / (2*os))),
            step (1.0 / os),
            expired (false)
          {
            mask = Tractography::get_mask (in);
            count = get_count (*mask) * Math::pow3 (os_factor);
          }

          ~Grid_per_voxel()
          {
            delete mask;
            mask = NULL;
          }

          bool get_seed (Point<float>& p)
          {

            if (expired)
              return false;

            Thread::Mutex::Lock lock (mutex);

            if (++pos[2] >= os) {
              pos[2] = 0;
              if (++pos[1] >= os) {
                pos[1] = 0;
                if (++pos[0] >= os) {
                  pos[0] = 0;
                  ++vox[2];

                  Mask::voxel_type v (*mask);
                  Image::Nav::set_pos (v, vox);

                  while (v[0] != v.dim(0) && !v.value()) {
                    if (++v[2] == v.dim(2)) {
                      v[2] = 0;
                      if (++v[1] == v.dim(1)) {
                        v[1] = 0;
                        ++v[0];
                      }
                    }
                  }
                  if (v[0] == v.dim(0)) {
                    expired = true;
                    return false;
                  }
                  vox[0] = v[0]; vox[1] = v[1], vox[2] = v[2];
                }
              }
            }

            p.set (vox[0]+offset+(pos[0]*step), vox[1]+offset+(pos[1]*step), vox[2]+offset+(pos[2]*step));
            p = mask->transform.voxel2scanner (p);
            return true;

          }


        private:
          Mask* mask;
          const int os;
          Point<int> vox, pos;
          const float offset, step;
          bool expired;

      };




      class Rejection : public Base
      {

          class FloatImage : public Image::BufferScratch<float> {
            public:
            template <class InputVoxelType>
            FloatImage (InputVoxelType& D, const Image::Info& info, const std::string& description) :
            Image::BufferScratch<float> (info, description),
            transform (this->info())
            {
              Image::BufferScratch<float>::voxel_type this_vox (*this);
              Image::copy (D, this_vox);
            }
            Image::Transform transform;
          };


          public:
          Rejection (const std::string&, const Math::RNG&);

          bool get_seed (Point<float>& p)
          {
            FloatImage::voxel_type seed (*image);
            float selector;
            do {
              seed[0] = rng.uniform_int (image->dim(0));
              seed[1] = rng.uniform_int (image->dim(1));
              seed[2] = rng.uniform_int (image->dim(2));
              selector = rng.uniform() * max;
            } while (seed.value() < selector);
            p.set (seed[0]+rng.uniform()-0.5, seed[1]+rng.uniform()-0.5, seed[2]+rng.uniform()-0.5);
            p = image->transform.voxel2scanner (p);
            return true;
          }


          private:
          RefPtr<FloatImage> image;
          float max;

      };






      }
    }
  }
}

#endif

