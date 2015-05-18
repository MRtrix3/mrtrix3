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
#include "adapter/subset.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {


        bool Sphere::get_seed (Math::RNG::Uniform<float>& rng, Eigen::Vector3f& p) 
        {
          do {
            p = { 2.0f*rng()-1.0f, 2.0f*rng()-1.0f, 2.0f*rng()-1.0f };
          } while (p.squaredNorm() > 1.0f);
          p = pos + rad*p;
          return true;
        }





        bool SeedMask::get_seed (Math::RNG::Uniform<float>& rng, Eigen::Vector3f& p) 
        {
          auto seed = *mask;
          do {
            seed.index(0) = std::uniform_int_distribution<int>(0, mask->size(0)-1)(rng.rng);
            seed.index(1) = std::uniform_int_distribution<int>(0, mask->size(1)-1)(rng.rng);
            seed.index(2) = std::uniform_int_distribution<int>(0, mask->size(2)-1)(rng.rng);
          } while (!seed.value());
          p = { seed.index(0)+rng()-0.5f, seed.index(1)+rng()-0.5f, seed.index(2)+rng()-0.5f };
          p = mask->voxel2scanner.cast<float>() * p;
          return true;
        }






        bool Random_per_voxel::get_seed (Math::RNG::Uniform<float>& rng, Eigen::Vector3f& p) 
        {

          if (expired)
            return false;

          std::lock_guard<std::mutex> lock (mutex);

          if (vox[2] < 0 || ++inc == num) {
            inc = 0;
            auto v = *mask;
            assign_pos_of (vox).to (v);

            do {
              if (++v.index(2) == v.size(2)) {
                v.index(2) = 0;
                if (++v.index(1) == v.size(1)) {
                  v.index(1) = 0;
                  ++v.index(0);
                }
              }
            } while (v.index(0) != v.size(0) && !v.value());

            if (v.index(0) == v.size(0)) {
              expired = true;
              return false;
            }
            vox[0] = v.index(0); vox[1] = v.index(1), vox[2] = v.index(2);
            assign_pos_of (v).to (vox);
          }

          p = { vox[0]+rng()-0.5f, vox[1]+rng()-0.5f, vox[2]+rng()-0.5f };
          p = mask->voxel2scanner.cast<float>() * p;
          return true;

        }







        bool Grid_per_voxel::get_seed (Math::RNG::Uniform<float>&, Eigen::Vector3f& p) 
        {

          if (expired)
            return false;

          std::lock_guard<std::mutex> lock (mutex);

          if (++pos[2] >= os) {
            pos[2] = 0;
            if (++pos[1] >= os) {
              pos[1] = 0;
              if (++pos[0] >= os) {
                pos[0] = 0;

                auto v = *mask;
                assign_pos_of (vox).to (v);

                do {
                  if (++v.index(2) == v.size(2)) {
                    v.index(2) = 0;
                    if (++v.index(1) == v.size(1)) {
                      v.index(1) = 0;
                      ++v.index(0);
                    }
                  }
                } while (v.index(0) != v.size(0) && !v.value());
                if (v.index(0) == v.size(0)) {
                  expired = true;
                  return false;
                }
                assign_pos_of (v).to (vox);
              }
            }
          }

          p = { vox[0]+offset+(pos[0]*step), vox[1]+offset+(pos[1]*step), vox[2]+offset+(pos[2]*step) };
          p = mask->voxel2scanner.cast<float>() * p;
          return true;

        }


        Rejection::Rejection (const std::string& in) :
          Base (in, "rejection sampling", MAX_TRACKING_SEED_ATTEMPTS_RANDOM),
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
          interp (in),
#endif
          max (0.0)
        {
          auto vox = Image<float>::open (in);
          std::vector<size_t> bottom (vox.ndim(), 0), top (vox.ndim(), 0);
          std::fill_n (bottom.begin(), 3, std::numeric_limits<size_t>::max());

          for (auto i = Loop (0,3) (vox); i; ++i) {
            const float value = vox.value();
            if (value) {
              if (value < 0.0)
                throw Exception ("Cannot have negative values in an image used for rejection sampling!");
              max = std::max (max, value);
              volume += value;
              if (size_t(vox.index(0)) < bottom[0]) bottom[0] = vox.index(0);
              if (size_t(vox.index(0)) > top[0])    top[0]    = vox.index(0);
              if (size_t(vox.index(1)) < bottom[1]) bottom[1] = vox.index(1);
              if (size_t(vox.index(1)) > top[1])    top[1]    = vox.index(1);
              if (size_t(vox.index(2)) < bottom[2]) bottom[2] = vox.index(2);
              if (size_t(vox.index(2)) > top[2])    top[2]    = vox.index(2);
            }
          }

          if (!max)
            throw Exception ("Cannot use image " + in + " for rejection sampling - image is empty");

          if (bottom[0]) --bottom[0];
          if (bottom[1]) --bottom[1];
          if (bottom[2]) --bottom[2];

          top[0] = std::min (size_t (vox.size(0)-bottom[0]), top[0]+2-bottom[0]);
          top[1] = std::min (size_t (vox.size(1)-bottom[1]), top[1]+2-bottom[1]);
          top[2] = std::min (size_t (vox.size(2)-bottom[2]), top[2]+2-bottom[2]);

          auto sub = Adapter::make<Adapter::Subset> (vox, bottom, top);

          image = Image<float>::scratch (sub);
          copy (sub, image);
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
          interp = Interp::Linear<Image<float>> (image);
#else
          voxel2scanner = Transform (image).voxel2scanner;
#endif

          volume *= image.size(0) * image.size(1) * image.size(2);

        }



        bool Rejection::get_seed (Math::RNG::Uniform<float>& rng, Eigen::Vector3f& p) 
        {
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
          Eigen::Vector3f pos;
          float selector;
          do {
            pos = {
              rng() * (image.size(0)-1), 
              rng() * (image.size(1)-1), 
              rng() * (image.size(2)-1) 
            };
            interp.voxel (pos);
            selector = rng.Uniform() * max;
          } while (interp.value() < selector);
          p = interp.voxel2scanner * pos;
#else
          auto seed = image;
          float selector;
          do {
            seed.index(0) = std::uniform_int_distribution<int> (0, image.size(0)-1) (rng.rng);
            seed.index(1) = std::uniform_int_distribution<int> (0, image.size(1)-1) (rng.rng);
            seed.index(2) = std::uniform_int_distribution<int> (0, image.size(2)-1) (rng.rng);
            selector = rng() * max;
          } while (seed.value() < selector);
          p = { seed.index(0)+rng()-0.5f, seed.index(1)+rng()-0.5f, seed.index(2)+rng()-0.5f };
          p = voxel2scanner.cast<float>() * p;
#endif
          return true;
        }




      }
    }
  }
}


