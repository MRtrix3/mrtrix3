/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */



#include "dwi/tractography/seeding/basic.h"
#include "dwi/tractography/rng.h"
#include "adapter/subset.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {


        bool Sphere::get_seed (Eigen::Vector3f& p) const
        {
          std::uniform_real_distribution<float> uniform;
          do {
            p = { 2.0f*uniform(*rng)-1.0f, 2.0f*uniform(*rng)-1.0f, 2.0f*uniform(*rng)-1.0f };
          } while (p.squaredNorm() > 1.0f);
          p = pos + rad*p;
          return true;
        }





        bool SeedMask::get_seed (Eigen::Vector3f& p) const
        {
          auto seed = mask;
          do {
            seed.index(0) = std::uniform_int_distribution<int>(0, mask.size(0)-1)(*rng);
            seed.index(1) = std::uniform_int_distribution<int>(0, mask.size(1)-1)(*rng);
            seed.index(2) = std::uniform_int_distribution<int>(0, mask.size(2)-1)(*rng);
          } while (!seed.value());
          std::uniform_real_distribution<float> uniform;
          p = { seed.index(0)+uniform(*rng)-0.5f, seed.index(1)+uniform(*rng)-0.5f, seed.index(2)+uniform(*rng)-0.5f };
          p = (*mask.voxel2scanner) * p;
          return true;
        }






        bool Random_per_voxel::get_seed (Eigen::Vector3f& p) const
        {

          std::lock_guard<std::mutex> lock (mutex);

          if (expired)
            return false;

          if (mask.index(2) < 0 || ++inc == num) {
            inc = 0;

            do {
              if (++mask.index(2) == mask.size(2)) {
                mask.index(2) = 0;
                if (++mask.index(1) == mask.size(1)) {
                  mask.index(1) = 0;
                  ++mask.index(0);
                }
              }
            } while (mask.index(0) != mask.size(0) && !mask.value());

            if (mask.index(0) == mask.size(0)) {
              expired = true;
              return false;
            }
          }

          std::uniform_real_distribution<float> uniform;
          p = { mask.index(0)+uniform(*rng)-0.5f, mask.index(1)+uniform(*rng)-0.5f, mask.index(2)+uniform(*rng)-0.5f };
          p = (*mask.voxel2scanner) * p;
          return true;
        }








        bool Grid_per_voxel::get_seed (Eigen::Vector3f& p) const
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
                  if (++mask.index(2) == mask.size(2)) {
                    mask.index(2) = 0;
                    if (++mask.index(1) == mask.size(1)) {
                      mask.index(1) = 0;
                      ++mask.index(0);
                    }
                  }
                } while (mask.index(0) != mask.size(0) && !mask.value());
                if (mask.index(0) == mask.size(0)) {
                  expired = true;
                  return false;
                }
              }
            }
          }

          p = { mask.index(0)+offset+(pos[0]*step), mask.index(1)+offset+(pos[1]*step), mask.index(2)+offset+(pos[2]*step) };
          p = (*mask.voxel2scanner) * p;
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
          if (!(vox.ndim() == 3 || (vox.ndim() == 4 && vox.size(3) == 1)))
            throw Exception ("Seed image must be a 3D image");
          std::vector<size_t> bottom (3, std::numeric_limits<size_t>::max());
          std::vector<size_t> top    (3, 0);

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
          Header header = sub;
          header.ndim() = 3;

          auto buf = Image<float>::scratch (header);
          volume *= buf.spacing(0) * buf.spacing(1) * buf.spacing(2);

          copy (sub, buf, 0, 3);
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
          interp = Interp::Linear<Image<float>> (buf);
#else
          image = buf;
          voxel2scanner = Transform (image).voxel2scanner.cast<float>();
#endif
        }



        bool Rejection::get_seed (Eigen::Vector3f& p) const
        {
          std::uniform_real_distribution<float> uniform;
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
          auto seed = interp;
          float selector;
          Eigen::Vector3f pos;
          do {
            pos = {
              uniform (*rng) * (interp.size(0)-1), 
              uniform (*rng) * (interp.size(1)-1), 
              uniform (*rng) * (interp.size(2)-1) 
            };
            seed.voxel (pos);
            selector = rng->Uniform() * max;
          } while (seed.value() < selector);
          p = interp.voxel2scanner * pos;
#else
          auto seed = image;
          float selector;
          do {
            seed.index(0) = std::uniform_int_distribution<int> (0, image.size(0)-1) (*rng);
            seed.index(1) = std::uniform_int_distribution<int> (0, image.size(1)-1) (*rng);
            seed.index(2) = std::uniform_int_distribution<int> (0, image.size(2)-1) (*rng);
            selector = uniform (*rng) * max;
          } while (seed.value() < selector);
          p = { seed.index(0)+uniform(*rng)-0.5f, seed.index(1)+uniform(*rng)-0.5f, seed.index(2)+uniform(*rng)-0.5f };
          p = voxel2scanner * p;
#endif
          return true;
        }




      }
    }
  }
}


