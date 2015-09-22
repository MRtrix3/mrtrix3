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


#include "dwi/tractography/seeding/basic.h"

#include "image/adapter/subset.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {


        bool Sphere::get_seed (Point<float>& p)
        {
          do {
            p.set (2.0*rng()-1.0, 2.0*rng()-1.0, 2.0*rng()-1.0);
          } while (p.norm2() > 1.0);
          p = pos + rad*p;
          return true;
        }





        SeedMask::~SeedMask()
        {
          delete mask;
          mask = nullptr;
        }

        bool SeedMask::get_seed (Point<float>& p)
        {
          auto seed = mask->voxel();
          do {
            seed[0] = std::uniform_int_distribution<int>(0, mask->dim(0)-1)(rng.rng);
            seed[1] = std::uniform_int_distribution<int>(0, mask->dim(1)-1)(rng.rng);
            seed[2] = std::uniform_int_distribution<int>(0, mask->dim(2)-1)(rng.rng);
          } while (!seed.value());
          p.set (seed[0]+rng()-0.5, seed[1]+rng()-0.5, seed[2]+rng()-0.5);
          p = mask->transform.voxel2scanner (p);
          return true;
        }





        bool Random_per_voxel::get_seed (Point<float>& p)
        {

          std::lock_guard<std::mutex> lock (mutex);

          if (expired)
            return false;

          if (vox[2] < 0 || ++inc == num) {
            inc = 0;

            do {
              if (++vox[2] == vox.dim(2)) {
                vox[2] = 0;
                if (++vox[1] == vox.dim(1)) {
                  vox[1] = 0;
                  ++vox[0];
                }
              }
            } while (vox[0] != vox.dim(0) && !vox.value());

            if (vox[0] == vox.dim(0)) {
              expired = true;
              return false;
            }
          }

          p.set (vox[0]+rng()-0.5, vox[1]+rng()-0.5, vox[2]+rng()-0.5);
          p = mask->transform.voxel2scanner (p);
          return true;

        }







        bool Grid_per_voxel::get_seed (Point<float>& p)
        {

          std::lock_guard<std::mutex> lock (mutex);

          if (expired)
            return false;

          if (++pos[2] >= os) {
            pos[2] = 0;
            if (++pos[1] >= os) {
              pos[1] = 0;
              if (++pos[0] >= os) {
                pos[0] = 0;

                do {
                  if (++vox[2] == vox.dim(2)) {
                    vox[2] = 0;
                    if (++vox[1] == vox.dim(1)) {
                      vox[1] = 0;
                      ++vox[0];
                    }
                  }
                } while (vox[0] != vox.dim(0) && !vox.value());
                if (vox[0] == vox.dim(0)) {
                  expired = true;
                  return false;
                }
              }
            }
          }

          p.set (vox[0]+offset+(pos[0]*step), vox[1]+offset+(pos[1]*step), vox[2]+offset+(pos[2]*step));
          p = mask->transform.voxel2scanner (p);
          return true;

        }


        Rejection::Rejection (const std::string& in) :
          Base (in, "rejection sampling", MAX_TRACKING_SEED_ATTEMPTS_RANDOM),
          max (0.0)
        {

          Image::Buffer<float> data (in);
          auto vox = data.voxel();
          std::vector<size_t> bottom (vox.ndim(), 0), top (vox.ndim(), 0);
          std::fill_n (bottom.begin(), 3, std::numeric_limits<size_t>::max());

          for (auto i = Image::Loop (0,3) (vox); i; ++i) {
            const float value = vox.value();
            if (value) {
              if (value < 0.0)
                throw Exception ("Cannot have negative values in an image used for rejection sampling!");
              max = MAX (max, value);
              volume += value;
              if (size_t(vox[0]) < bottom[0]) bottom[0] = vox[0];
              if (size_t(vox[0]) > top[0])    top[0]    = vox[0];
              if (size_t(vox[1]) < bottom[1]) bottom[1] = vox[1];
              if (size_t(vox[1]) > top[1])    top[1]    = vox[1];
              if (size_t(vox[2]) < bottom[2]) bottom[2] = vox[2];
              if (size_t(vox[2]) > top[2])    top[2]    = vox[2];
            }
          }

          if (!max)
            throw Exception ("Cannot use image " + in + " for rejection sampling - image is empty");

          if (bottom[0]) --bottom[0];
          if (bottom[1]) --bottom[1];
          if (bottom[2]) --bottom[2];

          top[0] = std::min (size_t (data.dim(0)-bottom[0]), top[0]+2-bottom[0]);
          top[1] = std::min (size_t (data.dim(1)-bottom[1]), top[1]+2-bottom[1]);
          top[2] = std::min (size_t (data.dim(2)-bottom[2]), top[2]+2-bottom[2]);

          Image::Adapter::Subset<decltype(vox)> sub (vox, bottom, top);
          Image::Info info (sub.info());
          if (info.ndim() > 3)
            info.set_ndim (3);

          image.reset (new FloatImage (sub, info, in));

          volume *= image->dim(0) * image->dim(1) * image->dim(2);

        }


        bool Rejection::get_seed (Point<float>& p)
        {
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
          FloatImage::interp_type interp (image->interp);
          Point<float> pos;
          float selector;
          do {
            pos[0] = rng() * (image->dim(0)-1);
            pos[1] = rng() * (image->dim(1)-1);
            pos[2] = rng() * (image->dim(2)-1);
            interp.voxel (pos);
            selector = rng.uniform() * max;
          } while (interp.value() < selector);
          p = interp.voxel2scanner (pos);
#else
          auto seed = image->voxel();
          float selector;
          do {
            seed[0] = std::uniform_int_distribution<int> (0, image->dim(0)-1) (rng.rng);
            seed[1] = std::uniform_int_distribution<int> (0, image->dim(1)-1) (rng.rng);
            seed[2] = std::uniform_int_distribution<int> (0, image->dim(2)-1) (rng.rng);
            selector = rng() * max;
          } while (seed.value() < selector);
          p.set (seed[0]+rng()-0.5, seed[1]+rng()-0.5, seed[2]+rng()-0.5);
          p = image->transform.voxel2scanner (p);
#endif
          return true;
        }




      }
    }
  }
}


